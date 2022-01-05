#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#if defined(_M_X64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)
#endif
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#endif

#include <cstddef>

namespace cista {

template <typename T>
constexpr unsigned trailing_zeros(T t) noexcept {
  static_assert(sizeof(T) == 8 || sizeof(T) == 4, "not supported");

  if (t == 0) {
    return sizeof(T) == 8 ? 64 : 32;
  }

  if constexpr (sizeof(T) == 8) {  // 64bit
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned long index = 0;
    _BitScanForward64(&index, t);
    return index;
#elif defined(_MSC_VER)
    unsigned long index = 0;
    if (static_cast<uint32_t>(t) == 0) {
      _BitScanForward(&index, t >> 32);
      return index + 32;
    }
    _BitScanForward(&index, static_cast<uint32_t>(t));
    return index;
#else
    return static_cast<unsigned>(__builtin_ctzll(t));
#endif
  } else if constexpr (sizeof(T) == 4) {  // 32bit
#if defined(_MSC_VER)
    unsigned long index = 0;
    _BitScanForward(&index, t);
    return index;
#else
    return static_cast<unsigned>(__builtin_ctz(t));
#endif
  }
}

template <typename T>
constexpr unsigned leading_zeros(T t) noexcept {
  static_assert(sizeof(T) == 8 || sizeof(T) == 4, "not supported");

  if (t == 0) {
    return sizeof(T) == 8 ? 64 : 32;
  }

  if constexpr (sizeof(T) == 8) {  // 64bit
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned long index = 0;
    if (_BitScanReverse64(&index, t)) {
      return 63 - index;
    }
    return 64;
#elif defined(_MSC_VER)
    unsigned long index = 0;
    if ((t >> 32) && _BitScanReverse(&index, t >> 32)) {
      return 31 - index;
    }
    if (_BitScanReverse(&index, static_cast<uint32_t>(t))) {
      return 63 - index;
    }
    return 64;
#else
    return static_cast<unsigned>(__builtin_clzll(t));
#endif
  } else if constexpr (sizeof(T) == 4) {  // 32bit
#if defined(_MSC_VER)
    unsigned long index = 0;
    if (_BitScanReverse(&index, t)) {
      return 31 - index;
    }
    return 32;
#else
    return static_cast<unsigned>(__builtin_clz(t));
#endif
  }
}

inline std::size_t popcount(std::uint64_t const b) {
#if defined(_MSC_VER) && defined(_M_X64)
  return __popcnt64(b);
#elif defined(_MSC_VER)
  return static_cast<std::size_t>(__popcnt(static_cast<uint32_t>(b)) +
                                  __popcnt(static_cast<uint32_t>(b >> 32)));
#elif defined(__INTEL_COMPILER)
  return static_cast<std::size_t>(_mm_popcnt_u64(b));
#else
  return static_cast<std::size_t>(__builtin_popcountll(b));
#endif
}

}  // namespace cista
