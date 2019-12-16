#pragma once
#include <cstddef>

template <class T> class MemoryManager {
public:
  using size_type = size_t;
  static const constexpr size_type chunks = 12800000;
  static const constexpr size_type chunk_size = sizeof(T);
  static const constexpr size_type total_size = chunks * chunk_size;

  MemoryManager() { cur_ = storage_ = new char[total_size]; }
  MemoryManager(const MemoryManager &) = delete;
  MemoryManager(MemoryManager &&) = delete;
  MemoryManager &operator=(const MemoryManager &) = delete;
  MemoryManager &operator=(MemoryManager &&) noexcept = delete;
  ~MemoryManager() { delete[] storage_; }

  T *allocate(size_t size) {
    // assert((counter_ + size) * (chunk_size + sizeof(size_t)) < total_size);
    new (cur_) size_t(size);
    cur_ += sizeof(size_t);
    auto ret = reinterpret_cast<T *>(cur_);
    for (size_t i = 0; i < size; ++i, cur_ += chunk_size) {
      new (cur_) T;
    }
    // assert(cur_ == reinterpret_cast<char *>(ret) + chunk_size * size);
    counter_ += size;
    ++block_;
    return ret;
  }

  void free(T *ptr) {
    char *cur = reinterpret_cast<char *>(ptr) - sizeof(size_t);
    free_block(cur);
  }

  void destroy() {
    for (char *cur = storage_; block_ > 0;) {
      cur = free_block(cur);
    }
    cur_ = storage_;
    // assert(block_ == 0);
    // assert(counter_ == 0);
  }

private:
  inline char *free_block(char *ptr) {
    auto sptr = reinterpret_cast<size_t *>(ptr);
    ptr += sizeof(size_t);
    const size_t size = *sptr;
    sptr->~size_t();

    for (size_t i = 0; i < size; ++i, ptr += chunk_size) {
      reinterpret_cast<T *>(ptr)->~T();
    }
    counter_ -= size;
    --block_;
    return ptr;
  }

private:
  char *storage_, *cur_;
  size_t counter_ = 0, block_ = 0;
};