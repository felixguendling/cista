#pragma once

#include <functional>

#include "cista/containers/hash_storage.h"

namespace cista {

struct identity {
  template <typename T>
  auto operator()(T&& t) {
    return std::forward<T>(t);
  }
};

template <typename T, template <typename> typename Ptr,
          typename Hash = std::hash<T>, typename Eq = std::equal_to<T>>
using basic_hash_set =
    hash_storage<T, Ptr, uint32_t, identity, identity, Hash, Eq>;

}  // namespace cista
