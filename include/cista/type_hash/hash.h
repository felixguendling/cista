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

inline hash_t hash(std::string_view s = "",
                   hash_t h = 14695981039346656037ull) {
  auto const initial = h;
  printf("==============\n");
  printf("hash(\"%.*s\", %" PRIu64 ")\n", static_cast<int>(s.size()), s.data(),
         h);
  for (auto i = size_t{0ULL}; i < s.size(); ++i) {
    h = hash_combine(h, s[i]);
    printf("%zu\t%c\t%" PRIu64 "\n", i, s[i], h);
  }
  printf("hash(\"%.*s\", %" PRIu64 ") = %" PRIu64 "\n",
         static_cast<int>(s.size()), s.data(), initial, h);
  printf("--------------\n\n");
  return h;
}

}  // namespace cista