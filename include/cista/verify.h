#pragma once

#include <stdexcept>
#include <iostream>

namespace cista {

inline void verify(bool const condition, char const* msg) {
  if (!condition) {
     std::cout << "Windows error: " << ::GetLastError() << std::endl;
    throw std::runtime_error(msg);
  }
}

}  // namespace cista