#pragma once
#include "board.hpp"
#include "random.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <unordered_map>

class RandomAgent {
public:
  size_t take_action(const Board &board, size_t bw) {
    return board.random_legal_move(bw, engine_);
  }

private:
  xorshift engine_{splitmix{}()};
};

class MCTSAgent {
private:
  class Node {
  public:
    constexpr void init_bw(size_t bw) noexcept { bw_ = bw; }
    constexpr Node *get_parent() const noexcept { return parent_; };
    constexpr bool has_children() const noexcept { return children_size_ > 0; }
    template <class PRNG>
    Node *select_child(PRNG &rng, size_t &bw, size_t &pos) {
      float max_score = -1.f;
      for (size_t i = 0; i < children_size_; ++i) {
        auto &child = children_[i];
        const float score = (child.rave_wins_ + child.wins_ +
                             std::sqrt(log_visits_ * child.visits_) * 0.25f) /
                            (child.rave_visits_ + child.visits_);
        child.uct_score_ = score;
        max_score = (score - max_score > 0.0001f) ? score : max_score;
      }
      Board::board_t max_children{};
      for (size_t i = 0; i < children_size_; ++i) {
        if ((children_[i].uct_score_ - max_score) > -0.0001f) {
          max_children.set(i);
        }
      }
      size_t idx = Board::random_move_from_board(max_children, rng);
      auto &child = children_[idx];
      bw = child.bw_;
      pos = child.pos_;
      return &child;
    }
    bool expand(const Board &b) noexcept {
      if (visits_ == 0 || is_leaf_) {
        return false;
      }
      auto moves(b.get_legal_moves(1 - bw_));
      const size_t size = moves.count();
      if (size == 0) {
        is_leaf_ = true;
        return false;
      }
      // expand children
      children_size_ = size;
      children_ = std::make_unique<Node[]>(size);
      for (size_t i = 0, pos = moves._Find_first(); i < size;
           ++i, pos = moves._Find_next(pos)) {
        children_[i].init(1 - bw_, pos, this);
      }
      return true;
    }
    void update(size_t winner,
                const std::array<Board::board_t, 2> &raves) noexcept {
      ++visits_;
      log_visits_ = std::log(visits_);
      wins_ += static_cast<size_t>(winner == bw_);
      // rave
      const size_t csize = children_size_,
                   cwin = static_cast<size_t>(winner == 1 - bw_);
      const auto &rave = raves[1 - bw_];
      for (size_t i = 0; i < csize; ++i) {
        auto &child = children_[i];
        if (rave.BIT_TEST(child.pos_)) {
          ++child.rave_visits_;
          child.rave_wins_ += cwin;
        }
      }
    }
    void get_children_visits(std::unordered_map<size_t, size_t> &visits) const
        noexcept {
      for (size_t i = 0; i < children_size_; ++i) {
        const auto &child = children_[i];
        if (child.visits_ > 0) {
          visits.emplace(child.pos_, child.visits_);
        }
      }
    }

  private:
    inline constexpr void init(size_t bw, size_t pos, Node *parent) noexcept {
      bw_ = bw;
      pos_ = pos;
      parent_ = parent;
    }

  private:
    size_t children_size_ = 0;
    std::unique_ptr<Node[]> children_;
    size_t bw_, pos_ = 81;
    bool is_leaf_ = false;
    Node *parent_ = nullptr;

  private:
    size_t wins_ = 0, visits_ = 0, rave_wins_ = 10, rave_visits_ = 20;
    float log_visits_ = 0.f, uct_score_;
  };

public:
  using hclock = std::chrono::high_resolution_clock;
  const static constexpr auto threshold_time = std::chrono::seconds(1);

  size_t take_action(const Board &b, size_t bw) {
    if (!b.has_legal_move(bw)) {
      return 81;
    }
    size_t total_counts = 0, cbw = 1 - bw, cpos = 81;
    const auto start_time = hclock::now();
    Node root;
    root.init_bw(1 - bw);
    do {
      Node *node = &root;
      Board board(b);
      // selection
      std::array<Board::board_t, 2> rave;
      while (node->has_children()) {
        node = node->select_child(engine_, cbw, cpos);
        board.place(cbw, cpos);
        rave[cbw].set(cpos);
      }
      // expansion
      if (node->expand(board)) {
        node = node->select_child(engine_, cbw, cpos);
        board.place(cbw, cpos);
        rave[cbw].set(cpos);
      }
      // simulation
      const auto init_two_go = board.get_two_go();
      bool is_two_go;
      while (board.has_legal_move(1 - cbw)) {
        cbw = 1 - cbw;
        cpos = board.heuristic_legal_move(cbw, init_two_go, is_two_go, engine_);
        board.place(cbw, cpos);
        if (is_two_go) {
          rave[cbw].set(cpos);
        }
      }
      size_t winner = cbw;
      // backpropogation
      while (node != nullptr) {
        node->update(winner, rave);
        node = node->get_parent();
      }
    } while (++total_counts < 50000 ||
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
  splitmix seed_{};
  xorshift engine_{seed_()};
};