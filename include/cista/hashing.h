#pragma once

#include <functional>
#include <iostream>
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
    std::cout << "seed=" << seed << "\n";
    using Type = decay_t<T>;
    if constexpr (has_hash_v<Type>) {
      std::cout << "-> el.hash()\n";
      return el.hash();
    } else if constexpr (has_std_hash_v<Type>) {
      std::cout << "-> std::hash\n";
      return std::hash<Type>()(el);
    } else if constexpr (is_iterable_v<Type>) {
      std::cout << "-> std::iterable\n";
      for (auto const& v : el) {
        seed = hashing<decltype(v)>()(v, seed);
      }
      return seed;
    } else if constexpr (is_pointer_v<Type>) {
      std::cout << "-> T*\n";
      return el == nullptr ? hash_combine(seed, el)
                           : hashing<decltype(*el)>(seed, *el);
    } else if constexpr (std::is_aggregate_v<Type> &&
                         std::is_standard_layout_v<Type> &&
                         !std::is_polymorphic_v<Type>) {
      std::cout << "-> struct\n";
      for_each_field(seed, [&seed](auto const& f) {
        seed = hashing<decltype(f)>{}(f, seed);
        std::cout << "-> " << f << ":" << seed << "\n";
      });
      std::cout << "=> " << seed << "\n";
      return seed;
    } else if constexpr (std::is_scalar_v<Type>) {
      std::cout << "-> scalar: " << el << "\n";
      return hash_combine(seed, el);
    } else if constexpr (true) {
      static_assert(
          has_hash_v<Type> || has_std_hash_v<Type> || is_iterable_v<Type> ||
              is_pointer_v<Type> ||
              (std::is_aggregate_v<Type> && std::is_standard_layout_v<Type> &&
               !std::is_polymorphic_v<Type>) ||
              std::is_scalar_v<Type>,
          "Implement hash");
    }
  }
};

}  // namespace cista