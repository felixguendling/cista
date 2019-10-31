#pragma once

#include <cinttypes>
#include <string_view>

#include "xxh3.h"

namespace cista {

// Algorithm: 64bit FNV-1a
// Source: http://www.isthe.com/chongo/tech/comp/fnv/

using hash_t = XXH64_hash_t;

constexpr auto const BASE_HASH = 0ULL;

template <typename... Args>
constexpr hash_t hash_combine(hash_t h, Args... val) {
  constexpr hash_t fnv_prime = 1099511628211ULL;
  auto fnv = [&](auto arg) { h = (h ^ static_cast<hash_t>(arg)) * fnv_prime; };
  ((fnv(val)), ...);
  return h;
}

inline hash_t hash(std::string_view s, hash_t h = BASE_HASH) {
  return XXH3_64bits_withSeed(s.data(), s.size(), h);
}

template <size_t N>
constexpr hash_t hash(const char (&str)[N], hash_t const h = BASE_HASH) {
  return XXH3_64bits_withSeed(str, N - 1, h);
}

template <typename T>
constexpr uint64_t hash(T const& buf, hash_t const h = BASE_HASH) {
  return XXH3_64bits_withSeed(&buf[0], buf.size(), h);
}

}  // namespace cista