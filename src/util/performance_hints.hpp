#pragma once

#include <stdint.h>
#include <cstdio>

#ifdef __GNUC__
  #define MC_LIKELY(x)       __builtin_expect(!!(x), 1)
  #define MC_UNLIKELY(x)     __builtin_expect(!!(x), 0)
  #define MC_FORCE_INLINE    __attribute__((always_inline)) inline
  #define MC_NO_INLINE       __attribute__((noinline))
  #define MC_PURE            __attribute__((pure))
  #define MC_CONST           __attribute__((const))
  #define MC_HOT             __attribute__((hot))
  #define MC_COLD            __attribute__((cold))
  #define MC_PREFETCH(addr)  __builtin_prefetch(addr, 0, 3)
#else
  #define MC_LIKELY(x)       (x)
  #define MC_UNLIKELY(x)     (x)
  #define MC_FORCE_INLINE    inline
  #define MC_NO_INLINE
  #define MC_PURE
  #define MC_CONST
  #define MC_HOT
  #define MC_COLD
  #define MC_PREFETCH(addr)
#endif

constexpr size_t MC_CACHE_LINE_SIZE = 64;

#define MC_CACHE_ALIGNED alignas(MC_CACHE_LINE_SIZE)

namespace mc::utils {

MC_FORCE_INLINE void fast_memcpy(void* dest, const void* src, size_t n) {
    if (MC_LIKELY(n > 0)) {
        __builtin_memcpy(dest, src, n);
    }
}

MC_FORCE_INLINE void fast_memset(void* dest, int value, size_t n) {
    if (MC_LIKELY(n > 0)) {
        __builtin_memset(dest, value, n);
    }
}

MC_FORCE_INLINE MC_CONST uint32_t fast_bswap32(uint32_t x) {
#ifdef __GNUC__
    return __builtin_bswap32(x);
#else
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >> 8)  |
           ((x & 0x0000FF00) << 8)  |
           ((x & 0x000000FF) << 24);
#endif
}

MC_FORCE_INLINE MC_CONST uint64_t fast_bswap64(uint64_t x) {
#ifdef __GNUC__
    return __builtin_bswap64(x);
#else
    return ((x & 0xFF00000000000000ULL) >> 56) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x000000FF00000000ULL) >> 8)  |
           ((x & 0x00000000FF000000ULL) << 8)  |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x00000000000000FFULL) << 56);
#endif
}

} // namespace mc::utils
