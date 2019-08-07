#pragma once

#include <cinttypes>
#include <functional>

#include "cista/containers/array.h"
#include "cista/containers/hash_map.h"
#include "cista/containers/hash_set.h"
#include "cista/containers/string.h"
#include "cista/containers/unique_ptr.h"
#include "cista/containers/vector.h"

// Helper macro to prevent copy&paste.
#define CISTA_DEFINITIONS                                                 \
  template <typename T, size_t Size>                                      \
  using array = cista::array<T, Size>;                                    \
                                                                          \
  template <typename T>                                                   \
  using unique_ptr = cista::basic_unique_ptr<T, ptr<T>>;                  \
                                                                          \
  template <typename T>                                                   \
  using vector = cista::basic_vector<T, ptr<T>>;                          \
                                                                          \
  using string = cista::basic_string<ptr<char const>>;                    \
                                                                          \
  template <typename Key, typename Value, typename Hash = std::hash<Key>, \
            typename Eq = std::equal_to<Key>>                             \
  using hash_map = basic_hash_map<Key, Value, ptr, Hash, Eq>;             \
                                                                          \
  template <typename T, typename Hash = std::hash<T>,                     \
            typename Eq = std::equal_to<T>>                               \
  using hash_set = basic_hash_set<T, ptr, Hash, Eq>;                      \
                                                                          \
  template <typename T, typename... Args>                                 \
  unique_ptr<T> make_unique(Args&&... args) {                             \
    return unique_ptr<T>{new T{std::forward<Args>(args)...}, true};       \
  }

namespace cista {

// =============================================================================
// Offset based data structures:
// [+] can be read without any deserialization step
// [+] suitable for shared memory applications
// [-] slower at runtime (pointers needs to be resolved using on more add)
// -----------------------------------------------------------------------------
namespace offset {

template <typename T>
using ptr = cista::offset_ptr<T>;

CISTA_DEFINITIONS
}  // namespace offset

// =============================================================================
// Raw data structures:
// [-] deserialize step takes time (but still very fast also for GBs of data)
// [-] the buffer containing the serialized data needs to be modified
// [+] fast runtime access (raw access)
// -----------------------------------------------------------------------------
namespace raw {

template <typename T>
using ptr = T*;

CISTA_DEFINITIONS
}  // namespace raw

}  // namespace cista