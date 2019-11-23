#pragma once
#include "board.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <random>
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
  std::mt19937 engine_{std::random_device{}()};
};

class MCTSAgent final : public Agent {
public:
  size_t take_action(const Board &b, size_t bw) override {
    Board board(b);
    Node root(seed_(), board, bw);
    constexpr const double threshold_time = 1.;
    const auto start_time = std::chrono::high_resolution_clock::now();
    double dt;
    do {
      Node *node = &root;
      // selection
      while (!node->has_untried_moves() && node->has_children()) {
        node = node->get_UCT_child();
        auto &&[bw, pos] = node->get_move();
        board.place(bw, pos);
      }
      // expansion
      if (node->has_untried_moves()) {
        auto &&[bw, pos] = node->pop_untried_move();
        board.place(bw, pos);
        node = node->add_child(seed_(), board, bw, pos);
      }
      // simulation
      size_t winner = playout(board, node->get_player());
      // backpropogation
      while (node != nullptr) {
        node->update(winner == node->get_player());
        node = node->get_parent();
      }
      // time threshold
      dt = 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now() - start_time)
                      .count();
    } while (dt < threshold_time);
    auto &children = root.get_children();
    size_t total_visits = 0;
    std::map<size_t, size_t> visits;
    for (const auto &child : children) {
      auto &&[move, visit] = child.get_move_visits();
      visits.emplace(move, visit);
      total_visits += visit;
      // std::cerr << Position(move) << ' ' << visit << std::endl;
    }
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
      auto moves = board.get_legal_moves(bw);
      if (moves.empty()) {
        break;
      }
      std::uniform_int_distribution<size_t> choose(0, moves.size() - 1);
      auto it = moves.begin() + choose(engine_);
      size_t move = *it;
      moves.erase(it);
      board.place(bw, move);
      bw = 1 - bw;
    };
    return bw;
  }

  using seed_t = std::random_device::result_type;
  class Node {
  public:
    Node() = default;
    Node(seed_t seed, const Board &board, size_t bw, size_t pos = 81,
         Node *parent = nullptr)
        : engine_(seed), moves_(board.get_legal_moves(bw)), bw_(bw), pos_(pos),
          parent_(parent) {
      children_.reserve(81);
    }
    Node(const Node &) = default;
    Node(Node &&) noexcept = default;
    Node &operator=(const Node &) = default;
    Node &operator=(Node &&) noexcept = default;
    ~Node() = default;

  public:
    constexpr Node *get_parent() const { return parent_; };
    constexpr size_t get_player() const { return bw_; }
    constexpr std::pair<size_t, size_t> get_move() const { return {bw_, pos_}; }
    bool has_untried_moves() const { return !moves_.empty(); }
    std::pair<size_t, size_t> pop_untried_move() {
      std::uniform_int_distribution<size_t> choose(0, moves_.size() - 1);
      auto it = moves_.begin() + choose(engine_);
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
      return &*std::max_element(children_.begin(), children_.end(),
                                [](const Node &lhs, const Node &rhs) {
                                  return lhs.uct_score_ < rhs.uct_score_;
                                });
    }
    Node *add_child(seed_t seed, const Board &board, size_t bw, size_t pos) {
      Node node(seed, board, bw, pos, this);
      children_.emplace_back(node);
      return &children_.back();
    }
    constexpr void update(bool win) {
      ++visits_;
      win_rate_ += ((win ? 1 : 0) - win_rate_) / visits_;
    }
    constexpr const std::vector<Node> &get_children() const {
      return children_;
    }
    constexpr std::pair<size_t, size_t> get_move_visits() const {
      return {pos_, visits_};
    }

  private:
    std::mt19937 engine_;
    std::vector<Node> children_;
    std::vector<size_t> moves_;
    size_t bw_, pos_;
    Node *parent_;

  private:
    size_t visits_ = 0;
    double win_rate_ = 0, uct_score_;
  };

private:
  std::random_device seed_{};
  std::mt19937 engine_{seed_()};
};