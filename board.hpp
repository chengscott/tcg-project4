#pragma once
#include "dset.hpp"
#include <algorithm>
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
    const auto dlen = dir_len_[p];
    const auto &dirp = dir_[p];
    std::for_each(dirp, dirp + dlen, [&](auto i) {
      if (b[i] == bw + 1) {
        dset.unions(p, i);
      }
    });
    if (!has_liberty(b, bw, p)) {
      return false;
    }
    if (std::any_of(dirp, dirp + dlen, [&](auto i) {
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
        if (std::any_of(dir_[i], dir_[i] + dir_len_[i],
                        [&b](auto i) { return b[i] == 0; })) {
          return true;
        }
      }
    }
    return false;
  }

private:
  __uint128_t board_[2] = {};
  DSet dset_[2];
  constexpr const static size_t dir_len_[81] = {
      2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4,
      4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4,
      4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3,
      3, 4, 4, 4, 4, 4, 4, 4, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2};
  constexpr const static size_t dir_[81][4] = {
      {1, 9, 81, 81},   {0, 2, 10, 81},   {1, 3, 11, 81},   {2, 4, 12, 81},
      {3, 5, 13, 81},   {4, 6, 14, 81},   {5, 7, 15, 81},   {6, 8, 16, 81},
      {7, 17, 81, 81},  {0, 10, 18, 81},  {1, 9, 11, 19},   {2, 10, 12, 20},
      {3, 11, 13, 21},  {4, 12, 14, 22},  {5, 13, 15, 23},  {6, 14, 16, 24},
      {7, 15, 17, 25},  {8, 16, 26, 81},  {9, 19, 27, 81},  {10, 18, 20, 28},
      {11, 19, 21, 29}, {12, 20, 22, 30}, {13, 21, 23, 31}, {14, 22, 24, 32},
      {15, 23, 25, 33}, {16, 24, 26, 34}, {17, 25, 35, 81}, {18, 28, 36, 81},
      {19, 27, 29, 37}, {20, 28, 30, 38}, {21, 29, 31, 39}, {22, 30, 32, 40},
      {23, 31, 33, 41}, {24, 32, 34, 42}, {25, 33, 35, 43}, {26, 34, 44, 81},
      {27, 37, 45, 81}, {28, 36, 38, 46}, {29, 37, 39, 47}, {30, 38, 40, 48},
      {31, 39, 41, 49}, {32, 40, 42, 50}, {33, 41, 43, 51}, {34, 42, 44, 52},
      {35, 43, 53, 81}, {36, 46, 54, 81}, {37, 45, 47, 55}, {38, 46, 48, 56},
      {39, 47, 49, 57}, {40, 48, 50, 58}, {41, 49, 51, 59}, {42, 50, 52, 60},
      {43, 51, 53, 61}, {44, 52, 62, 81}, {45, 55, 63, 81}, {46, 54, 56, 64},
      {47, 55, 57, 65}, {48, 56, 58, 66}, {49, 57, 59, 67}, {50, 58, 60, 68},
      {51, 59, 61, 69}, {52, 60, 62, 70}, {53, 61, 71, 81}, {54, 64, 72, 81},
      {55, 63, 65, 73}, {56, 64, 66, 74}, {57, 65, 67, 75}, {58, 66, 68, 76},
      {59, 67, 69, 77}, {60, 68, 70, 78}, {61, 69, 71, 79}, {62, 70, 80, 81},
      {63, 73, 81, 81}, {64, 72, 74, 81}, {65, 73, 75, 81}, {66, 74, 76, 81},
      {67, 75, 77, 81}, {68, 76, 78, 81}, {69, 77, 79, 81}, {70, 78, 80, 81},
      {71, 79, 81, 81}};
};