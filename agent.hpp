#pragma once
#include "board.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <unordered_map>
#include <utility>
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
    auto moves = board.get_legal_moves(bw);
    std::uniform_int_distribution<size_t> choose(0, moves.size() - 1);
    return moves[choose(engine_)];
  }

private:
  std::default_random_engine engine_{std::random_device{}()};
};

class MCTSAgent final : public Agent {
public:
  size_t take_action(const Board &b, size_t bw) override {
    using clock_ = std::chrono::high_resolution_clock;
    Node root(seed_(), Board(b), bw);
    constexpr const auto threshold_time = std::chrono::seconds(1);
    const auto start_time = clock_::now();
    while ((clock_::now() - start_time) < threshold_time) {
      Node *node = &root;
      // selection
      while (!node->has_untried_moves() && node->has_children()) {
        node = node->get_UCT_child();
      }
      // expansion
      if (node->has_untried_moves()) {
        auto &&[bw, pos] = node->pop_untried_move();
        node = node->add_child(seed_(), bw, pos);
      }
      // simulation
      size_t winner = playout(node->get_board(), node->get_player());
      // backpropogation
      while (node != nullptr) {
        node->update(winner == node->get_player());
        node = node->get_parent();
      }
    }
    auto visits = root.get_children_visits();
    size_t total_visits =
        std::accumulate(std::begin(visits), std::end(visits), 0u,
                        [](auto acc, const auto &p) { return acc + p.second; });
    std::cerr << total_visits << std::endl;
    size_t best_move = std::max_element(std::begin(visits), std::end(visits),
                                        [](const auto &p1, const auto &p2) {
                                          return p1.second < p2.second;
                                        })
                           ->first;
    return best_move;
  }

private:
  size_t playout(Board board, size_t bw) {
    while (true) {
      std::shuffle(std::begin(all_moves_), std::end(all_moves_), engine_);
      auto move =
          std::find_if(std::begin(all_moves_), std::end(all_moves_),
                       [&](size_t move) { return board.place(bw, move); });
      if (move == all_moves_.end()) {
        break;
      }
      bw = 1 - bw;
    };
    return bw;
  }

  using seed_t = std::random_device::result_type;
  class Node {
  public:
    Node() = default;
    Node(seed_t seed, Board &&b, size_t bw, size_t pos = 81,
         Node *parent = nullptr)
        : engine_(seed), moves_(b.get_legal_moves(bw)), board_(b), bw_(bw),
          pos_(pos), parent_(parent) {}
    Node(const Node &) = default;
    Node(Node &&) noexcept = default;
    Node &operator=(const Node &) = default;
    Node &operator=(Node &&) noexcept = default;
    ~Node() = default;

  public:
    constexpr Node *get_parent() const { return parent_; };
    constexpr size_t get_player() const { return bw_; }
    constexpr const Board &get_board() const { return board_; }
    bool has_untried_moves() const { return !moves_.empty(); }
    std::pair<size_t, size_t> pop_untried_move() {
      std::uniform_int_distribution<size_t> choose(0, moves_.size() - 1);
      auto it = std::begin(moves_) + choose(engine_);
      size_t pos = *it;
      moves_.erase(it);
      return {1 - bw_, pos};
    }
    bool has_children() const { return !children_.empty(); }
    Node *get_UCT_child() {
      double log_visits = std::log(visits_);
      for (auto &child : children_) {
        child.uct_score_ =
            child.win_rate_ + std::sqrt(2.0 * log_visits / child.visits_);
      }
      return &*std::max_element(std::begin(children_), std::end(children_),
                                [](const Node &lhs, const Node &rhs) {
                                  return lhs.uct_score_ < rhs.uct_score_;
                                });
    }
    Node *add_child(seed_t seed, size_t bw, size_t pos) {
      Board board(board_);
      board.place(bw, pos);
      Node node(seed, std::move(board), bw, pos, this);
      children_.emplace_back(node);
      return &children_.back();
    }
    constexpr void update(bool win) {
      ++visits_;
      win_rate_ += ((win ? 1 : 0) - win_rate_) / visits_;
    }
    std::unordered_map<size_t, size_t> get_children_visits() const {
      std::unordered_map<size_t, size_t> visits;
      std::transform(std::begin(children_), std::end(children_),
                     std::inserter(visits, std::end(visits)),
                     [](const auto &child) -> std::pair<size_t, size_t> {
                       return {child.pos_, child.visits_};
                     });
      return visits;
    }

  private:
    std::default_random_engine engine_;
    std::vector<Node> children_;
    std::vector<size_t> moves_;
    Board board_;
    size_t bw_, pos_;
    Node *parent_;

  private:
    size_t visits_ = 0;
    double win_rate_ = 0, uct_score_;
  };

private:
  std::array<size_t, 81> all_moves_ = []() constexpr {
    std::array<size_t, 81> ret{};
    for (size_t i = 0; i < 81; ++i) {
      ret[i] = i;
    }
    return ret;
  }
  ();
  std::random_device seed_{};
  std::default_random_engine engine_{seed_()};
};