#ifndef DATATYPES_H
#define DATATYPES_H

#include <tuple>
#include <memory>
#include <vector>


namespace datatypes {

using LInt = long long int;

using Vertex = std::tuple<LInt, LInt>;

using Edge = std::tuple<LInt, LInt, double>;

struct GraphByEdges {
  std::unique_ptr<std::vector<Vertex>> vertexes;
  std::unique_ptr<std::vector<Edge>> edges;

  GraphByEdges(
    std::unique_ptr<std::vector<Vertex>> &&pV,
    std::unique_ptr<std::vector<Edge>> &&pE)
  : vertexes(std::move(pV)), edges(std::move(pE)) {}

  GraphByEdges (const GraphByEdges&) = delete;
  GraphByEdges& operator= (const GraphByEdges&) = delete;
};

struct ConnectedComp;

struct Node {
  LInt id;
  LInt attr;
  ConnectedComp* ccomp;
  std::vector<Node*> neighbors;

  Node() : id(-1), attr(-1), ccomp(nullptr) {}
  Node(LInt id, LInt attr) : id(id), attr(attr), ccomp(nullptr) {}
};

struct ConnectedComp {
  LInt id;
  LInt size;

  ConnectedComp() : id(-1), size(0) {}

  ConnectedComp(const Node &node) : id(node.id), size(1) {}

  void add(const Node& node);
};

struct Graph {
  std::unique_ptr<std::vector<Node>> nodes;

  Graph(LInt nums);
  Graph(const std::unique_ptr<std::vector<Node>>& from_nodes);

  Graph (const Graph&) = delete;
  Graph& operator= (const Graph&) = delete;
};

struct NodeIndexedCover {
  LInt cc_id;
  LInt cc_size;
};

struct NodeMeasure {
  LInt id;
  LInt measure;

  NodeMeasure() : id(0), measure(0) {}
  NodeMeasure(LInt id, LInt msr) : id(id), measure(msr) {}
};

struct Bicriteria {
  LInt seed_size;
  LInt feasible_lo;
  LInt feasible_hi;
  std::vector<NodeMeasure> seed_set;

  Bicriteria(LInt k, LInt n) :
    seed_size(k), feasible_lo(1), feasible_hi(n), seed_set(std::vector<NodeMeasure>(k)) {}
};

}

#endif
