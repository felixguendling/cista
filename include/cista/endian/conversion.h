#pragma once

#include "cista/endian/detection.h"
#include "cista/mode.h"

// Based on
// https://github.com/google/flatbuffers/blob/master/include/flatbuffers/base.h

#if defined(_MSC_VER)
#define CISTA_BYTESWAP_16 _byteswap_ushort
#define CISTA_BYTESWAP_32 _byteswap_ulong
#define CISTA_BYTESWAP_64 _byteswap_uint64
#else
#define CISTA_BYTESWAP_16 __builtin_bswap16
#define CISTA_BYTESWAP_32 __builtin_bswap32
#define CISTA_BYTESWAP_64 __builtin_bswap64
#endif

namespace cista {

template <typename T>
constexpr T endian_swap(T t) {
  static_assert(sizeof(T) == 1U || sizeof(T) == 2U || sizeof(T) == 4U ||
                sizeof(T) == 8U);

  if constexpr (sizeof(T) == 1U) {
    return t;
  } else if constexpr (sizeof(T) == 2U) {
    union {
      T t;
      uint16_t i;
    } u{t};
    u.i = CISTA_BYTESWAP_16(u.i);
    return u.t;
  } else if constexpr (sizeof(T) == 4U) {
    union {
      T t;
      uint32_t i;
    } u{t};
    u.i = CISTA_BYTESWAP_32(u.i);
    return u.t;
  } else if constexpr (sizeof(T) == 8U) {
    union {
      T t;
      uint64_t i;
    } u{t};
    u.i = CISTA_BYTESWAP_64(u.i);
    return u.t;
  }
}

template <mode Mode>
constexpr bool endian_conversion_necessary() {
  if constexpr ((Mode & mode::SERIALIZE_BIG_ENDIAN) ==
                mode::SERIALIZE_BIG_ENDIAN) {
#if defined(CISTA_BIG_ENDIAN)
    return false;
#else
    return true;
#endif
  } else {
#if defined(CISTA_LITTLE_ENDIAN)
    return false;
#else
    return true;
#endif
  }
}

template <mode Mode, typename T>
constexpr T convert_endian(T t) {
  if constexpr (endian_conversion_necessary<Mode>()) {
    return endian_swap(t);
  } else {
    return t;
  }
}

}  // namespace cista

#undef CISTA_BYTESWAP_16
#undef CISTA_BYTESWAP_32
#undef CISTA_BYTESWAP_64
