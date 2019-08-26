#pragma once

#include <functional>
#include <type_traits>
#include <tuple>

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
struct has_std_hash<
    T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T>()))>>
    : std::true_type {};

}  // namespace detail

template <typename T>
constexpr bool has_hash_v = detail::has_hash<T>::value;

template <typename T>
constexpr bool has_std_hash_v = detail::has_std_hash<T>::value;

template <typename T>
struct hashing {
  constexpr hash_t operator()(T const& el, hash_t const seed = BASE_HASH) {
    using Type = decay_t<T>;
    if constexpr (has_hash_v<Type>) {
      return el.hash();
    } else if constexpr (std::is_scalar_v<Type>) {
      return hash_combine(seed, el);
    } else if constexpr (has_std_hash_v<Type>) {
      return std::hash<Type>()(el);
    } else if constexpr (is_iterable_v<Type>) {
      auto h = seed;
      for (auto const& v : el) {
        h = hashing<decltype(v)>()(v, h);
      }
      return h;
    } else if constexpr (to_tuple_works_v<Type>) {
      auto h = seed;
      for_each_field(el, [&h](auto&& f) { h = hashing<decltype(f)>{}(f, h); });
      return h;
    } else {
      static_assert(has_hash_v<Type> || std::is_scalar_v<Type> ||
                        has_std_hash_v<Type> || is_iterable_v<Type> ||
                        to_tuple_works_v<Type>,
                    "Implement hash");
    }
  }
};  // namespace cista

template <typename T1, typename T2>
struct hashing<std::pair<T1, T2>> {
  constexpr hash_t operator()(std::pair<T1, T2> const& el,
                              hash_t const seed = BASE_HASH) {
    std::size_t h = seed;
    h = hashing<T1>{}(el.first, h);
    h = hashing<T2>{}(el.second, h);
    return h;
  }
};

template <typename... Args>
struct hashing<std::tuple<Args...>> {
  constexpr hash_t operator()(std::tuple<Args...> const& el,
                              hash_t const seed = BASE_HASH) {
    std::size_t h = seed;
    std::apply(
        [&h](auto&&... args) {
          ((h = hashing<decltype(args)>{}(args, h)), ...);
        },
        el);
    return h;
  }
};

}  // namespace cista