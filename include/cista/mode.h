#pragma once

#include <type_traits>

namespace cista {

enum class mode {
  NONE = 0U,
  UNCHECKED = 1U << 0U,
  WITH_VERSION = 1U << 1U,
  WITH_INTEGRITY = 1U << 2U,
  SERIALIZE_BIG_ENDIAN = 1U << 3U,
  DEEP_CHECK = 1U << 4U,
  CAST = 1U << 5U,
  _CONST = 1U << 29U,
  _PHASE_II = 1U << 30U
};

constexpr mode operator|(mode const& a, mode const& b) noexcept {
  return mode{static_cast<std::underlying_type_t<mode>>(a) |
              static_cast<std::underlying_type_t<mode>>(b)};
}

constexpr mode operator&(mode const& a, mode const& b) noexcept {
  return mode{static_cast<std::underlying_type_t<mode>>(a) &
              static_cast<std::underlying_type_t<mode>>(b)};
}

constexpr bool is_mode_enabled(mode const in, mode const flag) noexcept {
  return (in & flag) == flag;
}

constexpr bool is_mode_disabled(mode const in, mode const flag) noexcept {
  return (in & flag) == mode::NONE;
}

}  // namespace cista
