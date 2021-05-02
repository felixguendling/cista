#pragma once

#include <functional>
#include <type_traits>

namespace cista {

namespace detail {

template <typename T>
struct decay {
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T>
struct decay<std::reference_wrapper<T>> {
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

}  // namespace detail

template <typename T>
using decay_t = typename detail::decay<std::remove_reference_t<T>>::type;

}  // namespace cista
