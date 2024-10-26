#pragma once

#include <stdexcept>
#include <string>

#include "cista/cista_exception.h"
#include "cista/exception.h"

namespace cista {

inline void verify(bool const condition, std::string msg) {
  if (!condition) {
    throw_exception(cista_exception{std::move(msg)});
  }
}

}  // namespace cista
