#pragma once

#include <functional>
#include <utility>

#include "cista/containers/hash_storage.h"
#include "cista/containers/ptr.h"
#include "cista/equal_to.h"
#include "cista/hashing.h"

namespace cista {

struct get_first {
  template <typename T>
  auto&& operator()(T&& t) {
    return t.first;
  }
};

struct get_second {
  template <typename T>
  auto&& operator()(T&& t) {
    return t.second;
  }
};

namespace raw {
template <typename Key, typename Value, typename Hash = hashing<Key>,
          typename Eq = equal_to<Key>>
using hash_map = hash_storage<std::pair<Key, Value>, ptr, uint32_t, get_first,
                              get_second, Hash, Eq>;
}  // namespace raw

namespace offset {
template <typename Key, typename Value, typename Hash = hashing<Key>,
          typename Eq = equal_to<Key>>
using hash_map = hash_storage<std::pair<Key, Value>, ptr, uint32_t, get_first,
                              get_second, Hash, Eq>;
}  // namespace offset

}  // namespace cista
