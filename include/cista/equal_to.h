#pragma once

#include <algorithm>
#include <type_traits>

#include "cista/containers/pair.h"
#include "cista/decay.h"
#include "cista/is_iterable.h"
#include "cista/reflection/to_tuple.h"

namespace cista {

namespace detail {

template <class F, class Tuple, std::size_t... I>
constexpr bool tuple_equal_impl(F&& is_equal, Tuple&& a, Tuple&& b,
                                std::index_sequence<I...>) {
  return (is_equal(std::get<I>(std::forward<Tuple>(a)),
                   std::get<I>(std::forward<Tuple>(b))) &&
          ...);
}

}  // namespace detail

template <class F, class Tuple>
constexpr decltype(auto) tuple_equal(F&& is_equal, Tuple&& a, Tuple&& b) {
  return detail::tuple_equal_impl(
      std::forward<F>(is_equal), std::forward<Tuple>(a), std::forward<Tuple>(b),
      std::make_index_sequence<
          std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

template <typename A, typename B, typename = void>
struct is_eq_comparable : std::false_type {};

template <typename A, typename B>
struct is_eq_comparable<
    A, B, std::void_t<decltype(std::declval<A>() == std::declval<B>())>>
    : std::true_type {};

template <typename A, typename B>
constexpr bool is_eq_comparable_v = is_eq_comparable<A, B>::value;

template <typename T>
struct equal_to;

template <typename T>
struct equal_to {
  template <typename T1>
  constexpr bool operator()(T const& a, T1 const& b) const {
    using Type = decay_t<T>;
    using Type1 = decay_t<T1>;
    if constexpr (is_iterable_v<Type> && is_iterable_v<Type1>) {
      using std::begin;
      using std::end;
      auto const eq = std::equal(
          begin(a), end(a), begin(b), end(b),
          [](auto&& x, auto&& y) { return equal_to<decltype(x)>{}(x, y); });
      return eq;
    } else if constexpr (to_tuple_works_v<Type> && to_tuple_works_v<Type1>) {
      return tuple_equal(
          [](auto&& x, auto&& y) { return equal_to<decltype(x)>{}(x, y); },
          to_tuple(a), to_tuple(b));
    } else if constexpr (is_eq_comparable_v<Type, Type1>) {
      return a == b;
    } else {
      static_assert(is_iterable_v<Type> || is_eq_comparable_v<Type, Type1> ||
                        to_tuple_works_v<Type>,
                    "Implement custom equality");
    }
    return false;
  }
};

template <typename A, typename B>
struct equal_to<pair<A, B>> {
  template <typename T1>
  constexpr bool operator()(pair<A, B> const& a, T1 const& b) const {
    return a.first == b.first && a.second == b.second;
  }
};

}  // namespace cista
