#include <memory>
#include <vector>
#include <set>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <functional>
#include <fstream>
#include <string>
#include <sstream>
#include "datatypes.h"
#include "util.h"
#include "graph.h"

namespace graph {

using std::make_unique;
using std::unique_ptr;
using std::make_tuple;
using std::vector;
using std::set;
using datatypes::LInt;
using datatypes::GraphByEdges;
using datatypes::Vertex;
using datatypes::Edge;
using datatypes::Graph;
using datatypes::Node;
using datatypes::ConnectedComp;
using datatypes::NodeIndexedCover;
using util::Dice;
using util::STDice;

std::unique_ptr<datatypes::GraphByEdges> read_edges(
    const std::string &fname, const double &activation, const int &seed) {

  struct VertexCompare {
    bool operator() (const datatypes::Vertex &lhs, const datatypes::Vertex &rhs) const {
      return std::get<1>(lhs) < std::get<1>(rhs);
    }
  };

  auto pV = make_unique<set<Vertex, VertexCompare>>();
  auto pE = make_unique<set<Edge>>();

  auto fin = std::ifstream(fname);
  std::string line;
  auto dice = STDice(seed, 0.001, 0.05);

  LInt u, v;
  LInt counter = 0;

  while (getline(fin, line)) {
    if (line[0] == '#' || line.empty()) continue;

    std::istringstream(line) >> u >> v;

    auto tu = make_tuple(counter, u);
    counter++;
    auto find_tu = pV->find(tu);
    if (pV->find(tu) == pV->end()) {
      pV->insert(tu);
    } else {
      tu = *find_tu;
      counter--;
    }

    auto tv = make_tuple(counter, v);
    counter++;
    auto find_tv = pV->find(tv);
    if (pV->find(tv) == pV->end()) {
      pV->insert(tv);
    } else {
      tv = *find_tv;
      counter--;
    }

    if (activation > 0) {
      pE->insert(make_tuple(std::get<0>(tu), std::get<0>(tv), activation));
    } else {
      pE->insert(make_tuple(std::get<0>(tu), std::get<0>(tv), dice.roll()));
    }
  }

  fin.close();

  auto vV = make_unique<vector<Vertex>>(pV->size());
  std::copy(pV->begin(), pV->end(), vV->begin());

  auto vE = make_unique<vector<Edge>>(pE->begin(), pE->end());

  return make_unique<GraphByEdges>(std::move(vV), std::move(vE));
}

std::unique_ptr<std::vector<datatypes::Node>> get_graph_nodes(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges) {

  auto vertices = *(graph_edges->vertexes);
  auto nodes = make_unique<vector<Node>>();
  nodes->resize(vertices.size());

  for (auto x : vertices) {
    auto i = std::get<0>(x);
    nodes->at(i) = Node(i, std::get<1>(x));
  }
  return nodes;
}

std::unique_ptr<datatypes::Graph> sample_graph(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_by_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const std::unique_ptr<util::Dice> &dice) {

  auto graph = make_unique<Graph>(nodes);
  auto& sg_nodes = graph->nodes;

  for (auto e: *(graph_by_edges->edges)) {
    LInt u, v;
    double a;
    std::tie(u, v, a) = e;
    if (dice->roll() < a) {
      sg_nodes->at(u).neighbors.push_back(&(sg_nodes->at(v)));
      sg_nodes->at(v).neighbors.push_back(&(sg_nodes->at(u)));
    }
  }

  return graph;
}

std::unique_ptr<std::vector<std::unique_ptr<datatypes::ConnectedComp>>> connected_component(
    std::unique_ptr<datatypes::Graph>& graph) {

  auto ccs = make_unique<vector<unique_ptr<ConnectedComp>>>();

  std::for_each(graph->nodes->begin(), graph->nodes->end(), [&ccs](Node& u){
    if (u.ccomp == nullptr) {
      // auto& cc = ccs->emplace_back(make_unique<ConnectedComp>(u));
      // u.ccomp = cc.get();
      // code below for old g++ 5.2 on UH cluster...
      std::unique_ptr<ConnectedComp> cc (new ConnectedComp(u));
      u.ccomp = cc.get();
      ccs->emplace_back(std::move(cc));
    }
    for (auto v: u.neighbors) {
      if (v->ccomp == nullptr) {
        u.ccomp->add(*v);
        v->ccomp = u.ccomp;
      }
    }
  });

  return ccs;
}

std::unique_ptr<std::vector<datatypes::NodeIndexedCover>> get_cover(
    std::unique_ptr<datatypes::Graph>& graph) {

  auto niis = make_unique<vector<NodeIndexedCover>>();
  niis->resize(graph->nodes->size());

  for (auto& u: *(graph->nodes)) {
    niis->at(u.id).cc_id = u.ccomp->id;
    niis->at(u.id).cc_size = u.ccomp->size;
  }

  return niis;
}

LInt calculate_cover(
    std::unique_ptr<std::vector<LInt>>& nodes,
    std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>& covers) {

  auto covered = set<LInt>();
  LInt sum = 0;

  for (auto& id: *nodes) {
    auto iter_bool = covered.insert(covers->at(id).cc_id);
    if (iter_bool.second == true) sum += covers->at(id).cc_size;
  }

  return sum;
}

std::unique_ptr<std::vector<datatypes::LInt>> calculate_accumulative_cover(
    std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
    std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>& covers) {

  auto ret = make_unique<vector<LInt>>();
  ret->resize(nodes->size());
  auto cover = set<LInt>();
  LInt sum = 0;

  for (size_t i = 0; i < nodes->size(); i++) {
    auto& id = nodes->at(i);
    auto iter_bool = cover.insert(covers->at(id).cc_id);
    if (iter_bool.second == true) sum += covers->at(id).cc_size;

    ret->at(i) = sum;
  }

  return ret;
}

std::unique_ptr<std::vector<datatypes::LInt>> calculate_accumulative_average_cover(
    std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
    std::unique_ptr<std::vector<
      std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>>>& covers) {

  auto ret = make_unique<vector<LInt>>(nodes->size(), 0);

  for (auto& x: *covers) {
    auto a = calculate_accumulative_cover(nodes, x);
    std::transform(
      a->begin(), a->end(), ret->begin(), ret->begin(),
      std::plus<LInt>());
  }

  auto num = covers->size();
  for (auto& x: *ret) { x = x / num; }

  return ret;
}

std::unique_ptr<std::vector<datatypes::LInt>> calculate_total_cover_per_sample(
    std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
    std::unique_ptr<std::vector<
      std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>>>& covers) {

  auto ret = make_unique<vector<LInt>>();

  for (auto& c: *covers) {
    auto influence = calculate_cover(nodes, c);
    ret->emplace_back(influence);
  }

  return ret;
}

}
