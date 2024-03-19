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

#include <cinttypes>
#include <cstddef>

namespace cista {

template <typename T>
inline constexpr unsigned constexpr_trailing_zeros(T t) {
  auto const is_bit_set = [&](unsigned const i) {
    return ((t >> i) & T{1U}) == T{1U};
  };
  for (auto i = 0U; i != sizeof(T) * 8U; ++i) {
    if (is_bit_set(i)) {
      return i;
    }
  }
  return 0U;
}

template <typename T>
constexpr unsigned trailing_zeros(T t) noexcept {
  static_assert(sizeof(T) == 8U || sizeof(T) == 4U, "not supported");

  if (t == 0U) {
    return sizeof(T) == 8U ? 64U : 32U;
  }

  if constexpr (sizeof(T) == 8U) {  // 64bit
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned long index = 0U;
    _BitScanForward64(&index, t);
    return index;
#elif defined(_MSC_VER)
    unsigned long index = 0U;
    if (static_cast<std::uint32_t>(t) == 0) {
      _BitScanForward(&index, t >> 32U);
      return index + 32U;
    }
    _BitScanForward(&index, static_cast<std::uint32_t>(t));
    return index;
#else
    return static_cast<unsigned>(__builtin_ctzll(t));
#endif
  } else if constexpr (sizeof(T) == 4U) {  // 32bit
#if defined(_MSC_VER)
    unsigned long index = 0U;
    _BitScanForward(&index, t);
    return index;
#else
    return static_cast<unsigned>(__builtin_ctz(t));
#endif
  }
}

template <typename T>
constexpr unsigned leading_zeros(T t) noexcept {
  static_assert(sizeof(T) == 8U || sizeof(T) == 4U, "not supported");

  if (t == 0U) {
    return sizeof(T) == 8U ? 64U : 32U;
  }

  if constexpr (sizeof(T) == 8U) {  // 64bit
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned long index = 0U;
    if (_BitScanReverse64(&index, t)) {
      return 63U - index;
    }
    return 64U;
#elif defined(_MSC_VER)
    unsigned long index = 0U;
    if ((t >> 32U) && _BitScanReverse(&index, t >> 32U)) {
      return 31U - index;
    }
    if (_BitScanReverse(&index, static_cast<std::uint32_t>(t))) {
      return 63U - index;
    }
    return 64U;
#else
    return static_cast<unsigned>(__builtin_clzll(t));
#endif
  } else if constexpr (sizeof(T) == 4U) {  // 32bit
#if defined(_MSC_VER)
    unsigned long index = 0;
    if (_BitScanReverse(&index, t)) {
      return 31U - index;
    }
    return 32U;
#else
    return static_cast<unsigned>(__builtin_clz(t));
#endif
  }
}

inline std::size_t popcount(std::uint64_t const b) noexcept {
#if defined(_MSC_VER) && defined(_M_X64)
  return __popcnt64(b);
#elif defined(_MSC_VER)
  return static_cast<std::size_t>(
      __popcnt(static_cast<std::uint32_t>(b)) +
      __popcnt(static_cast<std::uint32_t>(b >> 32U)));
#elif defined(__INTEL_COMPILER)
  return static_cast<std::size_t>(_mm_popcnt_u64(b));
#else
  return static_cast<std::size_t>(__builtin_popcountll(b));
#endif
}

}  // namespace cista
