#pragma once

#include <functional>

#include "cista/containers/hash_storage.h"
#include "cista/equal_to.h"
#include "cista/hashing.h"

namespace cista {

struct identity {
  template <typename T>
  decltype(auto) operator()(T&& t) const noexcept {
    return std::forward<T>(t);
  }
};

namespace raw {
template <typename T, typename Hash = hashing<T>, typename Eq = equal_to<T>>
using hash_set = hash_storage<T, ptr, identity, identity, Hash, Eq>;
}  // namespace raw

namespace offset {
template <typename T, typename Hash = hashing<T>, typename Eq = equal_to<T>>
using hash_set = hash_storage<T, ptr, identity, identity, Hash, Eq>;
}  // namespace offset

}  // namespace cista
