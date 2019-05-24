#pragma once

#include <set>

#include "cista/containers.h"
#include "cista/decay.h"
#include "cista/hash.h"
#include "cista/reflection/for_each_field.h"
#include "cista/type_hash/type_name.h"

namespace cista {

template <typename T>
hash_t base_type_hash() {
  return hash(canonical_type_str<decay_t<T>>());
}

template <typename T>
struct use_standard_hash : public std::false_type {};

template <typename T>
hash_t type_hash(T const& el, hash_t h, std::set<hash_t> pred) {
  if (!pred.insert(base_type_hash<T>()).second) {
    return h;
  }

  using Type = decay_t<T>;
  if constexpr (use_standard_hash<Type>()) {
    return hash_combine(h, base_type_hash<T>());
  } else if constexpr (!std::is_scalar_v<Type>) {
    static_assert(std::is_aggregate_v<Type> &&
                      std::is_standard_layout_v<Type> &&
                      !std::is_polymorphic_v<Type>,
                  "Please implement custom type hash.");
    h = hash_combine(h, hash("struct"));
    for_each_field(el,
                   [&](auto const& member) { h = type_hash(member, h, pred); });
    return h;
  } else if constexpr (std::is_pointer_v<Type>) {
    return type_hash(typename std::remove_pointer_t<Type>{},
                     hash_combine(h, hash("pointer")), std::move(pred));
  } else {
    return hash_combine(h, base_type_hash<T>());
  }
}

template <typename T, size_t Size>
hash_t type_hash(array<T, Size> const&, hash_t h, std::set<hash_t> pred) {
  h = hash_combine(h, base_type_hash<array<T, Size>>());
  return type_hash(T{}, h, std::move(pred));
}

template <typename T>
hash_t type_hash(offset::ptr<T> const&, hash_t h, std::set<hash_t> pred) {
  h = hash_combine(h, base_type_hash<offset::ptr<T>>());
  return type_hash(T{}, h, std::move(pred));
}

template <typename T>
hash_t type_hash(offset::vector<T> const&, hash_t h, std::set<hash_t> pred) {
  h = hash_combine(h, base_type_hash<offset::vector<T>>());
  return type_hash(T{}, h, std::move(pred));
}

template <typename T>
hash_t type_hash(offset::unique_ptr<T> const&, hash_t h,
                 std::set<hash_t> pred) {
  h = hash_combine(h, base_type_hash<offset::unique_ptr<T>>());
  return type_hash(T{}, h, std::move(pred));
}

template <typename T>
hash_t type_hash(raw::vector<T> const&, hash_t h, std::set<hash_t> pred) {
  h = hash_combine(h, base_type_hash<raw::vector<T>>());
  return type_hash(T{}, h, std::move(pred));
}

template <typename T>
hash_t type_hash(raw::unique_ptr<T> const&, hash_t h, std::set<hash_t> pred) {
  h = hash_combine(h, base_type_hash<raw::unique_ptr<T>>());
  return type_hash(T{}, h, std::move(pred));
}

template <>
struct use_standard_hash<offset::string> : public std::true_type {};

template <>
struct use_standard_hash<raw::string> : public std::true_type {};

template <typename T>
hash_t type_hash() {
  return type_hash(T{}, base_type_hash<T>(), {});
}

}  // namespace cista
