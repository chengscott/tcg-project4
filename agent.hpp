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
    return board.random_legal_move(bw, engine_);
  }

private:
  std::default_random_engine engine_{std::random_device{}()};
};

class MCTSAgent final : public Agent {
private:
  class Node {
    using seed_t = std::random_device::result_type;

  public:
    Node() = default;
    Node(seed_t seed, Board b, size_t bw, size_t pos = 81,
         Node *parent = nullptr)
        : engine_(seed), moves_(b.get_legal_moves(bw)), board_(b), bw_(bw),
          pos_(pos), parent_(parent) {}
    Node(const Node &) = default;
    Node(Node &&) noexcept = default;
    Node &operator=(const Node &) = default;
    Node &operator=(Node &&) noexcept = default;
    ~Node() = default;

  public:
    constexpr Node *get_parent() const noexcept { return parent_; };
    constexpr size_t get_player() const noexcept { return bw_; }
    constexpr const Board &get_board() const noexcept { return board_; }
    bool has_untried_moves() const noexcept { return moves_.any(); }
    std::pair<size_t, size_t> pop_untried_move() noexcept {
      size_t pos = Board::random_move_from_board(moves_, engine_);
      moves_.reset(pos);
      return {1 - bw_, pos};
    }
    bool has_children() const noexcept { return !children_.empty(); }
    Node *get_UCT_child() {
      double max_score = -1;
      for (auto &child : children_) {
        const double score = (child.rave_wins_ + child.wins_ +
                              std::sqrt(log_visits_ * child.visits_) * 0.25) /
                             (child.rave_visits_ + child.visits_);
        child.uct_score_ = score;
        max_score = std::max(score, max_score);
      }
      std::vector<Node *> max_children;
      for (auto &child : children_) {
        if (child.uct_score_ == max_score) {
          max_children.push_back(&child);
        }
      }
      std::uniform_int_distribution<> choose(0, max_children.size() - 1);
      return max_children[choose(engine_)];
    }
    Node *add_child(seed_t seed, size_t bw, size_t pos) noexcept {
      Board board(board_);
      board.place(bw, pos);
      children_.emplace_back(seed, board, bw, pos, this);
      return &children_.back();
    }
    constexpr void update(bool win) noexcept {
      ++visits_;
      log_visits_ = std::log(visits_);
      wins_ += (win ? 1 : 0);
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
    Board::board_t moves_;
    Board board_;
    size_t bw_, pos_;
    Node *parent_;

  private:
    size_t wins_ = 0, visits_ = 0, rave_wins_ = 10, rave_visits_ = 20;
    double log_visits_ = 0., uct_score_;
  };

public:
  size_t take_action(const Board &b, size_t bw) override {
    Node root(seed_(), Board(b), bw);
    using clock_ = std::chrono::high_resolution_clock;
    constexpr const auto threshold_time = std::chrono::seconds(1);
    const auto start_time = clock_::now();
    size_t total_counts = 0;
    do {
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
      size_t winner = rollout(node->get_board(), node->get_player());
      // backpropogation
      while (node != nullptr) {
        node->update(winner == node->get_player());
        node = node->get_parent();
      }
    } while (++total_counts < 10000 ||
             (clock_::now() - start_time) < threshold_time);
    auto visits = root.get_children_visits();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        clock_::now() - start_time);
    std::cerr << std::to_string(duration.count()) << " ms" << std::endl
              << total_counts << " simulations" << std::endl;
    size_t best_move = std::max_element(std::begin(visits), std::end(visits),
                                        [](const auto &p1, const auto &p2) {
                                          return p1.second < p2.second;
                                        })
                           ->first;
    return best_move;
  }

private:
  size_t rollout(Board board, size_t bw) noexcept {
    while (board.has_legal_move(bw)) {
      board.place(bw, board.random_legal_move(bw, engine_));
      bw = 1 - bw;
    }
    return 1 - bw;
  }

private:
  std::random_device seed_{};
  std::default_random_engine engine_{seed_()};
};