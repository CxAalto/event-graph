#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>

#include <hyperloglog.hpp>
#include <dag.hpp>

#ifndef HLL_DENSE_PERC
#pragma message "HLL_DENSE_PERC undefined. Using default definition."
#define HLL_DENSE_PERC 12
#endif

#ifndef NETWORK_TYPE
#pragma message "NETWORK_TYPE undefined. Using default definition."
#define NETWORK_TYPE dag::undirected_temporal_network<uint32_t, double>
#endif

using hll_t = hll::HyperLogLog<HLL_DENSE_PERC, 19>;

using temp_net = NETWORK_TYPE;
using temp_vert = typename temp_net::VertexType;
using temp_edge = typename temp_net::EdgeType;
using temp_time = typename temp_edge::TimeType;

enum class size_measures { events, nodes };
enum class prob_dist_types { deterministic, exponential };

#include "measures.hpp"

#include "opts.hpp"
#include "event_graph.hpp"
#include "network.hpp"
#include "out_component_size_estimate.hpp"

auto parse_options(int, const char*);


class null_buffer : public std::streambuf {
  public:
    int overflow(int c) { return c; }
};

size_t measure_size(
    const counter<temp_edge, exact_estimator>& c,
    size_measures measure) {
  if (measure == size_measures::events)
    return c.edge_set().size();
  else
    return c.node_set().size();
}

size_t measure_size(
    const event_graph<temp_edge>& eg,
    size_measures measure) {
  if (measure == size_measures::events)
    return eg.event_count();
  else
    return eg.node_count();
}

double measure_estimate(
    const counter<temp_edge, hll_estimator_readonly>& c,
    size_measures measure) {
  if (measure == size_measures::events)
    return c.edge_set().estimate();
  else
    return c.node_set().estimate();
}

using probabilistic_counter = counter<temp_edge, hll_estimator_readonly>;
using exact_counter = counter<temp_edge, exact_estimator>;

// We didn't use const std::vector<...> out_comps because we explicitly want a
// copy to manipulate (sort and pop and on)
exact_counter largest_out_component(
    const event_graph<temp_edge>& eg,
    std::vector<std::pair<temp_edge, probabilistic_counter>> out_comps,
    size_measures measure,
    double significance) {

  std::sort(out_comps.begin(), out_comps.end(),
      [measure] (
        const std::pair<temp_edge, probabilistic_counter>& a,
        const std::pair<temp_edge, probabilistic_counter>& b) {
      return measure_estimate(a.second, measure) <
      measure_estimate(b.second, measure);
      });

  auto loc_event = out_comps.back().first;
  auto loc_est = out_comps.back().second;
  auto loc = out_component(eg, loc_event,
      (size_t)(loc_est.node_set().estimate()*1.05),
      (size_t)(loc_est.edge_set().estimate()*1.05));
  size_t loc_size = measure_size(loc, measure);
  size_t loc_max = measure_size(eg, measure);

  std::cerr << "loc-candidate-size: " << loc_size <<
    " (est: " << measure_estimate(loc_est, measure) << ")"<< std::endl;

  out_comps.pop_back();

  auto current_candidate = out_comps.begin();

  double cutoff = std::log(1-significance);
  double p_smaller_total = 1.0;

  while (current_candidate != out_comps.end() &&
      p_smaller_total > cutoff) {
    double p_larger = hll_estimator_readonly<temp_edge>::p_larger(
            measure_estimate(current_candidate->second, measure),
            loc_size, loc_max);
    p_smaller_total += std::log(1.0 - p_larger);
    current_candidate++;
  }

  current_candidate--;

  std::cerr << "manual-bfs: " << out_comps.end() - current_candidate
    << std::endl;

  while (current_candidate != out_comps.end()) {
    // we are giving an upper bound on out-component size to prevent rehash in
    // worst-case, but number of vertices are usually exact.
    auto current_est = current_candidate->second;
    auto out_comp = out_component(eg, current_candidate->first,
        (size_t)(current_est.node_set().estimate()*1.05),
        (size_t)(current_est.edge_set().estimate()*1.05));
    auto comp_size = measure_size(out_comp, measure);
    if (comp_size > loc_size) {
      loc = out_comp;
      loc_size = comp_size;
      loc_event = current_candidate->first;
    }
    if ((out_comps.end() - current_candidate) % 10 == 0)
      std::cerr << "manual bfs remaining: " << (out_comps.end() - current_candidate) <<
        " last size: "<< comp_size
        << " (est: " << measure_estimate(current_est, measure) << ")"
        << std::endl;
    current_candidate++;
  }

  return loc;
}


int main(int argc, const char* argv[]) {
  options_t opts = parse_options(argc, argv);

  null_buffer null_buf;

  std::ofstream summary_file;
  if (opts.summary())
    summary_file.open(opts.summary_filename);
  else
    summary_file.basic_ios<char>::rdbuf(&null_buf); // nope nope nope

  summary_file << "seed: `" << opts.seed << "'" << std::endl;
  summary_file << "dt: " << opts.dt << std::endl;

  event_graph<temp_edge> eg;
  {
    std::vector<temp_edge> events = event_list<temp_edge>(
        opts.network_filename,
        opts.temporal_reserve);

    eg = event_graph<temp_edge>(
        events, opts.dt, opts.prob_dist, opts.seed,
        opts.prob_dist_type == prob_dist_types::deterministic);
  }

  summary_file << "temporal-vertices: " << eg.node_count() << std::endl;
  summary_file << "temporal-edges: " << eg.event_count() << std::endl;

  temp_time min_t, max_t;
  std::tie(min_t, max_t) = eg.time_window();
  summary_file << "time-min: " << min_t << std::endl;
  summary_file << "time-max: " << max_t << std::endl;


  auto estimate_start = std::clock();
  std::vector<std::pair<temp_edge, probabilistic_counter>>
    out_comp_size = out_component_size_estimate<temp_edge>(
        eg,
        opts.hll_seed,
        true); // return the estimation only for events with no predecessor
  auto estimate_end = std::clock();
  summary_file << "estimate-time: "
    << (double)(1000 * (estimate_end-estimate_start))/CLOCKS_PER_SEC
    << std::endl;

  summary_file << "root-events: " << out_comp_size.size() << std::endl;

  auto largest_e_start = std::clock();
  auto loc_events =
    largest_out_component(eg, out_comp_size, size_measures::events, opts.significance);
  auto largest_e_end = std::clock();
  summary_file << "largest-e-search-time: "
    << (double)(1000 * (largest_e_end-largest_e_start))/CLOCKS_PER_SEC
    << std::endl;

  summary_file << "loc-e: " << loc_events.edge_set().size() << std::endl;



  auto largest_g_start = std::clock();
  auto loc_nodes =
    largest_out_component(eg, out_comp_size, size_measures::nodes, opts.significance);
  auto largest_g_end = std::clock();
  summary_file << "largest-g-search-time: "
    << (double)(1000 * (largest_g_end-largest_g_start))/CLOCKS_PER_SEC
    << std::endl;

  summary_file << "loc-g: " << loc_nodes.node_set().size() << std::endl;



  auto largest_lt_start = std::clock();

  auto max_it = out_comp_size.begin();
  for (auto it = out_comp_size.begin(); it < out_comp_size.end(); it++) {
    temp_time t1, t2;
    std::tie(t1, t2) = it->second.lifetime();

    temp_time max_t1, max_t2;
    std::tie(max_t1, max_t2) = max_it->second.lifetime();

    if ((max_t2 - max_t1) < (t2 - t1))
      max_it = it;
  }

  auto loc_lt = out_component(eg, max_it->first,
      (size_t)(max_it->second.node_set().estimate()*1.05),
      (size_t)(max_it->second.edge_set().estimate()*1.05));
  auto largest_lt_end = std::clock();
  summary_file << "largest-lt-search-time: "
    << (double)(1000 * (largest_lt_end-largest_lt_start))/CLOCKS_PER_SEC
    << std::endl;

  temp_time max_t1, max_t2;
  std::tie(max_t1, max_t2) = loc_lt.lifetime();
  summary_file << "loc-lt: " << max_t2 - max_t1 << std::endl;
  summary_file << "loc-lt-begin: " << max_t1 << std::endl;
  summary_file << "loc-lt-end: "   << max_t2 << std::endl;
}
