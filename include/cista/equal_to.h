#pragma once

#include <algorithm>
#include <type_traits>

#include "cista/is_iterable.h"
#include "cista/reflection/to_tuple.h"

namespace cista {

namespace detail {

template <typename T, typename = void>
struct is_eq_comparable : std::false_type {};

template <typename T>
struct is_eq_comparable<
    T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>>
    : std::true_type {};

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

template <typename T>
constexpr bool is_eq_comparable_v = detail::is_eq_comparable<T>::value;

template <typename T>
struct equal_to {
  constexpr bool operator()(T const& a, T const& b) const {
    if constexpr (is_eq_comparable_v<T>) {
      return a == b;
    } else if constexpr (std::is_aggregate_v<T> &&
                         std::is_standard_layout_v<T> &&
                         !std::is_polymorphic_v<T>) {
      return tuple_equal(
          [](auto&& x, auto&& y) { return equal_to<decltype(x)>{}(x, y); },
          to_tuple(a), to_tuple(b));
    } else if constexpr (is_iterable_v<T>) {
      using std::begin;
      using std::end;
      return std::equal(
          begin(a), end(a), begin(b), end(b),
          [](auto&& x, auto&& y) { return equal_to<decltype(x)>{}(x, y); });
    }
  }
};

}  // namespace cista