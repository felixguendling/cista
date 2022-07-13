#pragma once

#include <array>
#include <ostream>

#ifndef CISTA_PRINTABLE_NO_VEC
#include <vector>
#endif

#include "cista/decay.h"
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
    using Type = cista::decay_t<T>;
    if constexpr (std::is_enum_v<Type>) {
      out << static_cast<std::underlying_type_t<Type>>(e);
    } else {
      out << e;
    }
    first = false;
  }
  return out << "\n]";
}
#endif

template <typename... T>
constexpr std::array<char const*, sizeof...(T)> to_str_array(T... args) {
  return {args...};
}

#define CISTA_PRINTABLE(class_name, ...)                                    \
  friend std::ostream& operator<<(std::ostream& out, class_name const& o) { \
    constexpr auto const names = to_str_array(__VA_ARGS__);                 \
    bool first = true;                                                      \
    out << "{";                                                             \
    size_t i = 0;                                                           \
    ::cista::for_each_field(o, [&](auto&& f) {                              \
      using Type = ::cista::decay_t<decltype(f)>;                           \
      if (!first) {                                                         \
        out << ", ";                                                        \
      } else {                                                              \
        first = false;                                                      \
      }                                                                     \
      if (i < names.size()) {                                               \
        out << names[i] << '=';                                             \
      }                                                                     \
      if constexpr (std::is_enum_v<Type>) {                                 \
        out << static_cast<std::underlying_type_t<Type>>(f);                \
      } else {                                                              \
        out << f;                                                           \
      }                                                                     \
      ++i;                                                                  \
    });                                                                     \
    return out << "}";                                                      \
  }
