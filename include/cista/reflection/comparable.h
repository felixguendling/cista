#pragma once

#include "cista/reflection/to_tuple.hpp"

#define MAKE_COMPARABLE()                                \
  template <typename T>                                  \
  bool operator==(T&& b) const {                         \
    return cista::to_tuple(*this) == cista::to_tuple(b); \
  }                                                      \
                                                         \
  template <typename T>                                  \
  bool operator!=(T&& b) const {                         \
    return cista::to_tuple(*this) != cista::to_tuple(b); \
  }                                                      \
                                                         \
  template <typename T>                                  \
  bool operator<(T&& b) const {                          \
    return cista::to_tuple(*this) < cista::to_tuple(b);  \
  }                                                      \
                                                         \
  template <typename T>                                  \
  bool operator<=(T&& b) const {                         \
    return cista::to_tuple(*this) <= cista::to_tuple(b); \
  }                                                      \
                                                         \
  template <typename T>                                  \
  bool operator>(T&& b) const {                          \
    return cista::to_tuple(*this) > cista::to_tuple(b);  \
  }                                                      \
                                                         \
  template <typename T>                                  \
  bool operator>=(T&& b) const {                         \
    return cista::to_tuple(*this) >= cista::to_tuple(b); \
  }
