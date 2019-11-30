#pragma once
#include "dset.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

struct Position {
  Position() = default;
  Position(const Position &) = default;
  Position(Position &&) noexcept = default;
  explicit Position(size_t p) : p0(p % 9), p1(p / 9) {}
  explicit operator size_t() const { return p0 + p1 * 9; }
  Position &operator=(const Position &) = default;
  Position &operator=(Position &&) noexcept = default;
  ~Position() = default;

  friend std::istream &operator>>(std::istream &in, Position &p) {
    std::string ipos;
    in >> ipos;
    // assert(ipos.size() == 2);
    size_t p0 = tolower(ipos[0]) - 'a';
    p.p0 = p0 > 8 ? p0 - 1 : p0;
    p.p1 = 8 - (ipos[1] - '1');
    return in;
  }
  friend std::ostream &operator<<(std::ostream &out, const Position &p) {
    out << char((p.p0 >= 8 ? 1 : 0) + p.p0 + 'A') << char((8 - p.p1) + '1');
    return out;
  }

  size_t p0, p1;
};

class Board {
public:
  size_t operator[](size_t p) const {
    return ((board_[0] >> p) & 1u) + (((board_[1] >> p) & 1u) << 1u);
  }
  bool place(size_t bw, size_t p) {
    // assert(bw == 0 || bw == 1);
    if (operator[](p) != 0) {
      return false;
    }
    auto b = Board(*this);
    b.board_[bw] |= ((__uint128_t)1) << p;
    auto &dset = b.dset_[bw];
    const auto &dir_begin = std::begin(dir_[p]),
               &dir_end = dir_begin + dir_len_[p];
    std::for_each(dir_begin, dir_end, [&](auto i) {
      if (b[i] == bw + 1) {
        dset.unions(p, i);
      }
    });
    if (!has_liberty(b, bw, p)) {
      return false;
    }
    if (std::any_of(dir_begin, dir_end, [&](auto i) {
          return (b[i] == 2 - bw) && !has_liberty(b, 1 - bw, i);
        })) {
      return false;
    }
    std::swap(*this, b);
    return true;
  }

  std::vector<size_t> get_legal_moves(size_t bw) const {
    std::vector<size_t> moves;
    for (size_t i = 0; i < 81; ++i) {
      Board b(*this);
      if (b.place(bw, i)) {
        moves.push_back(i);
      }
    }
    return moves;
  }

public:
  friend std::ostream &operator<<(std::ostream &out, const Board &b) {
    out << "  A B C D E F G H J\n";
    for (size_t i = 0; i < 9; ++i) {
      out << 9 - i << ' ';
      for (size_t j = 0; j < 9; ++j) {
        out << ".XO"[b[i * 9 + j]] << ' ';
      }
      out << 9 - i << '\n';
    }
    out << "  A B C D E F G H J\n\n";
    return out;
  }

private:
  static bool has_liberty(Board &b, size_t bw, size_t p) {
    // assert(bw == 0 || bw == 1);
    auto &dset = b.dset_[bw];
    const size_t x = dset.find(p);
    for (size_t i = 0; i < 81; ++i) {
      if (x == dset.find(i)) {
        const auto &dir_begin = std::begin(dir_[i]),
                   &dir_end = dir_begin + dir_len_[i];
        if (std::any_of(dir_begin, dir_end,
                        [&b](auto i) { return b[i] == 0; })) {
          return true;
        }
      }
    }
    return false;
  }

private:
  __uint128_t board_[2] = {};
  DSet<81> dset_[2];
  const static constexpr auto dir_ = []() constexpr {
    std::array<std::array<size_t, 4>, 81> ret{};
    for (size_t i = 0; i < 81; ++i) {
      auto &ri = ret[i];
      size_t j = 0;
      if (i / 9 > 0) {
        ri[j++] = i - 9;
      }
      if (i % 9 > 0) {
        ri[j++] = i - 1;
      }
      if (i % 9 < 8) {
        ri[j++] = i + 1;
      }
      if (i / 9 < 8) {
        ri[j++] = i + 9;
      }
    }
    return ret;
  }
  ();
  const static constexpr auto dir_len_ = []() constexpr {
    std::array<size_t, 81> ret{};
    for (size_t i = 0; i < 81; ++i) {
      ret[i] = (i / 9 > 0) + (i % 9 > 0) + (i % 9 < 8) + (i / 9 < 8);
    }
    return ret;
  }
  ();
};