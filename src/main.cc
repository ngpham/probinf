#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <iterator>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include "datatypes.h"
#include "util.h"
#include "graph.h"
#include "inflalgos.h"

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;
using std::make_unique;
using std::istringstream;
using std::istream_iterator;
using std::ostream_iterator;
using std::chrono::time_point;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using datatypes::LInt;
using datatypes::GraphByEdges;
using datatypes::Vertex;
using datatypes::Edge;
using datatypes::Graph;
using datatypes::Node;
using datatypes::ConnectedComp;
using datatypes::NodeMeasure;
using datatypes::NodeIndexedCover;
using util::Dice;
using util::STDice;

void evaluate_seed_set_by_node_attrb(
    unique_ptr<GraphByEdges>& graph_edges,
    unique_ptr<vector<Node>>& nodes,
    std::string& input2,
    int& num_samples_test,
    int& rand_seed_test) {

  unique_ptr<Dice> dice = make_unique<STDice>(rand_seed_test);
  auto testsets = make_unique<vector<unique_ptr<vector<NodeIndexedCover>>>>();

  for (int i = 0; i < num_samples_test; i++) {
    auto g = graph::sample_graph(graph_edges, nodes, dice);
    auto cc = graph::connected_component(g);
    auto nc = graph::get_cover(g);
    testsets->emplace_back(std::move(nc));
  }

  auto seed_set_str = make_unique<vector<string>>();

  auto fin = std::ifstream(input2);
  std::string line;
  while (getline(fin, line)) {
    if (line[0] == '#' || line.empty()) continue;

    istringstream iss(line);
    std::copy(
      istream_iterator<string>(iss),
      istream_iterator<string>(),
      std::back_inserter(*seed_set_str));
  }
  fin.close();

  auto seed_set_attrb = make_unique<vector<LInt>>();
  std::transform(seed_set_str->begin(), seed_set_str->end(), std::back_inserter(*seed_set_attrb),
    [](string& s) -> LInt { return std::stoi(s); }
  );

  auto seed_set = make_unique<vector<LInt>>();
  for (auto& x: *seed_set_attrb) {
    auto find = std::find_if(
      nodes->begin(), nodes->end(),
      [&x](Node& u) { return u.attr == x; } );
    seed_set->emplace_back((*find).id);
  }

  auto m = graph::calculate_total_cover_per_sample(seed_set, testsets);

  std::ofstream file;
  file.open(input2 + ".out", std::ofstream::out | std::ofstream::app);

  auto cout_buff = std::cout.rdbuf();
  std::cout.rdbuf(file.rdbuf());

  for (auto& x: *m) {
    cout << x << ", ";
  }
  cout << endl;

  file.close();
  std::cout.rdbuf(cout_buff);

  return;
}

int main(int argc, char** argv) {
  auto ap = util::ArgParser(argc, argv);

  auto input = ap.get_arg("-f");
  int seed_size(0);
  double activation(0);
  double prob(0);
  int rand_seed(0);
  int rand_seed_input(0);
  int rand_seed_test(0);
  std::string algorithm("maxprobbicritinfl");
  std::string input2("");
  int num_samples(0);
  int num_samples_test(0);
  int num_threads(1);

  try {
    seed_size = std::stoi(ap.get_arg("-k"));
    activation = std::stod(ap.get_arg("-a"));
    prob = std::stod(ap.get_arg("-p"));
    rand_seed = std::stoi(ap.get_arg("-rs"));
    rand_seed_input = std::stoi(ap.get_arg("-rsin"));
    rand_seed_test = std::stoi(ap.get_arg("-rstest"));
    algorithm = ap.get_arg("-alg");
    num_samples = std::stoi(ap.get_arg("-numsamp"));
    num_samples_test = std::stoi(ap.get_arg("-numsamptest"));
    num_threads = omp_get_max_threads();
    input2 = ap.get_arg("-ff");
  } catch (...) {
    cout << "Warning: Some arguments are missing." << endl;
  }

  auto batch_size = num_samples / num_threads;
  if (num_samples > batch_size * num_threads)
    num_samples = (batch_size + 1) * num_threads;

  unique_ptr<GraphByEdges> graph_edges = graph::read_edges(input, activation, rand_seed_input);
  unique_ptr<vector<Node>> nodes = graph::get_graph_nodes(graph_edges);

  auto result = make_unique<vector<NodeMeasure>>();
  auto seed_set = make_unique<vector<LInt>>();
  auto measure = make_unique<vector<LInt>>();

  auto start = high_resolution_clock::now();

  if (algorithm.compare("maxexpinfl") == 0) {
    result = inflalgos::max_exp_infl(
      graph_edges, nodes, seed_size, num_samples, num_threads, rand_seed);
  } else if (algorithm.compare("maxprobinfl") == 0) {
    result = inflalgos::max_prob_infl(
      graph_edges, nodes, prob, seed_size, num_samples, num_threads, rand_seed);
  } else if (algorithm.compare("maxprobbicritinfl") == 0) {
    auto seed_sizes = make_unique<vector<LInt>>();
    seed_sizes->emplace_back(seed_size);

    auto bc = inflalgos::max_prob_bicriteria(
      graph_edges, nodes, prob, seed_sizes, num_samples, num_threads, rand_seed);

    result->reserve(seed_size);
    for (auto& u: bc->at(0).seed_set) {
      result->emplace_back(u);
    }
  } else if (algorithm.compare("evaluate") == 0) {
    evaluate_seed_set_by_node_attrb(graph_edges, nodes, input2, num_samples_test, rand_seed_test);
    return 0;
  }

  auto stop = high_resolution_clock::now();
  auto exec_time = duration_cast<std::chrono::seconds>(stop - start);

  for (auto& id_val: *result) {
    seed_set->push_back(id_val.id);
  }

  unique_ptr<Dice> dice = make_unique<STDice>(rand_seed_test);
  auto testsets = make_unique<vector<unique_ptr<vector<NodeIndexedCover>>>>();

  for (int i = 0; i < num_samples_test; i++) {
    auto s = graph::sample_graph(graph_edges, nodes, dice);
    auto cc = graph::connected_component(s);
    auto nc = graph::get_cover(s);
    testsets->emplace_back(std::move(nc));
  }

  measure = graph::calculate_accumulative_average_cover(seed_set, testsets);

  std::ofstream file;
  file.open("output.txt", std::ofstream::out | std::ofstream::app);

  auto cout_buff = std::cout.rdbuf();
  std::cout.rdbuf(file.rdbuf());

  cout << "[" << input << ", k=" << seed_size << ", activation=" << activation
    << ", delta=" << prob << ", samples=" << num_samples << ", algorithm=" << algorithm
    << ", random_seed=" << rand_seed << ", random_seed_input=" << rand_seed_input
    << ", random_seed_test=" << rand_seed_test << ", samples_test=" << num_samples_test
    << "]" << endl;
  cout << "time in secs: " << exec_time.count() << endl;

  for (size_t i = 0; i < seed_set->size(); i++) {
    auto& u = seed_set->at(i);
    auto& attr = nodes->at(u).attr;
    auto& msr_found = result->at(i).measure;
    auto& msr_test = measure->at(i);

    cout << std::left <<
      std::setw(30) << "Node(" + std::to_string(u) + ", " + std::to_string(attr) + ")" <<
      std::setw(30) << msr_found <<
      std::setw(30) << msr_test << endl;
  }

  cout << "---------------" << std::endl;

  file.close();
  std::cout.rdbuf(cout_buff);
  std::cout << "Done!" << std::endl;

  return 0;
}
