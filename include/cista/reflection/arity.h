#pragma once

#include <type_traits>

namespace cista {

// Source: playfulprogramming.blogspot.de

template <size_t N>
using field_count = std::integral_constant<size_t, N>;

struct wildcard {
  template <typename T,
            typename = std::enable_if_t<!std::is_lvalue_reference<T>::value>>
  operator T &&() const;

  template <typename T,
            typename = std::enable_if_t<std::is_copy_constructible<T>::value>>
  operator T&() const;
};

template <size_t N = 0>
static constexpr const wildcard _{};

template <typename T, size_t... I>
inline constexpr auto is_brace_constructible_(std::index_sequence<I...>, T*)
    -> decltype(T{_<I>...}, std::true_type{}) {
  return {};
}

template <size_t... I>
inline constexpr std::false_type is_brace_constructible_(
    std::index_sequence<I...>, ...) {
  return {};
}

template <typename T, size_t N>
constexpr auto is_brace_constructible()
    -> decltype(is_brace_constructible_(std::make_index_sequence<N>{},
                                        static_cast<T*>(nullptr))) {
  return {};
}

template <typename T, typename U>
struct is_paren_constructible_;

template <typename T, size_t... I>
struct is_paren_constructible_<T, std::index_sequence<I...>>
    : std::is_constructible<T, decltype(_<I>)...> {};

template <typename T, size_t N>
constexpr auto is_paren_constructible()
    -> is_paren_constructible_<T, std::make_index_sequence<N>> {
  return {};
}

#define CISTA_MAKE_ARITY_FUNC(count)                                        \
  template <typename T,                                                     \
            typename =                                                      \
                std::enable_if_t<is_brace_constructible<T, count>() &&      \
                                 !is_brace_constructible<T, count + 1>() && \
                                 !is_paren_constructible<T, count>()>>      \
  constexpr field_count<count> arity(T&) {                                  \
    return {};                                                              \
  }

CISTA_MAKE_ARITY_FUNC(1)
CISTA_MAKE_ARITY_FUNC(2)
CISTA_MAKE_ARITY_FUNC(3)
CISTA_MAKE_ARITY_FUNC(4)
CISTA_MAKE_ARITY_FUNC(5)
CISTA_MAKE_ARITY_FUNC(6)
CISTA_MAKE_ARITY_FUNC(7)
CISTA_MAKE_ARITY_FUNC(8)
CISTA_MAKE_ARITY_FUNC(9)
CISTA_MAKE_ARITY_FUNC(10)
CISTA_MAKE_ARITY_FUNC(11)
CISTA_MAKE_ARITY_FUNC(12)
CISTA_MAKE_ARITY_FUNC(13)
CISTA_MAKE_ARITY_FUNC(14)
CISTA_MAKE_ARITY_FUNC(15)
CISTA_MAKE_ARITY_FUNC(16)
CISTA_MAKE_ARITY_FUNC(17)
CISTA_MAKE_ARITY_FUNC(18)
CISTA_MAKE_ARITY_FUNC(19)
CISTA_MAKE_ARITY_FUNC(20)

#undef CISTA_MAKE_ARITY_FUNC

}  // namespace cista
