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

#include "event_graph.hpp"
#include "network.hpp"
#include "out_component_size_estimate.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#include "cxxopts.hpp"

#pragma GCC diagnostic pop

cxxopts::Options define_options() {
  cxxopts::Options options("largest_out_component",
      "largest out-component of a temporal network");

  options.add_options()
    ("s,seed", "random number generator seed (required)",
     cxxopts::value<size_t>())
    ("dt",
     "delta-t parameter. If --exponential is used this indicates expected "
     "value of dts of dag events that are not removed, otherwise this is "
     "maximum dt of dag",
     cxxopts::value<temp_time>()->default_value("1"))
    ("prob-dist",
     "adjacency probability function. Values: deterministic (default) or"
     " exponential",
     cxxopts::value<std::string>()->default_value("deterministic"))
    ("size-measure",
     "measure used to find the maximum: events (default) or nodes",
     cxxopts::value<std::string>()->default_value("events"))
    ("significance",
     "set the probability of not getting the correct maximum out-component",
     cxxopts::value<double>()->default_value("0.001"))
    ("exponential",
     "exponentially decay probably of a link in event graph w.r.t. time")
    ("h,help", "Print help")
    ;

  options.add_options("Event List File")
    ("temporal-reserve", "estimated size of temporal network",
     cxxopts::value<size_t>()->default_value("0"))
    ("n,network", "network in event-list format (required)",
     cxxopts::value<std::string>())
    ;

  options.add_options("Output")
    ("summary", "file to store summary statistics of the network",
     cxxopts::value<std::string>())
    ("out-component-sizes", "file to store out-component sizes of all events",
     cxxopts::value<std::string>())
    ("weakly-component-sizes", "file to store weakly connected component "
     "distributions of the network",
     cxxopts::value<std::string>())
    ;
  return options;
}

class null_buffer : public std::streambuf {
  public:
    int overflow(int c) { return c; }
};


template <prob_dist_types dist_type>
double prob_dist(const temp_edge& a, const temp_edge& b, temp_time dt);

template <>
double prob_dist<prob_dist_types::deterministic>(
    const temp_edge& a, const temp_edge& b, temp_time max_dt) {
  if (b.time > a.effect_time() && b.time - a.effect_time() < max_dt)
    return 1;
  else
    return 0;
}

template <>
double prob_dist<prob_dist_types::exponential>(
    const temp_edge& a, const temp_edge& b, temp_time avg_dt) {
    temp_time dt = b.time - a.time;
    double lambda = (1.0/(double)avg_dt);
    return lambda*std::exp(-lambda*((double)dt));
}

struct options_t {
  public:
    size_t seed;
    uint32_t hll_seed;

    bool summary() {
      return !summary_filename.empty();
    }
    std::string summary_filename;

    bool out_comps_file() {
      return !out_comps_filename.empty();
    }
    std::string out_comps_filename;

    bool weakly_comps_file() {
      return !weakly_comps_filename.empty();
    }
    std::string weakly_comps_filename;

    size_t temporal_reserve = 0;
    std::string network_filename;

    prob_dist_types prob_dist_type;
    std::function<double(const temp_edge& a, const temp_edge& b, temp_time dt)>
      prob_dist;

    size_measures size_measure;

    double significance;

    temp_time dt;
};

options_t parse_options(int argc, const char* argv[]) {
  cxxopts::Options option_defs = define_options();
  auto options = option_defs.parse(argc, argv);
  options_t opts;

  if (options.count("help") != 0) {
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(0);
  }


  if (options.count("seed") == 0) {
    std::cerr << "ERROR: needs a seed argument" << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }
  opts.seed = options["seed"].as<size_t>();
  std::mt19937_64 gen(opts.seed);
  std::uniform_int_distribution<uint32_t> sd;
  opts.hll_seed = sd(gen);

  if (options.count("network") == 0) {
    std::cerr << "ERROR: needs a network argument" << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }

  opts.network_filename = options["network"].as<std::string>();

  double sig =  options["significance"].as<double>();
  if (sig > 0 && sig <= 1)
    opts.significance = sig;
  else {
    std::cerr << "ERROR: significance should be in (0, 1] range"
      << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }



  if (options.count("temporal-reserve") != 0)
    opts.temporal_reserve = options["temporal-reserve"].as<size_t>();

  if (options["size-measure"].as<std::string>() == "events") {
    opts.size_measure = size_measures::events;
  } else if (options["size-measure"].as<std::string>() == "nodes") {
    opts.size_measure = size_measures::nodes;
  } else {
    std::cerr << "ERROR: needs a correct size-measure"
      << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }

  if (options.count("summary") != 0)
    opts.summary_filename = options["summary"].as<std::string>();

  if (options.count("weakly-component-sizes") != 0)
    opts.weakly_comps_filename = options["weakly-component-sizes"].as<std::string>();

  if (options.count("out-component-sizes") != 0)
    opts.out_comps_filename = options["out-component-sizes"].as<std::string>();

  opts.dt = options["dt"].as<temp_time>();

  if (options["prob-dist"].as<std::string>() == "deterministic") {
    opts.prob_dist_type = prob_dist_types::deterministic;
    opts.prob_dist = prob_dist<prob_dist_types::deterministic>;
  } else if (options["prob-dist"].as<std::string>() == "exponential") {
    opts.prob_dist_type = prob_dist_types::exponential;
    opts.prob_dist = prob_dist<prob_dist_types::exponential>;
  } else {
    std::cerr << "ERROR: needs a correct probability distribtuion type"
      << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }

  return opts;
}

template <class EdgeT>
void log_weakly_component_sizes(
    const event_graph<EdgeT>& eg,
    std::ofstream& summary_file,
    std::ofstream& weakly_comps_file) {

  using TimeType =  typename EdgeT::TimeType;

  auto weakly_start = std::clock();
  auto weakly_comps = weakly_connected_components(eg, true);
  auto weakly_end = std::clock();
  summary_file << "weakly-time: "
    << (double)(1000 * (weakly_end-weakly_start))/CLOCKS_PER_SEC
    << std::endl;

  size_t e_max = std::numeric_limits<size_t>::lowest(),
         g_max = std::numeric_limits<size_t>::lowest();
  TimeType lt_max = std::numeric_limits<TimeType>::lowest();


  for (auto&& w: weakly_comps) {
    counter<EdgeT, exact_estimator> c(0, w.size(), w.size()/2);
    for (auto&& e: w)
      c.insert(e);

    TimeType start, end;
    std::tie(start, end) = c.lifetime();

    e_max = std::max(c.edge_set().size(), e_max);
    g_max = std::max(c.node_set().size(), g_max);
    lt_max = std::max(end - start, lt_max);

    weakly_comps_file << c.edge_set().size() << " "
      << c.node_set().size() << " " << start << " " << end << "\n";
  }

  summary_file << "largest-weakly-e: " << e_max << std::endl;
  summary_file << "largest-weakly-g: " << g_max << std::endl;
  summary_file << "largest-weakly-lt: " << lt_max << std::endl;
}

int main(int argc, const char* argv[]) {
  options_t opts = parse_options(argc, argv);

  null_buffer null_buf;
  std::ofstream summary_file;
  if (opts.summary())
    summary_file.open(opts.summary_filename);
  else
    summary_file.basic_ios<char>::rdbuf(&null_buf); // nope nope nope

  std::ofstream weakly_comps_file;
  if (opts.weakly_comps_file())
    weakly_comps_file.open(opts.weakly_comps_filename);
  else
    weakly_comps_file.basic_ios<char>::rdbuf(&null_buf); // nope nope nope


  std::vector<temp_edge> events = event_list<temp_edge>(
      opts.network_filename,
      opts.temporal_reserve);

  auto eg = event_graph<temp_edge>(events, opts.dt, opts.prob_dist, opts.seed,
      opts.prob_dist_type == prob_dist_types::deterministic);

  summary_file << "seed: `" << opts.seed << "'" << std::endl;
  summary_file << "dt: " << opts.dt << std::endl;

  summary_file << "temporal-vertices: " << eg.node_count() << std::endl;
  summary_file << "temporal-edges: " << eg.event_count() << std::endl;

  temp_time min_t, max_t;
  std::tie(min_t, max_t) = eg.time_window();
  summary_file << "time-min: " << min_t << std::endl;
  summary_file << "time-max: " << max_t << std::endl;

  log_weakly_component_sizes(eg, summary_file, weakly_comps_file);

  using probabilistic_counter = counter<temp_edge, hll_estimator_readonly>;
  auto estimation_start = std::clock();
  std::vector<std::pair<temp_edge, probabilistic_counter>>
    out_comp_size = out_component_size_estimate<temp_edge>(
        eg, opts.hll_seed, false); // return the estimation for all events
  auto estimation_end = std::clock();
  summary_file << "estimation-time: "
    << (double)(1000 * (estimation_end-estimation_start))/CLOCKS_PER_SEC
    << std::endl;


  std::ofstream out_comps_file;
  if (opts.out_comps_file())
    out_comps_file.open(opts.out_comps_filename);
  else
    out_comps_file.basic_ios<char>::rdbuf(&null_buf); // nope nope nope

  double e_max = std::numeric_limits<double>::lowest(),
         g_max = std::numeric_limits<double>::lowest();
  temp_time lt_max = std::numeric_limits<temp_time>::lowest();
  temp_time start_max = std::numeric_limits<temp_time>::max(),
            end_max = std::numeric_limits<temp_time>::lowest();

  for (auto&& p: out_comp_size) {
    temp_time start, end;
    std::tie(start, end) = p.second.lifetime();

    e_max = std::max(p.second.edge_set().estimate(), e_max);
    g_max = std::max(p.second.node_set().estimate(), g_max);
    if ((end - start) > lt_max) {
      lt_max = std::max(end - start, lt_max);
      start_max = start;
      end_max = end;
    }

    out_comps_file << p.second.edge_set().estimate() << " "
      << p.second.node_set().estimate() << " " << start << " " << end << "\n";
  }

  summary_file << "largest-out-e: " << e_max << std::endl;
  summary_file << "largest-out-g: " << g_max << std::endl;
  summary_file << "largest-out-lt: " << lt_max << std::endl;

  summary_file << "loc-lt-begin: " << start_max << std::endl;
  summary_file << "loc-lt-end: "   << end_max << std::endl;
}

