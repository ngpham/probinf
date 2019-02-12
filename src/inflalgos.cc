#include <memory>
#include <vector>
#include <set>
#include <algorithm>
#include <cmath>
#include "datatypes.h"
#include "graph.h"
#include "util.h"
#include "inflalgos.h"

namespace inflalgos {

using std::unique_ptr;
using std::make_unique;
using std::vector;
using std::set;
using util::Dice;
using util::STDice;
using datatypes::NodeMeasure;
using datatypes::Node;
using datatypes::Graph;
using datatypes::GraphByEdges;
using datatypes::NodeIndexedCover;
using datatypes::LInt;
using datatypes::Bicriteria;

void _update_node_measure(
    unique_ptr<vector<LInt>>& node_indexed_measure,
    const unique_ptr<vector<NodeIndexedCover>>& nics,
    const unique_ptr<set<LInt>>& base_nodeids) {

  auto base_ccids = set<LInt>();
  for (auto& u: *base_nodeids) {
    base_ccids.insert(nics->at(u).cc_id);
  }

  for (size_t i = 0; i < node_indexed_measure->size(); i++) {
    auto& m = node_indexed_measure->at(i);

    auto ccid = nics->at(i).cc_id;
    auto find = base_ccids.find(ccid);

    if (find != base_ccids.end()) continue;

    m = nics->at(i).cc_size;
  }
  return;
}

void _gather_node_measure(
    const unique_ptr<vector<LInt>>& node_indexed_measure,
    unique_ptr<vector<NodeMeasure>>& node_measure) {

  for (size_t i = 0; i < node_measure->size(); i++) {
    node_measure->at(i).measure += node_indexed_measure->at(i);
  }
}

NodeMeasure _greedy_exp(
    const unique_ptr<GraphByEdges>& graph_edges,
    const unique_ptr<vector<Node>>& nodes,
    const unique_ptr<set<LInt>>& kset_ids,
    const vector<unique_ptr<Dice>>& dices,
    const int& num_samples,
    const int& num_threads) {

  auto batch_size = num_samples / num_threads;
  auto node_measure = make_unique<vector<NodeMeasure>>(nodes->size());

  for (size_t i = 0; i < node_measure->size(); i++) {
    node_measure->at(i).id = i;
    node_measure->at(i).measure = 0;
  }

  #pragma omp parallel for
  for (int i = 0; i < num_threads; i++) {
    auto node_indexed_measure = make_unique<vector<LInt>>(nodes->size(), 0);

    for (int j = 0; j < batch_size; j++) {
      auto g = graph::sample_graph(graph_edges, nodes, dices[i]);
      auto temp = graph::connected_component(g);
      unique_ptr<vector<NodeIndexedCover>> nics = graph::get_cover(g);

      _update_node_measure(node_indexed_measure, nics, kset_ids);
    }

    #pragma omp critical
    _gather_node_measure(node_indexed_measure, node_measure);
  }

  auto max_iterator = std::max_element(
    node_measure->begin(), node_measure->end(),
    [](const NodeMeasure& lhs, const NodeMeasure& rhs) -> bool {
      return lhs.measure < rhs.measure;
    });

  return *max_iterator;
}

std::unique_ptr<std::vector<datatypes::NodeMeasure>> max_exp_infl(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const int& seed_size,
    const int& num_samples,
    const int& num_threads,
    const int& rand_seed) {

  auto kset = make_unique<vector<NodeMeasure>>();
  kset->reserve(seed_size);
  auto kset_ids = make_unique<set<LInt>>();

  auto dices = vector<unique_ptr<Dice>>(num_threads);
  for (int i = 0; i < num_threads; i++) {
    dices[i] = make_unique<STDice>((i+1) * rand_seed);
  }

  for (int i = 0; i < seed_size; i++) {
    NodeMeasure best = _greedy_exp(graph_edges, nodes, kset_ids, dices, num_samples, num_threads);
    kset->push_back(best);
    kset_ids->insert(best.id);
  }

  return kset;
}

struct NodeLoHiCount {
  LInt id;
  LInt lo, hi, count;

  NodeLoHiCount() : id(0), lo(0), hi(0), count(0) {}

  NodeLoHiCount(LInt id, LInt lo, LInt hi, LInt count) :
    id(id), lo(lo), hi(hi), count(count) {}
};

void _update_feasible_count(
    unique_ptr<vector<NodeLoHiCount>>& node_lhcs,
    const unique_ptr<vector<NodeIndexedCover>>& nics,
    const unique_ptr<set<LInt>>& kset_ids) {

  auto base_ccids = set<LInt>();
  LInt base_covered = 0;
  for (auto& u: *kset_ids) {
    auto iter_bool = base_ccids.insert(nics->at(u).cc_id);
    if (iter_bool.second) base_covered += nics->at(u).cc_size;
  }

  for (size_t i = 0; i < node_lhcs->size(); i++) {
    auto t = kset_ids->find(i);
    if (t != kset_ids->end()) continue;

    auto m = base_covered;
    auto& nlhc = node_lhcs->at(i);

    auto ccid = nics->at(i).cc_id;
    auto find = base_ccids.find(ccid);

    if (find == base_ccids.end()) m += nics->at(i).cc_size;

    if (m > (nlhc.lo + nlhc.hi) / 2) nlhc.count += 1;
  }
  return;
}

void _tally_feasible_count(
    unique_ptr<vector<NodeLoHiCount>>& node_lhcs,
    const unique_ptr<vector<NodeLoHiCount>>& local_node_lhcs) {

  std::transform(
    node_lhcs->begin(), node_lhcs->end(), local_node_lhcs->begin(),
    node_lhcs->begin(),
    [](NodeLoHiCount& acc, NodeLoHiCount& part) -> NodeLoHiCount {
      return NodeLoHiCount(acc.id, acc.lo, acc.hi, acc.count + part.count);
    });

  return;
}

NodeMeasure _greedy_prob(
    const unique_ptr<GraphByEdges>& graph_edges,
    const unique_ptr<vector<Node>>& nodes,
    const double& prob,
    const unique_ptr<set<LInt>>& kset_ids,
    const vector<unique_ptr<Dice>>& dices,
    const int& num_samples,
    const int& num_threads) {

  auto batch_size = num_samples / num_threads;
  auto n = nodes->size();
  auto threshold = prob * num_samples;
  auto num_steps = std::llround(std::log(n) / std::log(2));

  auto node_lhcs = make_unique<vector<NodeLoHiCount>>();
  node_lhcs->reserve(n);
  for (size_t i = 0; i < n; i++) {
    node_lhcs->emplace_back(NodeLoHiCount(i, 1, n, 0));
  }

  for (int e = 0; e < num_steps; e++) {
    for (auto& nlhc: *node_lhcs) nlhc.count = 0;

    #pragma omp parallel for
    for (int j = 0; j < num_threads; j++) {
      auto local_node_lhcs = make_unique<vector<NodeLoHiCount>>();
      local_node_lhcs->reserve(n);
      for (size_t i = 0; i < n; i++) {
        local_node_lhcs->emplace_back(
          NodeLoHiCount(i, node_lhcs->at(i).lo, node_lhcs->at(i).hi, 0));
      }

      for (int i = 0; i < batch_size; i++) {
        auto g = graph::sample_graph(graph_edges, nodes, dices[j]);
        auto tmp = graph::connected_component(g);
        auto nics = graph::get_cover(g);
        _update_feasible_count(local_node_lhcs, nics, kset_ids);
      }

      #pragma omp critical
      _tally_feasible_count(node_lhcs, local_node_lhcs);
    }

    for (auto& nlhc: *node_lhcs) {
      auto mid = (nlhc.lo + nlhc.hi) / 2;
      if (nlhc.count > threshold) {
        nlhc.lo = mid;
      } else {
        nlhc.hi = mid;
      }
    }

  } // for num_steps

  auto max_iterator = std::max_element(
    node_lhcs->begin(), node_lhcs->end(),
    [](const NodeLoHiCount &lhs, const NodeLoHiCount &rhs) -> bool {
      return (lhs.lo < rhs.lo);
    });

  return NodeMeasure(max_iterator->id, max_iterator->lo);
}

std::unique_ptr<std::vector<datatypes::NodeMeasure>> max_prob_infl(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const double& prob,
    const int& seed_size,
    const int& num_samples,
    const int& num_threads,
    const int& rand_seed) {

  auto kset = make_unique<vector<NodeMeasure>>();
  kset->reserve(seed_size);
  auto kset_ids = make_unique<set<LInt>>();

  auto dices = vector<unique_ptr<Dice>>();
  dices.reserve(num_threads);
  for (int i = 0; i < num_threads; i++) {
    dices.push_back(make_unique<STDice>((i+1) * rand_seed));
  }

  for (int i = 0; i < seed_size; i++) {
    NodeMeasure best =
      _greedy_prob(graph_edges, nodes, prob, kset_ids, dices, num_samples, num_threads);
    kset->push_back(best);
    kset_ids->insert(best.id);
  }

  return kset;
}

using CompactSampleCollection = unique_ptr<vector<unique_ptr<vector<NodeIndexedCover>>>>;

CompactSampleCollection _get_samples_collection(
    const unique_ptr<GraphByEdges>& graph_edges,
    const unique_ptr<vector<Node>>& nodes,
    const vector<unique_ptr<Dice>>& dices,
    const size_t& num_samples,
    const size_t& num_threads) {

  auto ret =  make_unique<vector<unique_ptr<vector<NodeIndexedCover>>>>();
  ret->resize(num_samples);

  auto batch_size = num_samples / num_threads;

  #pragma omp parallel for
  for (size_t i = 0; i < num_threads; i++) {
    auto r = i * batch_size;
    for (size_t j = 0; j < batch_size; j++) {
      auto g = graph::sample_graph(graph_edges, nodes, dices[i]);
      auto tmp = graph::connected_component(g);
      auto nics = graph::get_cover(g);
      ret->at(r + j) = std::move(nics);
    }
  }

  return ret;
}

NodeMeasure _greedy_bicriteria(
    const CompactSampleCollection& csc,
    const LInt& cutoff,
    const set<LInt>& base_nodeids) {

  auto fmsr = make_unique<vector<NodeMeasure>>();
  fmsr->reserve(csc->at(0)->size());
  for (size_t i = 0; i < csc->at(0)->size(); i++) {
    fmsr->emplace_back(NodeMeasure(i, 0));
  }

  for (auto& nics: *csc) {
    auto base_ccids = set<LInt>();
    LInt base_covered = 0;
    for (auto& x: base_nodeids) {
      auto iter_bool = base_ccids.insert(nics->at(x).cc_id);
      if (iter_bool.second) base_covered += nics->at(x).cc_size;
    }

    for (size_t i = 0; i < nics->size(); i++) {
      LInt value = 0;

      auto t = base_nodeids.find(i);
      if (t != base_nodeids.end()) continue;

      value = base_covered;
      auto ccid = nics->at(i).cc_id;
      auto find = base_ccids.find(ccid);

      if (find == base_ccids.end()) value += nics->at(i).cc_size;

      fmsr->at(i).measure += std::min(value, cutoff);
    }
  }

  auto max_iterator = std::max_element(
    fmsr->begin(), fmsr->end(),
    [](const NodeMeasure& lhs, const NodeMeasure& rhs) -> bool {
      return lhs.measure < rhs.measure;
    });

  return *max_iterator;
}

void _update_feasibility(
    Bicriteria& bicrit,
    const CompactSampleCollection& csc,
    const double prob) {

  auto mid = (bicrit.feasible_lo + bicrit.feasible_hi) / 2;
  double threshold = prob * mid * csc->size();
  auto acc_msr = 0;
  auto selected = set<LInt>();

  bicrit.seed_set = vector<NodeMeasure>();
  bicrit.seed_set.reserve(bicrit.seed_size);

  while (selected.size() < bicrit.seed_size) {
    NodeMeasure best = _greedy_bicriteria(csc, mid, selected);
    acc_msr = best.measure;
    selected.insert(best.id);
    bicrit.seed_set.emplace_back(NodeMeasure(best.id, best.measure / csc->size()));
  }

  if (acc_msr >= threshold) {
    bicrit.feasible_lo = mid;
  } else {
    bicrit.feasible_hi = mid;
  }

  return;
}

std::unique_ptr<std::vector<datatypes::Bicriteria>> max_prob_bicriteria(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const double& prob,
    const std::unique_ptr<std::vector<datatypes::LInt>>& seed_sizes,
    const int& num_samples,
    const int& num_threads,
    const int& rand_seed) {

  auto num_bicrits = seed_sizes->size();

  auto ret = make_unique<vector<Bicriteria>>();
  ret->reserve(num_bicrits);
  for (size_t i = 0; i < num_bicrits; i++) {
    ret->emplace_back(Bicriteria(seed_sizes->at(i), nodes->size()));
  }

  auto dices = vector<unique_ptr<Dice>>();
  dices.reserve(num_threads);
  for (int i = 0; i < num_threads; i++) {
    dices.push_back(make_unique<STDice>((i+1) * rand_seed));
  }

  auto num_steps = std::llround(std::log(nodes->size()) / std::log(2));

  for (size_t e = 0; e < num_steps; e++) {
    auto csc = _get_samples_collection(graph_edges, nodes, dices, num_samples, num_threads);
    for (size_t i = 0; i < num_bicrits; i++) {
      _update_feasibility(ret->at(i), csc, prob);
    }
  }

  return ret;
}

}
