#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>

#include <hyperloglog.hpp>
#include <dag.hpp>

#define HLL_DENSE_PERC 10

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

double dist(const temp_edge& a, const temp_edge& b, temp_time max_dt) {
    if (b.time > a.time && b.time - a.time < max_dt)
      return 1;
    else
      return 0;
}

#include "measures.hpp"


#include "opts.hpp"
#include "event_graph.hpp"
#include "network.hpp"
#include "out_component_size_estimate.hpp"


template <typename T>
class set_estimator {
  public:
  set_estimator(uint32_t /*seed*/, size_t size_est) {
    if (size_est > 0)
      _set.reserve(size_est);
  }

  size_t size() const { return _set.size(); }
  double estimate() const { return (double)size(); }

  void insert(const T& item) { _set.insert(item); }

  void merge(const set_estimator<T>& other) {
    for (auto&& t: other.set()) insert(t);
  }

  const std::unordered_set<T>& set() const { return _set; }

  private:
  std::unordered_set<T> _set;
};


using probabilistic_counter = counter<temp_edge, hll_estimator_readonly>;
using exact_counter = counter<temp_edge, set_estimator>;



int main(int argc, const char* argv[]) {
  if (argc < 2) {
    std::cout << "no input file" << std::endl;
    return 1;
  }
  std::vector<temp_edge> events = event_list<temp_edge>(argv[1], 0);
  auto eg = event_graph<temp_edge>(events, (temp_time)72000, dist, 1, true);

  std::mt19937_64 gen(1);
  std::uniform_int_distribution<uint32_t> sd;
  uint32_t hll_seed = sd(gen);

  std::vector<std::pair<temp_edge, exact_counter>>
    out_comp_ext = out_component_size_estimate<temp_edge, set_estimator, set_estimator>(
        eg, hll_seed, false);

  std::sort(out_comp_ext.begin(), out_comp_ext.end(),
      [] (
        const std::pair<temp_edge, exact_counter>& a,
        const std::pair<temp_edge, exact_counter>& b) {
        return a.first < b.first;
      });


  std::vector<std::pair<temp_edge, probabilistic_counter>>
    out_comp_est = out_component_size_estimate<temp_edge, hll_estimator, hll_estimator_readonly>(
        eg, hll_seed, false);

  std::sort(out_comp_est.begin(), out_comp_est.end(),
      [] (
        const std::pair<temp_edge, probabilistic_counter>& a,
        const std::pair<temp_edge, probabilistic_counter>& b) {
        return a.first < b.first;
      });


  for (size_t i = 0; i < out_comp_ext.size(); i++) {

    temp_edge e1 = out_comp_est.at(i).first, e2 = out_comp_ext.at(i).first;
    if (!(e1 == e2))
      std::cerr << "omgomg" << std::endl;


    auto loc_det = deterministic_out_component(eg, e1,
        (size_t)(out_comp_ext.at(i).second.node_set().estimate()),
        (size_t)(out_comp_ext.at(i).second.edge_set().estimate()));
    auto loc_gen = generic_out_component(eg, e1,
        (size_t)(out_comp_ext.at(i).second.node_set().estimate()),
        (size_t)(out_comp_ext.at(i).second.edge_set().estimate()));


    double edge_count_det = (double)loc_det.edge_set().size();
    double edge_count_gen = (double)loc_gen.edge_set().size();

    double edge_count_est_backtrack = (double)out_comp_est.at(i).second.edge_set().estimate();
    double edge_count_ext_backtrack = (double)out_comp_ext.at(i).second.edge_set().estimate();

    std::cout
      << edge_count_det << " " << edge_count_gen << " "
      << edge_count_est_backtrack << " " << edge_count_ext_backtrack << std::endl;
  }
}
