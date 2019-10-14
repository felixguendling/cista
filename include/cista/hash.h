#pragma once

#include <cinttypes>
#include <string_view>

namespace cista {

// Algorithm: 64bit FNV-1a
// Source: http://www.isthe.com/chongo/tech/comp/fnv/

using hash_t = std::uint64_t;

constexpr auto const BASE_HASH = 14695981039346656037ULL;

template <typename... Args>
constexpr hash_t hash_combine(hash_t h, Args... val) {
  constexpr hash_t fnv_prime = 1099511628211ULL;
  auto fnv = [&](auto arg) { h = (h ^ static_cast<hash_t>(arg)) * fnv_prime; };
  ((fnv(val)), ...);
  return h;
}

inline hash_t hash(std::string_view s, hash_t h = BASE_HASH) {
  auto const ptr = reinterpret_cast<uint8_t const*>(s.data());
  for (auto i = size_t{0ULL}; i < s.size(); ++i) {
    h = hash_combine(h, ptr[i]);
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