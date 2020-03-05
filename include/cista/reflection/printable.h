#pragma once

#include <ostream>
#include <type_traits>

#ifndef CISTA_PRINTABLE_NO_VEC
#include <vector>
#endif

#include "cista/reflection/for_each_field.h"

namespace cista {

namespace enum_ostream_operators {

template <typename E, typename = std::enable_if_t<std::is_enum_v<std::decay_t<E>>>>
std::ostream& operator<<(std::ostream& out, E value) {
  return out << static_cast<std::underlying_type_t<std::decay_t<E>>>(value);
}

}  // namespace cista::enum_ostream_operators

}  // namespace cista

#ifndef CISTA_PRINTABLE_NO_VEC
template <typename T>
inline std::ostream& operator<<(std::ostream& out, std::vector<T> const& v) {
  using namespace ::cista::enum_ostream_operators;
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
    ::cista::for_each_field(o, [&](auto&& f) {                              \
      using namespace ::cista::enum_ostream_operators;                      \
      if (first) {                                                          \
        out << f;                                                           \
        first = false;                                                      \
      } else {                                                              \
        out << ", " << f;                                                   \
      }                                                                     \
    });                                                                     \
    return out << "}";                                                      \
  }
