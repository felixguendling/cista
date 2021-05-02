#pragma once

#include <stdexcept>

namespace cista {

inline void verify(bool const condition, char const* msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}

}  // namespace cista
