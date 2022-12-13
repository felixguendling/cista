#pragma once

#include <type_traits>

#include "cista/hash.h"
#include "cista/containers.h"

namespace cista {

template <class T>
inline constexpr bool dependent_false_v = false;

template<class T, class = void>
struct has_static_hash : std::false_type {};

template<class T>
struct has_static_hash<T, std::enable_if_t<std::is_invocable_r_v<hash_t, decltype(T::static_type_hash)>>>
: std::true_type
{};

template <class T, size_t N>
struct has_static_hash<array<T, N>>: std::true_type {};

template<class T>
constexpr bool has_static_hash_v = has_static_hash<T>::value;

template <typename T>
hash_t static_type_hash(hash_t h)
{
  if constexpr (std::is_arithmetic_v<T>)
  {
    if constexpr (std::is_integral_v<T>)
    {
        if constexpr (std::is_signed_v<T>)
        return hash_combine(h, hash("unsigned integer"), sizeof(T));
        else return hash_combine(h, hash("signed integer"), sizeof(T));
    }
    else return hash_combine(h, hash("floating point"), sizeof(T));
  }

  else if constexpr (is_cista_array_v<T>)
    return static_type_hash<typename T::value_type>(hash_combine(h, hash("array"), T::size()));

  else if constexpr (has_static_hash_v<T>)
    return T::static_type_hash(h);

  else static_assert(dependent_false_v<T>, "One of the classes do not implement static type hashing.");
}

template <typename T>
hash_t static_type_hash() {
  return static_type_hash<T>(BASE_HASH);
}

}
