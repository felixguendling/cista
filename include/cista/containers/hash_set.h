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
struct basic_hash_set
    : public hash_storage<T, Ptr, uint32_t, identity, Hash, Eq> {};

}  // namespace cista
