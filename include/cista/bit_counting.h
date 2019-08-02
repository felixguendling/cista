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
constexpr size_t trailing_zeros(T t) {
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
    _BitScanForward(&index, t);
    return index;
#else
    return static_cast<size_t>(__builtin_ctzll(t));
#endif
  } else if constexpr (sizeof(T) == 4) {  // 32bit
#if defined(_MSC_VER)
    unsigned long index = 0;
    _BitScanForward(&index, t);
    return index;
#else
    return static_cast<size_t>(__builtin_ctz(t));
#endif
  }
}

template <typename T>
constexpr size_t leading_zeros(T t) {
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
    if ((n >> 32) && _BitScanReverse(&index, t >> 32)) {
      return 31 - index;
    }
    if (_BitScanReverse(&index, t)) {
      return 63 - index;
    }
    return 64;
#else
    return static_cast<size_t>(__builtin_clzll(t));
#endif
  } else if constexpr (sizeof(T) == 4) {  // 32bit
#if defined(_MSC_VER)
    unsigned long index = 0;
    if (_BitScanReverse(&index, t)) {
      return 31 - index;
    }
    return 32;
#else
    return static_cast<size_t>(__builtin_clz(t));
#endif
  }
}

}  // namespace cista