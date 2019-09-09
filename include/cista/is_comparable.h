#pragma once

#include <type_traits>

namespace cista {

template <typename A, typename B, typename = void>
struct is_eq_comparable : std::false_type {};

template <typename A, typename B>
struct is_eq_comparable<
    A, B, std::void_t<decltype(std::declval<A>() == std::declval<B>())>>
    : std::true_type {};

template <typename A, typename B, typename = void>
struct is_lt_comparable : std::false_type {};

template <typename A, typename B>
struct is_lt_comparable<
    A, B, std::void_t<decltype(std::declval<A>() < std::declval<B>())>>
    : std::true_type {};

template <typename A, typename B>
constexpr bool is_eq_comparable_v = is_eq_comparable<A, B>::value;

template <typename A, typename B>
constexpr bool is_lt_comparable_v = is_lt_comparable<A, B>::value;

}  // namespace cista
