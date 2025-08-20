#include "memory_pool.hpp"

namespace mc::utils {

thread_local MemoryPool<> g_memory_pool;

} // namespace mc::utils
