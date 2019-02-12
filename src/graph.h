#ifndef GRAPH_H
#define GRAPH_H

#include <memory>
#include "datatypes.h"
#include "util.h"

namespace graph {

std::unique_ptr<datatypes::GraphByEdges> read_edges(
  const std::string &fname, const double &activation, const int &seed);

std::unique_ptr<std::vector<datatypes::Node>> get_graph_nodes(
  const std::unique_ptr<datatypes::GraphByEdges>& graph_edges);

std::unique_ptr<datatypes::Graph> sample_graph(
  const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
  const std::unique_ptr<std::vector<datatypes::Node>>& graph_nodes,
  const std::unique_ptr<util::Dice> &dice);

// return a unique_ptr, which must have the same lifespan with the input Graph.
// update the Graph nodes pointers to their ConnectedComp.
std::unique_ptr<std::vector<std::unique_ptr<datatypes::ConnectedComp>>> connected_component(
  std::unique_ptr<datatypes::Graph>& graph);

// return a compact representation, indexed by Node Id.
// after this function returns, it is safe to destroy the Graph and the ConnectedComp vector.
std::unique_ptr<std::vector<datatypes::NodeIndexedCover>> get_cover(
  std::unique_ptr<datatypes::Graph>& graph);

datatypes::LInt calculate_cover(
  std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
  std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>& covers);

std::unique_ptr<std::vector<datatypes::LInt>> calculate_accumulative_cover(
  std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
  std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>& covers);

std::unique_ptr<std::vector<datatypes::LInt>> calculate_accumulative_average_cover(
  std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
  std::unique_ptr<std::vector<
      std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>>>& covers);

std::unique_ptr<std::vector<datatypes::LInt>> calculate_total_cover_per_sample(
  std::unique_ptr<std::vector<datatypes::LInt>>& nodes,
  std::unique_ptr<std::vector<
      std::unique_ptr<std::vector<datatypes::NodeIndexedCover>>>>& covers);

}

#endif
