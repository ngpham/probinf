#ifndef UTIL_H
#define UTIL_H

#include <atomic>
#include <random>

namespace util {

struct ArgParser {
  std::vector<std::string> tokens;
  static const std::string &empty_str;

  ArgParser(int argc, char **argv);

  const std::string& get_arg(const std::string &arg) const;
};

class Dice {
public:
  virtual double roll() = 0;
};

class STDice : public Dice {
public:
  STDice(int seed);
  STDice(int seed, double l, double r);

  inline double roll() { return distribution(engine); }
private:
  std::mt19937 engine;
  std::uniform_real_distribution<> distribution;
};

class MTDice : public Dice {
public:
  MTDice(int seed);
  MTDice(int seed, double l, double r);

  double roll();
private:
  std::mt19937 engine;
  std::uniform_real_distribution<> distribution;
  std::atomic<int> a {0};
};

}

#endif
