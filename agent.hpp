#pragma once
#include "board.hpp"
#include <algorithm>
#include <random>
#include <vector>

class Agent {
public:
  virtual size_t take_action(const Board &, size_t) = 0;
};

class RandomAgent : public Agent {
public:
  virtual size_t take_action(const Board &board, size_t bw) {
    std::vector<size_t> moves;
    for (size_t i = 0; i < 81; ++i) {
      Board b(board);
      if (b.place(bw, i))
        moves.push_back(i);
    }
    std::shuffle(std::begin(moves), std::end(moves), engine_);
    return moves[0];
  }

private:
  std::default_random_engine engine_;
};