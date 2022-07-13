#pragma once

#include <cstdint>

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "cista/containers/offset_ptr.h"
#include "cista/containers/string.h"
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

template <typename A, typename B>
struct is_hash_equivalent_helper : std::false_type {};

template <typename A, typename B>
constexpr bool is_hash_equivalent_v =
    is_hash_equivalent_helper<std::remove_cv_t<A>, std::remove_cv_t<B>>::value;

template <typename T, typename = void>
struct is_char_array_helper : std::false_type {};

template <std::size_t N>
struct is_char_array_helper<char const[N]> : std::true_type {};

template <std::size_t N>
struct is_char_array_helper<char[N]> : std::true_type {};

template <typename T>
constexpr bool is_char_array_v = is_char_array_helper<T>::value;

template <typename T>
constexpr bool is_string_like_v =
    is_string_v<std::remove_cv_t<T>> || is_char_array_v<T> ||
    std::is_same_v<T, char const*> ||
    std::is_same_v<std::remove_cv_t<T>, std::string> ||
    std::is_same_v<std::remove_cv_t<T>, std::string_view>;

template <typename A, typename B>
constexpr bool is_ptr_same = is_pointer_v<A>&& is_pointer_v<B>;

template <typename T>
struct hashing {
  template <typename A, typename B>
  static constexpr bool is_hash_equivalent() noexcept {
    using DecayA = decay_t<A>;
    using DecayB = decay_t<B>;
    return is_hash_equivalent_v<DecayA, DecayB> ||
           std::is_same_v<DecayA, DecayB> ||
           (is_string_like_v<DecayA> && is_string_like_v<DecayB>) ||
           std::is_convertible_v<A, B> || is_ptr_same<DecayA, DecayB>;
  }

  template <typename T1>
  static constexpr hashing<T1> create() noexcept {
    static_assert(is_hash_equivalent<T, T1>(), "Incompatible types");
    return hashing<T1>{};
  }

  constexpr hash_t operator()(T const& el, hash_t const seed = BASE_HASH) {
    using Type = decay_t<T>;
    if constexpr (has_hash_v<Type>) {
      return hash_combine(el.hash(), seed);
    } else if constexpr (is_pointer_v<Type>) {
      return hash_combine(seed, reinterpret_cast<intptr_t>(ptr_cast(el)));
    } else if constexpr (is_char_array_v<Type>) {
      return hash(std::string_view{el, sizeof(el) - 1}, seed);
    } else if constexpr (is_string_like_v<Type>) {
      using std::begin;
      using std::end;
      return el.size() == 0U
                 ? seed
                 : hash(std::string_view{&(*begin(el)), el.size()}, seed);
    } else if constexpr (std::is_scalar_v<Type>) {
      return hash_combine(seed, el);
    } else if constexpr (is_iterable_v<Type>) {
      auto h = seed;
      for (auto const& v : el) {
        h = hashing<decltype(v)>()(v, h);
      }
      return h;
    } else if constexpr (has_std_hash_v<Type>) {
      return std::hash<Type>()(el);
    } else if constexpr (to_tuple_works_v<Type>) {
      auto h = seed;
      for_each_field(el, [&h](auto&& f) { h = hashing<decltype(f)>{}(f, h); });
      return h;
    } else if (is_strong_v<Type>) {
      return hashing<typename Type::value_t>{}(el.v_);
    } else {
      static_assert(has_hash_v<Type> || std::is_scalar_v<Type> ||
                        has_std_hash_v<Type> || is_iterable_v<Type> ||
                        to_tuple_works_v<Type> || is_strong_v<Type>,
                    "Implement hash");
    }
  }
};

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
    hash_t h = seed;
    std::apply(
        [&h](auto&&... args) {
          ((h = hashing<decltype(args)>{}(args, h)), ...);
        },
        el);
    return h;
  }
};

template <>
struct hashing<char const*> {
  hash_t operator()(char const* el, hash_t const seed = BASE_HASH) {
    return hash(std::string_view{el}, seed);
  }
};

template <typename... Args>
hash_t build_hash(Args... args) {
  hash_t h = BASE_HASH;
  ((h = hashing<decltype(args)>{}(args, h)), ...);
  return h;
}

}  // namespace cista
