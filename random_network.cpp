#include <vector>
#include <limits>
#include <iostream>
#include <iomanip>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#include "cxxopts.hpp"

#pragma GCC diagnostic pop

#include <dag.hpp>

#ifndef TEMP_VERT_TYPE
#define TEMP_VERT_TYPE uint32_t
#endif

#ifndef TEMP_TIME_TYPE
#define TEMP_TIME_TYPE double
#endif


using temp_vert = TEMP_VERT_TYPE; // just for aesthetic reasons
using temp_time = TEMP_TIME_TYPE; // just for aesthetic reasons
using static_net = dag::undirected_network<temp_vert>;
using temp_net = dag::undirected_temporal_network<temp_vert,
      temp_time>;
using temp_edge = dag::undirected_temporal_edge<temp_vert,
      temp_time>;
using event_net = dag::directed_network<temp_edge>;

enum class size_measures { events, nodes, life_time };

cxxopts::Options define_options();

template<class RealType = double>
class truncated_power_law {

  public:
    RealType exponent, x0, x1, constant, mean;
    std::uniform_real_distribution<RealType> dist;

    truncated_power_law(RealType exponent, RealType x0, RealType x1,
        RealType average) : exponent(exponent), x0(x0), x1(x1) {
        constant = (std::pow(x1,(exponent+1.0)) - std::pow(x0,(exponent+1.0)));
        mean = ((std::pow(x1, exponent+2.0)-std::pow(x0, exponent+2.0))
            /(exponent+2.0))/(constant/(exponent+1.0));
        mean = mean/average;
      }

    template<class Generator>
    RealType operator()(Generator& g) {
      return std::pow(
          (constant*dist(g) + std::pow(x0, exponent+1.0)),
          (1.0/(exponent+1.0)))/mean;
    }
};

template <class Distribution>
temp_net
random_temporal_network(const static_net& net, temp_time max_t,
    double ev_dist, Distribution dist, size_t seed) {
  temp_net temp;
  temp.reserve(std::lround((double)net.edges().size() * max_t/ev_dist));

  for (const auto& e: net.edges()) {
    size_t edge_seed = combine_hash(seed,
        std::hash<typename static_net::EdgeType>{}(e));
    std::mt19937_64 generator(edge_seed);
    temp_time t = (temp_time)dist(generator);
    while (t < max_t) {
      temp.add_edge({e.v1, e.v2, t});
      t += (temp_time)dist(generator);
    }
  }
  return temp;
}

int main(int argc, const char *argv[]) {
  cxxopts::Options option_defs = define_options();
  auto options = option_defs.parse(argc, argv);

  if (options.count("help") != 0) {
    std::cerr << option_defs.help({"", "Random Network", "Event List File",
        "Output"}) << std::endl;
    std::exit(0);
  }


  if (options.count("seed") == 0) {
    std::cerr << "ERROR: needs a seed argument" << std::endl;
    std::cerr << option_defs.help({"", "Random Network", "Event List File",
        "Output"}) << std::endl;
    std::exit(1);
  }

  size_t seed = options["seed"].as<std::size_t>();

  std::mt19937_64 gen(seed);

  std::vector<temp_edge> topo;
  size_t node_count;

  node_count = 1ul << options["node"].as<int>();

  double average_degree = options["average-degree"].as<double>();

  double average_distance = options["average-distance"].as<double>();

  static_net net;
  bool ba = options["barabasi"].as<bool>();
  if (ba)
    net = dag::ba_random_graph<uint32_t>
      (node_count, std::lround(average_degree/2.0), gen);
  else
    net = dag::gnp_random_graph<uint32_t>
      (node_count, average_degree/(double)node_count, gen);


  temp_time max_t = options["max-t"].as<temp_time>();
  bool bursty = options["bursty"].as<bool>();

  temp_net temp;


  if (bursty) {
    truncated_power_law<> dist(-0.7, 0.01, max_t, average_distance);
    temp = random_temporal_network(net, max_t, average_distance, dist, seed);
  } else {
    std::exponential_distribution<> dist(average_distance);
    temp = random_temporal_network(net, max_t, average_distance, dist, seed);
  }


  topo = std::vector<temp_edge>(temp.edges().begin(), temp.edges().end());
  std::sort(topo.begin(), topo.end());
  topo.shrink_to_fit();

  std::cout <<
    std::setprecision(std::numeric_limits<temp_time>::digits10 + 1);

  for (auto &&e: topo)
    std::cout << e.v1 << " " << e.v2 << " " << e.time << std::endl;
}


cxxopts::Options define_options() {
  cxxopts::Options options("network_components",
      "event graph and reachability on temporal networks");

  options.add_options()
    ("s,seed", "seed number (required)",
     cxxopts::value<std::size_t>())
    ("barabasi", "use barabasi albert static networks")
    ("bursty",   "use bursty (truncated power-law) inter-event times")
    ("h,help", "Print help")
    ;

  options.add_options("Random Network")
    ("k,average-degree", "average degree of each node",
     cxxopts::value<double>()->default_value("4"))
    ("n,node", "exponent of number of nodes (n in 2^n)",
     cxxopts::value<int>()->default_value("14"))
    ("T,max-t", "maximum time T simulated",
     cxxopts::value<temp_time>()->default_value("40"))
    ("l,average-distance",
     "average distance between events (lambda of exponential distribution)",
     cxxopts::value<double>()->default_value("1"))
    ;

  return options;
}
