#pragma once

#include <type_traits>

namespace cista {

template <typename T>
struct indexed : public T {
  using value_type = T;
  using T::T;
  using T::operator=;
};

template <typename Ptr>
struct is_indexed_helper : std::false_type {};

template <typename T>
struct is_indexed_helper<indexed<T>> : std::true_type {};

template <class T>
constexpr bool is_indexed_v = is_indexed_helper<std::remove_cv_t<T>>::value;

}  // namespace cista

#if __has_include("fmt/ostream.h")

#include "fmt/ostream.h"

template <typename T>
struct fmt::formatter<cista::indexed<T>> : ostream_formatter {};

#endif
