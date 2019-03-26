#pragma once

#include <cinttypes>

#include "cista/decay.h"

namespace cista {

template <typename T>
static inline constexpr size_t serialized_size() {
  return sizeof(decay_t<T>);
}

}  // namespace cista