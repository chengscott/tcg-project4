#pragma once
#include <cstddef>

template <size_t N> struct DSet {
  size_t find(size_t x) { return x == dset[x] ? x : (dset[x] = find(dset[x])); }
  void unions(size_t x, size_t y) {
    x = find(x);
    y = find(y);
    if (x == y || x == 0 || y == 0) {
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
  size_t dset[N + 1] = {}, dsize[N + 1] = {N};
};