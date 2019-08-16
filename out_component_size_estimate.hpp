#include <unordered_map>

namespace hll {
  template <>
  uint64_t hash(const dag::undirected_temporal_edge<temp_vert, temp_time>& e,
      uint32_t seed) {
    uint32_t a, b;
    std::tie(a, b) = std::minmax(e.v1, e.v2);
    uint64_t h1 = hll::hash(a, seed);
    uint64_t h2 = hll::hash(b, seed);
    uint64_t ht = hll::hash(e.time, seed);
    h1 ^= h2 + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    h1 ^= ht + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    return h1;
  }

  template <>
  uint64_t hash(const dag::directed_temporal_edge<temp_vert, temp_time>& e,
      uint32_t seed) {
    uint64_t h1 = hll::hash(e.v1, seed);
    uint64_t h2 = hll::hash(e.v2, seed);
    uint64_t ht = hll::hash(e.time, seed);
    h1 ^= h2 + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    h1 ^= ht + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    return h1;
  }

  template <>
  uint64_t hash(const dag::directed_delayed_temporal_edge<temp_vert, temp_time>& e,
      uint32_t seed) {
    uint64_t h1 = hll::hash(e.v1, seed);
    uint64_t h2 = hll::hash(e.v2, seed);
    uint64_t ht = hll::hash(e.time, seed);
    uint64_t hd = hll::hash(e.delay, seed);
    h1 ^= h2 + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    h1 ^= ht + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    h1 ^= hd + 0x9e3779b97f4a7c15 + (h1<<6) + (h1>>2);
    return h1;
  }
}

template <class EdgeT,
         template<typename> class EstimatorT = hll_estimator,
         template<typename> class ReadOnlyEstimatorT = hll_estimator_readonly>
std::vector<std::pair<EdgeT, counter<EdgeT, ReadOnlyEstimatorT>>>
out_component_size_estimate(
    const event_graph<EdgeT>& eg,
    uint32_t seed,
    bool only_roots=false) {


  std::unordered_map<EdgeT, counter<EdgeT, EstimatorT>> out_components;
  std::vector<std::pair<EdgeT, counter<EdgeT, ReadOnlyEstimatorT>>>
    out_component_ests;
  out_component_ests.reserve(eg.topo().size());

  std::unordered_map<EdgeT, size_t> in_degrees;

  size_t log_increment = eg.topo().size()/20;

  auto temp_edge_iter = eg.topo().rbegin();
  while (temp_edge_iter < eg.topo().rend()) {
    if (log_increment > 10'000 &&
        std::distance(eg.topo().rbegin(), temp_edge_iter) % log_increment == 0)
      std::cerr <<
        std::distance(
            eg.topo().rbegin(),
            temp_edge_iter)*100/eg.topo().size() <<
        "\% processed" << std::endl;

    out_components.emplace(*temp_edge_iter, seed);
    in_degrees[*temp_edge_iter] =
      eg.predecessors(*temp_edge_iter).size();

    for (const auto& other:
        eg.successors(*temp_edge_iter)) {

      out_components.at(*temp_edge_iter).merge(out_components.at(other));

      if (--in_degrees.at(other) == 0) {
        if (!only_roots)
          out_component_ests.emplace_back(other,
              out_components.at(other));
        out_components.erase(other);
        in_degrees.erase(other);
      }
    }

    out_components.at(*temp_edge_iter).insert(*temp_edge_iter);

    if (in_degrees.at(*temp_edge_iter) == 0) {
      out_component_ests.emplace_back(*temp_edge_iter,
        out_components.at(*temp_edge_iter));
      out_components.erase(*temp_edge_iter);
      in_degrees.erase(*temp_edge_iter);
    }
    temp_edge_iter++;
  }

  if (only_roots)
    out_component_ests.shrink_to_fit();

  return out_component_ests;
}



template <class EdgeT>
counter<EdgeT, exact_estimator> out_component(
    const event_graph<EdgeT>& eg,
    const EdgeT& root,
    size_t node_size_est,
    size_t edge_size_est) {

  using TimeT = typename EdgeT::TimeType;
  using VertT = typename EdgeT::VertexType;
  using delayed = dag::directed_delayed_temporal_edge<VertT, TimeT>;
  using directed = dag::directed_temporal_edge<VertT, TimeT>;
  using undirected = dag::undirected_temporal_edge<VertT, TimeT>;

  if (!eg.deterministic())
    return generic_out_component(
        eg, root, node_size_est, edge_size_est);
  else if constexpr (std::is_same<EdgeT, directed>::value ||
            std::is_same<EdgeT, undirected>::value ||
            std::is_same<EdgeT, delayed>::value)
    return deterministic_out_component(
        eg, root, node_size_est, edge_size_est);
  else
    return generic_out_component(
        eg, root, node_size_est, edge_size_est);
}

template <class EdgeT>
counter<EdgeT, exact_estimator> generic_out_component(
    const event_graph<EdgeT>& eg,
    const EdgeT& root,
    size_t node_size_est,
    size_t edge_size_est) {

  std::queue<EdgeT> search({root});
  counter<EdgeT, exact_estimator> out_component(0, edge_size_est, node_size_est);
  out_component.insert(root);

  while (!search.empty()) {
    EdgeT e = search.front();
    search.pop();
    for (auto&& s: eg.successors(e))
      if (!out_component.edge_set().contains(s)) {
        search.push(s);
        out_component.insert(s);
      }
  }

  return out_component;
}



template <class EdgeT>
counter<EdgeT, exact_estimator> deterministic_out_component(
    const event_graph<EdgeT>& eg,
    const EdgeT& root,
    size_t node_size_est,
    size_t edge_size_est) {

  auto comp_function = [](const EdgeT& e1, const EdgeT& e2) {
    return (e1.effect_time()) > (e2.effect_time());
  };

  std::priority_queue<
    EdgeT, std::vector<EdgeT>,
    decltype(comp_function)> in_transition(comp_function);

  in_transition.push(root);

  using VertexType = typename EdgeT::VertexType;
  using TimeType = typename EdgeT::TimeType;

  counter<EdgeT, exact_estimator, exact_estimator>
    out_component(0, edge_size_est, node_size_est);
  out_component.insert(root);

  std::unordered_map<VertexType, TimeType> last_infected;
  last_infected.reserve(eg.topo().size());

  for (auto && v: root.mutated_verts())
    last_infected[v] = root.effect_time();

  TimeType last_infect_time = root.effect_time();

  auto topo_it = std::upper_bound(eg.topo().begin(), eg.topo().end(), root);

  while (topo_it < eg.topo().end() &&
      (topo_it->time < last_infect_time ||
       topo_it->time - last_infect_time < eg.expected_dt())) {

    while (!in_transition.empty() &&
        in_transition.top().effect_time() < topo_it->time) {
      for (auto && v: in_transition.top().mutated_verts()) {
        last_infected[v] = in_transition.top().effect_time();
      }
      out_component.insert(in_transition.top());
      in_transition.pop();
    }


    bool is_infecting = false;

    for (auto && v: topo_it->mutator_verts()) {
      auto last_infected_it = last_infected.find(v);
      if (last_infected_it != last_infected.end() &&
          topo_it->time > last_infected_it->second &&
          topo_it->time - last_infected_it->second < eg.expected_dt())
        is_infecting = true;
    }

    if (is_infecting) {
      if (topo_it->time == topo_it->effect_time()) {
        out_component.insert(*topo_it);
        for (auto && v: topo_it->mutated_verts())
          last_infected[v] = topo_it->time;
      } else in_transition.push(*topo_it);
      last_infect_time =
        std::max(topo_it->effect_time(), last_infect_time);
    }

    topo_it++;
  }

  while (!in_transition.empty()) {
      out_component.insert(in_transition.top());
      in_transition.pop();
  }

  return out_component;
}
