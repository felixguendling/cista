#pragma once

#include "cista/containers.h"
#include "cista/hash.h"
#include "cista/type_hash/type_name.h"

namespace cista {

template <typename T>
hash_t type_hash() {
  return hash(canonical_type_str<T>());
}

template <typename T>
hash_t type_hash(T const&) {
  return hash(canonical_type_str<T>());
}

template <typename T>
struct use_standard_hash : public std::false_type {};

template <typename T>
hash_t type_hash(T const& el, hash_t h) {
  using Type = decay_t<T>;
  if constexpr (use_standard_hash<Type>()) {
    return hash_combine(h, type_hash<Type>());
  } else if constexpr (!std::is_scalar_v<Type>) {
    static_assert(std::is_aggregate_v<Type> &&
                      std::is_standard_layout_v<Type> &&
                      !std::is_polymorphic_v<Type>,
                  "Please implement custom type hash.");
    h = hash_combine(h, hash("aggegate"));
    for_each_field(el, [&](auto const& member) { h = type_hash(member, h); });
    return h;
  } else if constexpr (std::is_pointer_v<Type>) {
    return type_hash(typename std::remove_pointer_t<Type>{},
                     hash_combine(h, hash("pointer")));
  } else {
    return hash_combine(h, type_hash<Type>());
  }
}

template <typename T, size_t Size>
hash_t type_hash(array<T, Size> const&, hash_t h) {
  h = hash_combine(h, type_hash<array<T, Size>>());
  return type_hash(T{}, h);
}

template <typename T>
hash_t type_hash(offset::ptr<T> const&, hash_t h) {
  h = hash_combine(h, type_hash<offset::ptr<T>>());
  return type_hash(T{}, h);
}

template <typename T>
hash_t type_hash(offset::vector<T> const&, hash_t h) {
  h = hash_combine(h, type_hash<offset::vector<T>>());
  return type_hash(T{}, h);
}

template <typename T>
hash_t type_hash(offset::unique_ptr<T> const&, hash_t h) {
  h = hash_combine(h, type_hash<offset::unique_ptr<T>>());
  return type_hash(T{}, h);
}

template <typename T>
hash_t type_hash(raw::vector<T> const&, hash_t h) {
  h = hash_combine(h, type_hash<raw::vector<T>>());
  return type_hash(T{}, h);
}

template <typename T>
hash_t type_hash(raw::unique_ptr<T> const&, hash_t h) {
  h = hash_combine(h, type_hash<raw::unique_ptr<T>>());
  return type_hash(T{}, h);
}

template <>
struct use_standard_hash<offset::string> : public std::true_type {};

template <>
struct use_standard_hash<raw::string> : public std::true_type {};

}  // namespace cista