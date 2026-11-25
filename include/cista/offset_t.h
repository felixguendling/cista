#pragma once

#include <cinttypes>
#include <cstddef>
#include <array>
#include <limits>
#include <type_traits>

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

namespace detail {
template <typename T>
struct array_element {
  using type = void;
};
template <typename T, std::size_t N>
struct array_element<std::array<T, N>> {
  using type = T;
};
template <typename T, std::size_t N>
struct array_element<T[N]> {
  using type = T;
};
}  // namespace detail

template <typename T>
constexpr std::size_t serialized_alignment() noexcept {
  using element_t = typename detail::array_element<std::remove_cv_t<T>>::type;
  if constexpr (!std::is_void_v<element_t>) {
    constexpr auto const a = serialized_alignment<element_t>();
    return a > alignof(T) ? a : alignof(T);
  } else if constexpr (sizeof(offset_t) > sizeof(void*) &&
                       std::is_scalar_v<T> && sizeof(T) <= sizeof(offset_t) &&
                       alignof(T) < sizeof(T)) {
    return sizeof(T);
  } else {
    return alignof(T);
  }
}

template <typename T>
inline constexpr std::size_t serialized_alignment_v = serialized_alignment<T>();

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();
constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1U;

}  // namespace cista
