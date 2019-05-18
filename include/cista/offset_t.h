#pragma once

#include <cinttypes>

#if _WIN32 || _WIN64
#if _WIN64
#define CISTA_64BIT
#else
#define CISTA_32BIT
#endif
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define CISTA_64BIT
#else
#define CISTA_32BIT
#endif
#endif

namespace cista {

#ifdef CISTA_64BIT
using offset_t = int64_t;
#define PRI_O PRId64
#endif

#ifdef CISTA_32BIT
using offset_t = int32_t;
#define PRI_O PRId32
#endif

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();

}  // namespace cista
