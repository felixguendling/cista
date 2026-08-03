#pragma once

#include <cinttypes>
#include <limits>

#define PRI_O PRIdPTR

namespace cista {

#ifdef CISTA_CUSTOM_OFFSET_T_UNDERLYING_TYPE
using offset_t = CISTA_CUSTOM_OFFSET_T_UNDERLYING_TYPE;
#else
using offset_t = intptr_t;
#endif

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();
constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1U;

}  // namespace cista
