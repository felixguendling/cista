#pragma once

#include <iterator>
#include <type_traits>

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

}  // namespace detail

template <typename T>
constexpr bool is_iterable_v = detail::is_iterable<T>::value;

}  // namespace cista