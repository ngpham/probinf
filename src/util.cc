#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include <algorithm>
#include "util.h"

namespace util {

const std::string& ArgParser::empty_str = std::string("");

ArgParser::ArgParser(int argc, char **argv) {
  for (int i = 0; i < argc; i++) tokens.push_back(std::string(argv[i]));
}

const std::string& ArgParser::get_arg(const std::string &arg) const {
  auto ret = tokens.end();

  std::vector<std::string>::const_iterator itr;
  itr = std::find(tokens.begin(), tokens.end(), arg);
  if (itr != tokens.end()) ret = ++itr;

  if (ret != tokens.end()) return *ret;
  return empty_str;
}

STDice::STDice(int seed) : engine(seed), distribution(std::uniform_real_distribution<>(0, 1)) {}

STDice::STDice(int seed, double l, double r) :
  engine(seed), distribution(std::uniform_real_distribution<>(l, r)) {}

MTDice::MTDice(int seed) : engine(seed), distribution(std::uniform_real_distribution<>(0, 1)) {}

MTDice::MTDice(int seed, double l, double r) :
  engine(seed), distribution(std::uniform_real_distribution<>(l, r)) {}

double MTDice::roll() {
  a += 1;
  auto x = distribution(engine);
  a += 1;
  return x;
}

}
