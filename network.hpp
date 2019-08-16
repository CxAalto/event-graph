#include <iostream>
#include <fstream>
#include <limits>

#include <disjoint_set.hpp>


template <class EdgeT>
std::vector<EdgeT> event_list(std::string net_filename,
    size_t temporal_reserve) {

  std::vector<EdgeT> topo;
  if (temporal_reserve > 0)
    topo.reserve(temporal_reserve);

  std::ifstream net(net_filename);

  EdgeT e;
  while (net >> e)
    if (e.v1 != e.v2)
      topo.push_back(e);

  return topo;
}


template <class EdgeT>
std::vector<std::vector<EdgeT>>
weakly_connected_components(const event_graph<EdgeT>& eg, bool singletons=false) {

  size_t log_increment = eg.topo().size()/20;

  auto disj_set = ds::disjoint_set<size_t>(eg.topo().size());

  auto temp_edge_iter = eg.topo().begin();
  while (temp_edge_iter < eg.topo().end()) {
    size_t temp_edge_idx = std::distance(eg.topo().begin(), temp_edge_iter);

    if (log_increment > 10'000 && temp_edge_idx % log_increment == 0)
      std::cerr << temp_edge_idx*100/eg.topo().size() <<
        "\% processed (weakly)" << std::endl;

    for (auto&& other: eg.successors(*temp_edge_iter)) {


      /// We are benefiting from the fact that successors are always after the
      /// original event. Might consider searching linearly for caching or small
      /// networks?
      // auto other_it = std::find(temp_edge_iter+1, eg.topo().end(), other);
      auto other_it = std::lower_bound(temp_edge_iter+1, eg.topo().end(), other);


      size_t other_idx = std::distance(eg.topo().begin(), other_it);
      disj_set.merge(temp_edge_idx, other_idx);
    }

    temp_edge_iter++;
  }

  auto sets = disj_set.sets(singletons);
  std::vector<std::vector<EdgeT>> sets_vector;
  sets_vector.reserve(sets.size());

  for (const auto& p: sets) {
    sets_vector.emplace_back();
    auto& current_set = sets_vector.back();
    current_set.reserve(p.second.size());
    for (const auto& event_idx: p.second)
      current_set.push_back(eg.topo().at(event_idx));
  }

  return sets_vector;
}
