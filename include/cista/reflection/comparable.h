#pragma once

#include "cista/reflection/to_tuple.h"

#define CISTA_COMPARABLE()                               \
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

#define CISTA_FRIEND_COMPARABLE(class_name)                          \
  friend bool operator==(class_name const& a, class_name const& b) { \
    return cista::to_tuple(a) == cista::to_tuple(b);                 \
  }                                                                  \
                                                                     \
  friend bool operator!=(class_name const& a, class_name const& b) { \
    return cista::to_tuple(a) != cista::to_tuple(b);                 \
  }                                                                  \
                                                                     \
  friend bool operator<(class_name const& a, class_name const& b) {  \
    return cista::to_tuple(a) < cista::to_tuple(b);                  \
  }                                                                  \
                                                                     \
  friend bool operator<=(class_name const& a, class_name const& b) { \
    return cista::to_tuple(a) <= cista::to_tuple(b);                 \
  }                                                                  \
                                                                     \
  friend bool operator>(class_name const& a, class_name const& b) {  \
    return cista::to_tuple(a) > cista::to_tuple(b);                  \
  }                                                                  \
                                                                     \
  friend bool operator>=(class_name const& a, class_name const& b) { \
    return cista::to_tuple(a) >= cista::to_tuple(b);                 \
  }
