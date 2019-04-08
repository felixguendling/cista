#pragma once

// Credits: Jonathan Müller (@foonathan)
// https://github.com/foonathan/string_id/blob/master/hash.hpp

#include <cinttypes>
#include <string_view>

namespace cista {

using hash_t = std::uint64_t;

template <typename T>
constexpr hash_t hash_combine(hash_t const h, T const val) {
  constexpr hash_t prime = 1099511628211ull;
  return (h ^ static_cast<hash_t>(val)) * prime;
}

constexpr hash_t hash(std::string_view s = "",
                      hash_t h = 14695981039346656037ull) {
  for (auto i = size_t{0ULL}; i < s.size(); ++i) {
    h = hash_combine(h, s[i]);
  }
  return h;
}

}  // namespace cista