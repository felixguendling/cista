#pragma once

#include <functional>

#include "cista/containers/hash_storage.h"
#include "cista/containers/pair.h"
#include "cista/equal_to.h"
#include "cista/hashing.h"

namespace cista {

struct get_first {
  template <typename T>
  auto&& operator()(T&& t) noexcept {
    return t.first;
  }
};

struct get_second {
  template <typename T>
  auto&& operator()(T&& t) noexcept {
    return t.second;
  }
};

namespace raw {
template <typename Key, typename Value, typename Hash = hashing<Key>,
          typename Eq = equal_to<Key>>
using hash_map =
    hash_storage<pair<Key, Value>, ptr, get_first, get_second, Hash, Eq>;
}  // namespace raw

namespace offset {
template <typename Key, typename Value, typename Hash = hashing<Key>,
          typename Eq = equal_to<Key>>
using hash_map =
    hash_storage<pair<Key, Value>, ptr, get_first, get_second, Hash, Eq>;
}  // namespace offset

}  // namespace cista
