#pragma once

#include <type_traits>

namespace cista {

enum class mode {
  NONE = 0,
  WITH_VERSION = 1 << 1,
  WITH_INTEGRITY = 1 << 2,
  SERIALIZE_BIG_ENDIAN = 1 << 3
};

constexpr mode operator|(mode const& a, mode const& b) {
  return mode{static_cast<std::underlying_type_t<mode>>(a) |
              static_cast<std::underlying_type_t<mode>>(b)};
}

constexpr mode operator&(mode const& a, mode const& b) {
  return mode{static_cast<std::underlying_type_t<mode>>(a) &
              static_cast<std::underlying_type_t<mode>>(b)};
}

}  // namespace cista