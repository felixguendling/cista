#pragma once

#include <map>

#include "cista/containers.h"
#include "cista/decay.h"
#include "cista/hash.h"
#include "cista/reflection/for_each_field.h"
#include "cista/type_hash/type_name.h"

namespace cista {

template <typename T>
hash_t type2str_hash() {
  return hash(canonical_type_str<decay_t<T>>());
}

template <typename T>
hash_t type_hash(T const& el, hash_t h, std::map<hash_t, unsigned>& done) {
  using Type = decay_t<T>;

  auto const base_hash = type2str_hash<Type>();
  if (auto it = done.lower_bound(base_hash);
      it != end(done) && it->first == base_hash) {
    return hash_combine(h, it->second);
  } else {
    done.emplace_hint(it, base_hash, done.size());
  }

  if constexpr (is_pointer_v<Type>) {
    return type_hash(remove_pointer_t<Type>{}, hash_combine(h, hash("pointer")),
                     done);
  } else if constexpr (std::is_scalar_v<Type>) {
    return hash_combine(h, type2str_hash<T>());
  } else {
    static_assert(std::is_aggregate_v<Type> &&
                      std::is_standard_layout_v<Type> &&
                      !std::is_polymorphic_v<Type>,
                  "Please implement custom type hash.");
    h = hash_combine(h, hash("struct"));
    for_each_field(el,
                   [&](auto const& member) { h = type_hash(member, h, done); });
    return h;
  }
}

template <typename T, size_t Size>
hash_t type_hash(array<T, Size> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) {
  h = hash_combine(h, hash("array"));
  h = hash_combine(h, Size);
  return type_hash(T{}, h, done);
}

template <typename T, typename Ptr, typename TemplateSizeType>
hash_t type_hash(basic_vector<T, Ptr, TemplateSizeType> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) {
  h = hash_combine(h, hash("vector"));
  return type_hash(T{}, h, done);
}

template <typename T, typename Ptr>
hash_t type_hash(basic_unique_ptr<T, Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) {
  h = hash_combine(h, hash("unique_ptr"));
  return type_hash(T{}, h, done);
}

template <typename Ptr>
hash_t type_hash(basic_string<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) {
  return hash_combine(h, hash("string"));
}

template <typename T>
hash_t type_hash() {
  auto done = std::map<hash_t, unsigned>{};
  return type_hash(T{}, BASE_HASH, done);
}

}  // namespace cista
