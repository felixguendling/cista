#pragma once

#include <iterator>
#include <type_traits>

#include "cista/decay.h"

namespace cista {
namespace detail {

using std::begin;
using std::end;

template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<decltype(begin(std::declval<T>())),
                                  decltype(end(std::declval<T>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct it_value {
  using type = void;
};

template <typename T>
struct it_value<T, std::enable_if_t<is_iterable<T>::value>> {
  using type = decay_t<decltype(*begin(std::declval<T>()))>;
};

}  // namespace detail

using detail::is_iterable;

template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

template <typename T>
using it_value_t = typename detail::it_value<T>::type;

}  // namespace cista
