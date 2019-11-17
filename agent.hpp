#pragma once
#include "board.hpp"
#include <algorithm>
#include <random>
#include <vector>

class Agent {
public:
  Agent() = default;
  Agent(const Agent &) = default;
  Agent(Agent &&) noexcept = default;
  Agent &operator=(const Agent &) = default;
  Agent &operator=(Agent &&) noexcept = default;
  virtual ~Agent() = default;

  virtual size_t take_action(const Board &, size_t) = 0;
};

class RandomAgent final : public Agent {
public:
  size_t take_action(const Board &board, size_t bw) override {
    std::vector<size_t> moves;
    for (size_t i = 0; i < 81; ++i) {
      Board b(board);
      if (b.place(bw, i)) {
        moves.push_back(i);
      }
    }
    std::shuffle(std::begin(moves), std::end(moves), engine_);
    return moves[0];
  }

private:
  std::default_random_engine engine_;
};