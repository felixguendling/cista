#pragma once

#include <type_traits>

namespace cista {

template <typename T, typename = void>
struct is_char_array_helper : std::false_type {};

template <std::size_t N>
struct is_char_array_helper<char const[N]> : std::true_type {};

template <std::size_t N>
struct is_char_array_helper<char[N]> : std::true_type {};

template <typename T>
constexpr bool is_char_array_v = is_char_array_helper<T>::value;

template <typename Ptr>
struct is_string_helper : std::false_type {};

template <class T>
constexpr bool is_string_v = is_string_helper<std::remove_cv_t<T>>::value;

}  // namespace cista