#pragma once

#include <cinttypes>
#include <limits>

#define PRI_O PRIdPTR

namespace cista {

using offset_t = intptr_t;

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();
constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1;

}  // namespace cista
