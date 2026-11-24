#pragma once

#include <cinttypes>
#include <cstdint>
#include <limits>

namespace cista {

// The type of the offsets stored in offset_ptr (and in every container that
// uses it). It has the pointer width unless CISTA_64BIT_OFFSET is defined:
// then 32bit programs use the 8 byte offsets of 64bit programs, so they can
// exchange buffers serialized in offset mode with them.
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
