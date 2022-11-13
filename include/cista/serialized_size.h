#pragma once

#include <cinttypes>

#include "cista/decay.h"

namespace cista {

template <typename T>
static inline constexpr size_t serialized_size(
    void* const param = nullptr) noexcept {
  static_cast<void>(param);
  return sizeof(decay_t<T>);
}

}  // namespace cista
