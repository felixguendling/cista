#pragma once

#include <cinttypes>
#include <limits>

namespace cista {

#ifdef CISTA_64BIT_OFFSET
#ifndef CISTA_SKIP_ALIGNMENT_CHECK
static_assert(alignof(std::int64_t) == 8 && alignof(double) == 8,
              "CISTA_64BIT_OFFSET requires 8 byte aligned 64bit integers and "
              "doubles (define CISTA_SKIP_ALIGNMENT_CHECK to skip this check)");
#endif
using offset_t = std::int64_t;
#define PRI_O PRId64
#else
using offset_t = std::intptr_t;
#define PRI_O PRIdPTR
#endif

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();
constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1U;

}  // namespace cista
