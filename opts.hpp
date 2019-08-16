#ifndef OPTS_H
#define OPTS_H

#include <random>
#include <string>

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
    ("largest-out-component", "file to store largest out-component events",
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

template <>
double prob_dist<prob_dist_types::exponential>(
    const temp_edge& a, const temp_edge& b, temp_time avg_dt) {
  if (b.time < a.effect_time())
    return 0;
  temp_time dt = b.time - a.effect_time();
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

    bool loc() {
      return !loc_filename.empty();
    }
    std::string loc_filename;

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

#endif /* OPTS_H */
