#pragma once

#include <ostream>

#ifndef CISTA_PRINTABLE_NO_VEC
#include <vector>
#endif

#include "cista/reflection/for_each_field.h"

#ifndef CISTA_PRINTABLE_NO_VEC
template <typename T>
inline std::ostream& operator<<(std::ostream& out, std::vector<T> const& v) {
  out << "[\n  ";
  auto first = true;
  for (auto const& e : v) {
    if (!first) {
      out << ",\n  ";
    }
    out << e;
    first = false;
  }
  return out << "\n]";
}
#endif

#define CISTA_PRINTABLE(class_name)                                         \
  friend std::ostream& operator<<(std::ostream& out, class_name const& o) { \
    bool first = true;                                                      \
    out << "{";                                                             \
    cista::for_each_field(o, [&](auto&& f) {                                \
      if (first) {                                                          \
        out << f;                                                           \
        first = false;                                                      \
      } else {                                                              \
        out << ", " << f;                                                   \
      }                                                                     \
    });                                                                     \
    return out << "}";                                                      \
  }
