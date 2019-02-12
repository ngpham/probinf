#include "datatypes.h"

namespace datatypes {

Graph::Graph(LInt nums) : nodes(std::make_unique<std::vector<Node>>()) {
  nodes->reserve(nums);
}

Graph::Graph(const std::unique_ptr<std::vector<Node>>& from_nodes) :
  nodes(std::make_unique<std::vector<Node>>(from_nodes->begin(), from_nodes->end())) {}

void ConnectedComp::add(const Node& node) {
  if (node.id < id) id = node.id;
  size += 1;
}

}
