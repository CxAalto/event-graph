#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <algorithm>

#include <hyperloglog.hpp>
#include <dag.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#include "cxxopts.hpp"

#pragma GCC diagnostic pop

#ifndef NETWORK_TYPE
#pragma message "NETWORK_TYPE undefined. Using default definition."
#define NETWORK_TYPE dag::undirected_temporal_network<uint32_t, double>
#endif

using hll_t = hll::HyperLogLog<18, 19>;

using temp_net = NETWORK_TYPE;
using temp_vert = typename temp_net::VertexType;
using temp_edge = typename temp_net::EdgeType;
using temp_time = typename temp_edge::TimeType;

enum class size_measures { events, nodes };

double dt_prob_dist(
    const temp_edge& a, const temp_edge& b, temp_time max_dt) {
  if (b.time > a.effect_time() && b.time - a.effect_time() < max_dt)
    return 1;
  else
    return 0;
}

#include "event_graph.hpp"
#include "network.hpp"
#include "measures.hpp"
#include "out_component_size_estimate.hpp"

cxxopts::Options define_options() {
  cxxopts::Options options("measures out-component size of a sample of events");

  options.add_options()
    ("s,seed", "random number generator seed (required)",
     cxxopts::value<size_t>())
    ("dt",
     "delta-t parameter",
     cxxopts::value<temp_time>()->default_value("1"))
    ("sample-size",
     "number of sample events that we measure out-components for",
     cxxopts::value<std::size_t>()->default_value("1000"))
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
    ("out-component-sizes", "file to store out-component sizes of all sampled events",
     cxxopts::value<std::string>())
    ;
  return options;
}

struct options_t {
  public:
    size_t seed;

    bool summary() {
      return !summary_filename.empty();
    }
    std::string summary_filename;

    size_t sample_size = 0;
    size_t temporal_reserve = 0;
    std::string network_filename;

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

  if (options.count("network") == 0) {
    std::cerr << "ERROR: needs a network argument" << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }

  opts.network_filename = options["network"].as<std::string>();


  if (options.count("temporal-reserve") != 0)
    opts.temporal_reserve = options["temporal-reserve"].as<size_t>();

  opts.sample_size = options["sample-size"].as<size_t>();


  if (options.count("summary") != 0)
    opts.summary_filename = options["summary"].as<std::string>();

  opts.dt = options["dt"].as<temp_time>();

  return opts;
}

class null_buffer : public std::streambuf {
  public:
    int overflow(int c) { return c; }
};

int main(int argc, const char* argv[]) {
  options_t opts = parse_options(argc, argv);

  null_buffer null_buf;

  std::ofstream summary_file;
  if (opts.summary())
    summary_file.open(opts.summary_filename);
  else
    summary_file.basic_ios<char>::rdbuf(&null_buf); // nope nope nope

  summary_file << "dt: " << opts.dt << std::endl;
  summary_file << "sample-size: " << opts.sample_size << std::endl;

  event_graph<temp_edge> eg;
  {
    std::vector<temp_edge> events = event_list<temp_edge>(
        opts.network_filename,
        opts.temporal_reserve);

    eg = event_graph<temp_edge>(
        events, opts.dt, dt_prob_dist, opts.seed,
        true);
  }

  summary_file << "temporal-vertices: " << eg.node_count() << std::endl;
  summary_file << "temporal-edges: " << eg.event_count() << std::endl;

  temp_time min_t, max_t;
  std::tie(min_t, max_t) = eg.time_window();
  summary_file << "time-min: " << min_t << std::endl;
  summary_file << "time-max: " << max_t << std::endl;

  std::unordered_set<temp_edge> roots;
  roots.reserve(opts.sample_size);

  std::mt19937 gen(opts.seed);
  std::sample(
      eg.topo().begin(),
      eg.topo().end(),
      std::inserter(roots, roots.end()),
      opts.sample_size,
      gen);


  std::size_t out_component_total = 0;
  size_t i = 0;
  for (auto&& root: roots) {
    std::cerr << "sample: " << (i++) << std::endl;
    auto out_component_start = std::clock();
    auto oc = out_component(eg, root, 0ul, 0ul);
    auto out_component_end = std::clock();
    out_component_total += (size_t)(out_component_end-out_component_start);
  }
  double out_component_time =
    (double)(1000*out_component_total)/CLOCKS_PER_SEC;
  summary_file << "out-component-time: "
    << out_component_time/(double)roots.size()*(double)eg.event_count()
    << std::endl;
}
