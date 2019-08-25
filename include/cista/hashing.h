#pragma once

#include <functional>
#include <type_traits>

#include "cista/containers/offset_ptr.h"
#include "cista/containers/unique_ptr.h"
#include "cista/decay.h"
#include "cista/hash.h"
#include "cista/is_iterable.h"
#include "cista/reflection/for_each_field.h"

namespace cista {

namespace detail {

template <typename T, typename = void>
struct has_hash : std::false_type {};

template <typename T>
struct has_hash<T, std::void_t<decltype(std::declval<T>().hash())>>
    : std::true_type {};

template <typename T, typename = void>
struct has_std_hash : std::false_type {};

template <typename T>
struct has_std_hash<T, std::void_t<decltype(std::declval<std::hash<T>>()())>>
    : std::true_type {};

}  // namespace detail

template <typename T>
constexpr bool has_hash_v = detail::has_hash<T>::value;

template <typename T>
constexpr bool has_std_hash_v = detail::has_std_hash<T>::value;

template <typename T>
struct hashing {
  constexpr hash_t operator()(T const& el, hash_t seed = BASE_HASH) {
    using Type = decay_t<T>;
    if constexpr (has_hash_v<Type>) {
      return el.hash();
    } else if constexpr (has_std_hash_v<Type>) {
      return std::hash<Type>()(el);
    } else if constexpr (is_iterable_v<Type>) {
      for (auto const& v : el) {
        seed = hashing<decltype(v)>()(v, seed);
      }
      return seed;
    } else if constexpr (is_pointer_v<Type>) {
      return el == nullptr ? hash_combine(seed, el)
                           : hashing<decltype(*el)>(seed, *el);
    } else if constexpr (std::is_aggregate_v<T> &&
                         std::is_standard_layout_v<T> &&
                         !std::is_polymorphic_v<T>) {
      for_each_field(seed, [&seed](auto const& f) {
        seed = hashing<decltype(f)>{}(f, seed);
      });
      return seed;
    } else if constexpr (std::is_scalar_v<Type>) {
      return hash_combine(seed, el);
    }
  }
};

}  // namespace cista