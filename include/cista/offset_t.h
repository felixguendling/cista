#pragma once

#include <cinttypes>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace cista {

// The type of the offsets stored in offset_ptr (and in every container that
// uses it). It defaults to the pointer width. Define
// CISTA_CUSTOM_OFFSET_T_UNDERLYING_TYPE (e.g. to std::int64_t) to give
// offset_ptr the same size on every architecture, so that a buffer serialized
// in offset mode by a 32bit program can be read by a 64bit program and vice
// versa. Every program sharing the buffers has to use the same type.
#ifdef CISTA_CUSTOM_OFFSET_T_UNDERLYING_TYPE
using offset_t = CISTA_CUSTOM_OFFSET_T_UNDERLYING_TYPE;
#else
using offset_t = std::intptr_t;
#define PRI_O PRIdPTR
#endif

static_assert(std::is_integral_v<offset_t> && std::is_signed_v<offset_t>,
              "cista::offset_t has to be a signed integer type");
static_assert(sizeof(offset_t) >= sizeof(void*),
              "cista::offset_t has to be able to hold a pointer");

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();
constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1U;

}  // namespace cista
