#pragma once
#include <cstddef>
#include <memory>
#include <vector>

namespace mc::utils {

template <size_t BlockSize = 4096> class MemoryPool {
public:
  MemoryPool() : current_block_(0), current_offset_(0) { allocateNewBlock(); }

  ~MemoryPool() = default;

  void *allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {

    size = (size + alignment - 1) & ~(alignment - 1);

    if (current_offset_ + size > BlockSize) {
      allocateNewBlock();
    }

    void *ptr = blocks_[current_block_].get() + current_offset_;
    current_offset_ += size;
    return ptr;
  }

  void reset() {
    current_block_ = 0;
    current_offset_ = 0;
  }

  size_t totalAllocated() const { return blocks_.size() * BlockSize; }

private:
  void allocateNewBlock() {
    blocks_.emplace_back(std::make_unique<char[]>(BlockSize));
    current_block_ = blocks_.size() - 1;
    current_offset_ = 0;
  }

  std::vector<std::unique_ptr<char[]>> blocks_;
  size_t current_block_;
  size_t current_offset_;
};

extern thread_local MemoryPool<> g_memory_pool;

} // namespace mc::utils
