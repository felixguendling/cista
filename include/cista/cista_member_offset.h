#pragma once

#include "cista/offset_t.h"

namespace cista {

template <typename T, typename Member>
cista::offset_t member_offset(T const* t, Member const* m) {
  static_assert(std::is_trivially_copyable_v<T>);
  return (reinterpret_cast<std::uint8_t const*>(m) -
          reinterpret_cast<std::uint8_t const*>(t));
}

template <typename T, typename Member>
offset_t member_offset(T const* t, Member T::*m) {
  static_assert(std::is_trivially_copyable_v<T>);
  return (reinterpret_cast<std::uint8_t const*>(&(t->*m)) -
          reinterpret_cast<std::uint8_t const*>(t));
}

}  // namespace cista

#ifndef cista_member_offset
#define cista_member_offset(Type, Member)                            \
  ([]() {                                                            \
    if constexpr (std::is_standard_layout_v<Type>) {                 \
      return static_cast<::cista::offset_t>(offsetof(Type, Member)); \
    } else {                                                         \
      return ::cista::member_offset(static_cast<Type*>(nullptr),     \
                                    &Type::Member);                  \
    }                                                                \
  }())
#endif