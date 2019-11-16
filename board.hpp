#pragma once
#include "dset.hpp"
#include <iostream>

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
    Board b = Board(*this);
    b.board_[bw] |= ((__uint128_t)1) << p;
    auto &dset = b.dset_[bw];
    dset.dset[p + 1] = p + 1;
    dset.dsize[0] -= 1;
    dset.dsize[p + 1] = 1;
    if (p % 9 > 0)
      dset.unions(p + 1, p - 1 + 1);
    if (p / 9 > 0)
      dset.unions(p + 1, p - 9 + 1);
    if (p / 9 < 8)
      dset.unions(p + 1, p + 9 + 1);
    if (p % 9 < 8)
      dset.unions(p + 1, p + 1 + 1);
    if (!has_liberty(b, bw, p) ||
        (p % 9 > 0 && !has_liberty(b, 1 - bw, p - 1)) ||
        (p / 9 > 0 && !has_liberty(b, 1 - bw, p - 9)) ||
        (p / 9 < 8 && !has_liberty(b, 1 - bw, p + 9)) ||
        (p % 9 < 8 && !has_liberty(b, 1 - bw, p + 1))) {
      return false;
    }
    std::swap(*this, b);
    return true;
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
    size_t x = dset.find(p + 1);
    for (size_t i = 0; i < 81; ++i) {
      if (x == dset.find(i + 1)) {
        if ((i % 9 > 0 && b[i - 1] == 0) || (i / 9 > 0 && b[i - 9] == 0) ||
            (i / 9 < 8 && b[i + 9] == 0) || (i % 9 < 8 && b[i + 1] == 0))
          return true;
      }
    }
    return false;
  }

private:
  __uint128_t board_[2] = {};
  DSet<81> dset_[2];
};