#pragma once

#include <map>

#include "cista/containers.h"
#include "cista/decay.h"
#include "cista/hash.h"
#include "cista/indexed.h"
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
  auto [it, inserted] =
      done.try_emplace(base_hash, static_cast<unsigned>(done.size()));
  if (!inserted) {
    return hash_combine(h, it->second);
  }

  if constexpr (is_pointer_v<Type>) {
    return type_hash(remove_pointer_t<Type>{}, hash_combine(h, hash("pointer")),
                     done);
  } else if constexpr (std::is_scalar_v<Type>) {
    return hash_combine(h, type2str_hash<T>());
  } else {
    static_assert(to_tuple_works_v<Type>, "Please implement custom type hash.");
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

template <typename T, typename Ptr, bool Indexed, typename TemplateSizeType>
hash_t type_hash(basic_vector<T, Ptr, Indexed, TemplateSizeType> const&,
                 hash_t h, std::map<hash_t, unsigned>& done) {
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
hash_t type_hash(generic_string<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) {
  return hash_combine(h, hash("string"));
}

template <typename Ptr>
hash_t type_hash(basic_string<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) {
  return hash_combine(h, hash("string"));
}

template <typename Ptr>
hash_t type_hash(basic_string_view<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) {
  return hash_combine(h, hash("string"));
}

template <typename T>
hash_t type_hash(indexed<T> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) {
  return type_hash(T{}, h, done);
}

template <typename T>
hash_t type_hash() {
  auto done = std::map<hash_t, unsigned>{};
  return type_hash(T{}, BASE_HASH, done);
}

}  // namespace cista
