#pragma once
#include <array>

template <size_t N> struct DSet {
  size_t find(size_t x) { return x == dset[x] ? x : (dset[x] = find(dset[x])); }
  void unions(size_t x, size_t y) {
    if ((x = find(x)) == (y = find(y))) {
      return;
    }
    if (dsize[x] < dsize[y]) {
      dset[x] = y;
      dsize[y] += dsize[x];
    } else {
      dset[y] = x;
      dsize[x] += dsize[y];
    }
  }

  std::array<size_t, N> dset = []() constexpr {
    std::array<size_t, N> ret{};
    for (size_t i = 0; i < N; ++i) {
      ret[i] = i;
    }
    return ret;
  }
  ();
  std::array<size_t, N> dsize = []() constexpr {
    std::array<size_t, N> ret{};
    for (size_t i = 0; i < N; ++i) {
      ret[i] = 1;
    }
    return ret;
  }
  ();
};