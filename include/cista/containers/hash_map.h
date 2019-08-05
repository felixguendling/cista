#pragma once

#include <functional>
#include <utility>

#include "cista/containers/hash_set.h"

namespace cista {

struct map_key {
  template <typename T>
  auto operator()(T&& t) {
    return t.first;
  }
};

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Eq = std::equal_to<Key>>
struct hash_map
    : public cista::basic_hash_set<std::pair<Key, Value>, map_key, Hash, Eq> {};

}  // namespace cista
