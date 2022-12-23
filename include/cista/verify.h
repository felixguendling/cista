#pragma once

#include <stdexcept>

#include "cista/cista_exception.h"

namespace cista {

inline void verify(bool const condition, char const* msg) {
  if (!condition) {
    throw cista_exception{msg};
  }
}

}  // namespace cista
