#pragma once

#include <array>

namespace cista {

template <typename T, std::size_t Size>
using array = std::array<T, Size>;

namespace raw {
using cista::array;
}  // namespace raw

namespace offset {
using cista::array;
}  // namespace offset

}  // namespace cista
