#pragma once

#include <type_traits>

namespace cista {

template <typename T>
using decay_t = std::remove_reference_t<std::remove_cv_t<T>>;

}  // namespace cista