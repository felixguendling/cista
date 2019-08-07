#pragma once

#include <functional>
#include <utility>

#include "cista/containers/hash_storage.h"

namespace cista {

struct get_first {
  template <typename T>
  auto operator()(T&& t) {
    return t.first;
  }
};

template <typename Key, typename Value, template <typename> typename Ptr,
          typename Hash = std::hash<Key>, typename Eq = std::equal_to<Key>>
struct basic_hash_map : public hash_storage<std::pair<Key, Value>, Ptr,
                                            uint32_t, get_first, Hash, Eq> {};

}  // namespace cista
