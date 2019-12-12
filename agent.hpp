#pragma once
#include "board.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
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
  public:
    Node() = default;
    Node(Board::board_t moves, size_t bw, size_t pos = 81,
         Node *parent = nullptr)
        : moves_(std::move(moves)), bw_(bw), pos_(pos), parent_(parent) {}
    Node(const Node &) = default;
    Node(Node &&) noexcept = default;
    Node &operator=(const Node &) = default;
    Node &operator=(Node &&) noexcept = default;
    ~Node() = default;

  public:
    constexpr Node *get_parent() const noexcept { return parent_; };
    constexpr size_t get_player() const noexcept { return bw_; }
    constexpr void get_move(size_t &bw, size_t &pos) const noexcept {
      bw = bw_;
      pos = pos_;
    }
    bool has_untried_moves() const noexcept { return moves_.any(); }
    template <class PRNG>
    void pop_untried_move(PRNG &rng, size_t &bw, size_t &pos) noexcept {
      bw = 1 - bw_;
      pos = Board::random_move_from_board(moves_, rng);
      moves_.reset(pos);
    }
    bool has_children() const noexcept { return !children_.empty(); }
    template <class PRNG> Node *get_UCT_child(PRNG &rng) {
      float max_score = -1;
      for (auto &child : children_) {
        const float score = (child.rave_wins_ + child.wins_ +
                             std::sqrt(log_visits_ * child.visits_) * 0.25f) /
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
      std::uniform_int_distribution<size_t> choose(0, max_children.size() - 1);
      return max_children[choose(rng)];
    }
    Node *add_child(Board::board_t moves, size_t bw, size_t pos) noexcept {
      children_.emplace_back(std::move(moves), bw, pos, this);
      return &children_.back();
    }
    constexpr void update(bool win) noexcept {
      ++visits_;
      log_visits_ = std::log(visits_);
      wins_ += (win ? 1 : 0);
    }
    void get_children_visits(std::unordered_map<size_t, size_t> &visits) const
        noexcept {
      std::transform(std::begin(children_), std::end(children_),
                     std::inserter(visits, std::end(visits)),
                     [](const auto &child) -> std::pair<size_t, size_t> {
                       return {child.pos_, child.visits_};
                     });
    }

  private:
    std::vector<Node> children_;
    Board::board_t moves_;
    size_t bw_, pos_;
    Node *parent_;

  private:
    size_t wins_ = 0, visits_ = 0, rave_wins_ = 10, rave_visits_ = 20;
    float log_visits_ = 0.f, uct_score_;
  };

public:
  using hclock = std::chrono::high_resolution_clock;
  const static constexpr auto threshold_time = std::chrono::seconds(1);

  size_t take_action(const Board &b, size_t bw) override {
    if (!b.has_legal_move(bw)) {
      return 81;
    }
    size_t total_counts = 0, cbw, cpos;
    const auto start_time = hclock::now();
    Node root(b.get_legal_moves(bw), bw);
    do {
      Node *node = &root;
      Board board(b);
      // selection
      while (!node->has_untried_moves() && node->has_children()) {
        node = node->get_UCT_child(engine_);
        node->get_move(cbw, cpos);
        board.place(cbw, cpos);
      }
      // expansion
      if (node->has_untried_moves()) {
        node->pop_untried_move(engine_, cbw, cpos);
        board.place(cbw, cpos);
        node = node->add_child(board.get_legal_moves(cbw), cbw, cpos);
      }
      // simulation
      size_t winner = rollout(engine_, board, node->get_player());
      // backpropogation
      while (node != nullptr) {
        node->update(winner == node->get_player());
        node = node->get_parent();
      }
    } while (++total_counts < 10000 ||
             (hclock::now() - start_time) < threshold_time);
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              hclock::now() - start_time)
                              .count();
    std::cerr << duration << " ms" << std::endl
              << total_counts << " simulations" << std::endl;

    std::unordered_map<size_t, size_t> visits;
    root.get_children_visits(visits);
    size_t best_move = std::max_element(std::begin(visits), std::end(visits),
                                        [](const auto &p1, const auto &p2) {
                                          return p1.second < p2.second;
                                        })
                           ->first;
    return best_move;
  }

private:
  template <class PRNG>
  static size_t rollout(PRNG &rng, Board board, size_t bw) noexcept {
    while (board.has_legal_move(bw)) {
      board.place(bw, board.random_legal_move(bw, rng));
      bw = 1 - bw;
    }
    return 1 - bw;
  }

private:
  std::random_device seed_{};
  std::default_random_engine engine_{seed_()};
};