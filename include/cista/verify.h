#pragma once

#include <stdexcept>

#include "cista/cista_exception.h"
#include "cista/exception.h"

namespace cista {

inline void verify(bool const condition, char const* msg) {
  if (!condition) {
    throw_exception(cista_exception{msg});
  }
}

}  // namespace cista
