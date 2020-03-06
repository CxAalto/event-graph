#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>

#include <hyperloglog.hpp>
#include <dag.hpp>

#define HLL_DENSE_PERC 10
#define NETWORK_TYPE dag::undirected_temporal_network<uint32_t, double>

using hll_t = hll::HyperLogLog<HLL_DENSE_PERC, 19>;

using temp_net = NETWORK_TYPE;
using temp_vert = typename temp_net::VertexType;
using temp_edge = typename temp_net::EdgeType;
using temp_time = typename temp_edge::TimeType;

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
    ("dt", "delta-t parameter. maximum dt of dag",
     cxxopts::value<temp_time>()->default_value("1"))
    ("h,help", "Print help")
    ;

  options.add_options("Event List File")
    ("temporal-reserve", "estimated size of temporal network",
     cxxopts::value<size_t>()->default_value("0"))
    ("n,network", "network in event-list format (required)",
     cxxopts::value<std::string>())
    ;

  options.add_options("Output")
    ("out-component-sizes", "file to store out-component sizes of all events",
     cxxopts::value<std::string>())
    ;
  return options;
}

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


struct options_t {
  public:
    size_t seed;
    uint32_t hll_seed;

    std::string out_comps_filename;

    size_t temporal_reserve = 0;
    std::string network_filename;

    prob_dist_types prob_dist_type;
    std::function<double(const temp_edge& a, const temp_edge& b, temp_time dt)>
      prob_dist;

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

  if (options.count("temporal-reserve") != 0)
    opts.temporal_reserve = options["temporal-reserve"].as<size_t>();


  if (options.count("out-component-sizes") == 0) {
    std::cerr << "ERROR: needs an out-component-sizes argument" << std::endl;
    std::cerr << option_defs.help({"", "Event List File", "Output"})
      << std::endl;
    std::exit(1);
  }
  opts.out_comps_filename = options["out-component-sizes"].as<std::string>();

  opts.dt = options["dt"].as<temp_time>();
  opts.prob_dist = prob_dist<prob_dist_types::deterministic>;

  return opts;
}

int main(int argc, const char* argv[]) {
  options_t opts = parse_options(argc, argv);

  std::vector<temp_edge> events = event_list<temp_edge>(
      opts.network_filename,
      opts.temporal_reserve);

  auto eg = event_graph<temp_edge>(events, opts.dt, opts.prob_dist, opts.seed,
      opts.prob_dist_type == prob_dist_types::deterministic);

  using probabilistic_counter = counter<temp_edge, hll_estimator_readonly>;
  std::vector<std::pair<temp_edge, probabilistic_counter>>
    out_comp_size = out_component_size_estimate<temp_edge>(
        eg, opts.hll_seed, false); // return the estimation for all events

  std::ofstream out_comps_file;
  out_comps_file.open(opts.out_comps_filename);
  out_comps_file
    << "S_e-real" << " "
    << "S_e-est" << " "
    << "S_n-real" << " "
    << "S_n-est" << "\n";
  for (auto&& p: out_comp_size) {
    temp_time start, end;
    std::tie(start, end) = p.second.lifetime();

    auto out_comp = out_component(eg, p.first,
        static_cast<size_t>(p.second.node_set().estimate()+0.5),
        static_cast<size_t>(p.second.edge_set().estimate()+0.5));

    out_comps_file
      << out_comp.edge_set().size() << " "
      << p.second.edge_set().estimate() << " "
      << out_comp.node_set().size() << " "
      << p.second.node_set().estimate() << "\n";
  }
}

