#pragma once

#include <exception>

namespace cista {

struct cista_exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

}  // namespace cista
