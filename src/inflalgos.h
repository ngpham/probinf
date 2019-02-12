#ifndef INFLALGOS_H
#define INFLALGOS_H

#include <memory>
#include <vector>
#include "datatypes.h"
#include "util.h"

namespace inflalgos {

  std::unique_ptr<std::vector<datatypes::NodeMeasure>> max_exp_infl(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const int& seed_size,
    const int& num_samples,
    const int& num_threads,
    const int& rand_seed);

  std::unique_ptr<std::vector<datatypes::NodeMeasure>> max_prob_infl(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const double& prob,
    const int& seed_size,
    const int& num_samples,
    const int& num_threads,
    const int& rand_seed);

  std::unique_ptr<std::vector<datatypes::Bicriteria>> max_prob_bicriteria(
    const std::unique_ptr<datatypes::GraphByEdges>& graph_edges,
    const std::unique_ptr<std::vector<datatypes::Node>>& nodes,
    const double& prob,
    const std::unique_ptr<std::vector<datatypes::LInt>>& seed_sizes,
    const int& num_samples,
    const int& num_threads,
    const int& rand_seed);
}

#endif
