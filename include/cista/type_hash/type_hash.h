#pragma once

#include "cista/containers.h"
#include "cista/type_hash/hash.h"
#include "cista/type_hash/type_name.h"

namespace cista {

template <typename T>
hash_t type_hash() {
  auto const s = type_str<T>();
  printf("%.*s\n", static_cast<int>(s.size()), s.data());
  return hash(type_str<T>());
}

template <typename T>
hash_t type_hash(T const&) {
  auto const s = type_str<T>();
  printf("%.*s\n", static_cast<int>(s.size()), s.data());
  return hash(type_str<T>());
}

template <typename T>
struct use_standard_hash : public std::false_type {};

template <typename T>
hash_t type_hash(T const& el, hash_t hash) {
  using Type = decay_t<T>;
  if constexpr (use_standard_hash<Type>()) {
    return hash_combine(hash, type_hash<Type>());
  } else if constexpr (!std::is_scalar_v<Type>) {
    static_assert(std::is_aggregate_v<Type> &&
                      std::is_standard_layout_v<Type> &&
                      !std::is_polymorphic_v<Type>,
                  "Please implement custom type hash.");
    hash = hash_combine(hash, type_hash<Type>());
    for_each_field(el,
                   [&](auto const& member) { hash = type_hash(member, hash); });
    return hash;
  } else if constexpr (std::is_pointer_v<Type>) {
    hash = hash_combine(hash, type_hash<Type>());
    return hash_combine(hash, type_hash(*el, hash));
  } else {
    return hash_combine(hash, type_hash<Type>());
  }
}

template <typename T, size_t Size>
hash_t type_hash(array<T, Size> const&, hash_t hash) {
  hash = hash_combine(hash, type_hash<array<T, Size>>());
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(offset::ptr<T> const&, hash_t hash) {
  hash = hash_combine(hash, type_hash<offset::ptr<T>>());
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(offset::vector<T> const&, hash_t hash) {
  hash = hash_combine(hash, type_hash<offset::vector<T>>());
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(offset::unique_ptr<T> const&, hash_t hash) {
  hash = hash_combine(hash, type_hash<offset::unique_ptr<T>>());
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(raw::vector<T> const&, hash_t hash) {
  hash = hash_combine(hash, type_hash<raw::vector<T>>());
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(raw::unique_ptr<T> const&, hash_t hash) {
  hash = hash_combine(hash, type_hash<raw::unique_ptr<T>>());
  return type_hash(T{}, hash);
}

template <>
struct use_standard_hash<offset::string> : public std::true_type {};

template <>
struct use_standard_hash<raw::string> : public std::true_type {};

}  // namespace cista