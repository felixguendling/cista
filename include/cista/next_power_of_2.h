#pragma once

namespace cista {

template <typename TemplateSizeType>
constexpr TemplateSizeType next_power_of_two(TemplateSizeType n) noexcept {
  n--;
  n |= n >> 1U;
  n |= n >> 2U;
  n |= n >> 4U;
  n |= n >> 8U;
  n |= n >> 16U;
  if constexpr (sizeof(TemplateSizeType) > 32U) {
    n |= n >> 32U;
  }
  n++;
  return n;
}

}  // namespace cista
