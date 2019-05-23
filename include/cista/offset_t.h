#pragma once

#include <cinttypes>
#include <limits>

#define PRI_O PRIdPTR

namespace cista {

using offset_t = intptr_t;

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();

}  // namespace cista
