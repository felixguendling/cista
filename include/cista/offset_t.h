#pragma once

#include <cinttypes>
#include <cstdint>
#include <limits>

namespace cista {

#ifdef CISTA_64BIT_OFFSET
using offset_t = std::int64_t;
#define PRI_O PRId64
#else
using offset_t = std::intptr_t;
#define PRI_O PRIdPTR
#endif

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();
constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1U;

}  // namespace cista
