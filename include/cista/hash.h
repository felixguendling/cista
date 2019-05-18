#pragma once

#include <cinttypes>
#include <string_view>

namespace cista {

// Algorithm: 64bit FNV-1a
// Source: http://www.isthe.com/chongo/tech/comp/fnv/

using hash_t = std::uint64_t;

constexpr auto const BASE_HASH = 14695981039346656037ULL;

template <typename T>
constexpr hash_t hash_combine(hash_t const h, T const val) {
  constexpr hash_t prime = 1099511628211ULL;
  return (h ^ static_cast<hash_t>(val)) * prime;
}

constexpr hash_t hash(std::string_view s, hash_t h = BASE_HASH) {
  for (auto i = size_t{0ULL}; i < s.size(); ++i) {
    h = hash_combine(h, s[i]);
  }
  return h;
}

template <size_t N>
constexpr hash_t hash(const char (&str)[N], hash_t const h = BASE_HASH) {
  return hash(std::string_view{str, N - 1}, h);
}

template <typename T>
constexpr uint64_t hash(T const& buf, hash_t const h = BASE_HASH) {
  return hash(
      std::string_view{reinterpret_cast<char const*>(&buf[0]), buf.size()}, h);
}

}  // namespace cista