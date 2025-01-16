#pragma once

#include <type_traits>

namespace cista {

template <class T, class R = void>
struct enable_if_type {
  using type = R;
};

template <typename T, typename = void>
struct has_const_iterator : std::false_type {};

template <typename T>
struct has_const_iterator<
    T, typename enable_if_type<typename T::const_iterator>::type>
    : std::true_type {};

template <typename T>
inline constexpr bool has_const_iterator_v = has_const_iterator<T>::value;

template <typename Container, typename Enable = void>
struct const_iterator {
  using type = typename Container::iterator;
};

template <typename Container>
struct const_iterator<Container,
                      std::enable_if_t<has_const_iterator_v<Container>>> {
  using type = typename Container::const_iterator;
};

template <typename T>
using const_iterator_t = typename const_iterator<T>::type;

}  // namespace cista