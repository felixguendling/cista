#pragma once

#include <cinttypes>
#include <string_view>

namespace cista {

#if defined(CISTA_XXH3)

#include "xxh3.h"

using hash_t = XXH64_hash_t;

constexpr auto const BASE_HASH = 0ULL;

template <typename... Args>
constexpr hash_t hash_combine(hash_t h, Args... val) {
  auto xxh3 = [&](auto const& arg) {
    h = XXH3_64bits_withSeed(&arg, sizeof(arg), h);
  };
  ((xxh3(val)), ...);
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
  return buf.size() == 0 ? h : XXH3_64bits_withSeed(&buf[0], buf.size(), h);
}

#elif defined(CISTA_WYHASH)

#include "wyhash.h"

using hash_t = uint64_t;

constexpr auto const BASE_HASH = 34432ULL;

template <typename... Args>
constexpr hash_t hash_combine(hash_t h, Args... val) {
  auto wy = [&](auto const& arg) {
    h = wyhash::wyhash(&arg, sizeof(arg), h, wyhash::_wyp);
  };
  ((wy(val)), ...);
  return h;
}

inline hash_t hash(std::string_view s, hash_t h = BASE_HASH) {
  return wyhash::wyhash(s.data(), s.size(), h, wyhash::_wyp);
}

template <size_t N>
constexpr hash_t hash(const char (&str)[N], hash_t const h = BASE_HASH) {
  return wyhash::wyhash(str, N - 1, h, wyhash::_wyp);
}

template <typename T>
constexpr uint64_t hash(T const& buf, hash_t const h = BASE_HASH) {
  return buf.size() == 0 ? h
                         : wyhash::wyhash(&buf[0], buf.size(), h, wyhash::_wyp);
}

#elif defined(CISTA_WYHASH_FASTEST)

#include "wyhash.h"

using hash_t = uint64_t;

constexpr auto const BASE_HASH = 123ULL;

template <typename... Args>
constexpr hash_t hash_combine(hash_t h, Args... val) {
  auto fh = [&](auto const& arg) {
    h = wyhash::FastestHash(&arg, sizeof(arg), h);
  };
  ((fh(val)), ...);
  return h;
}

inline hash_t hash(std::string_view s, hash_t h = BASE_HASH) {
  return wyhash::FastestHash(s.data(), s.size(), h);
}

template <size_t N>
constexpr hash_t hash(const char (&str)[N], hash_t const h = BASE_HASH) {
  return wyhash::FastestHash(str, N - 1, h);
}

template <typename T>
constexpr uint64_t hash(T const& buf, hash_t const h = BASE_HASH) {
  return buf.size() == 0 ? h : wyhash::FastestHash(&buf[0], buf.size(), h);
}

#else  // defined(CISTA_FNV1A)

// Algorithm: 64bit FNV-1a
// Source: http://www.isthe.com/chongo/tech/comp/fnv/

using hash_t = std::uint64_t;

constexpr auto const BASE_HASH = 14695981039346656037ULL;

template <typename... Args>
constexpr hash_t hash_combine(hash_t h, Args... val) noexcept {
  constexpr hash_t fnv_prime = 1099511628211ULL;
  auto fnv = [&](auto arg) { h = (h ^ static_cast<hash_t>(arg)) * fnv_prime; };
  ((fnv(val)), ...);
  return h;
}

constexpr hash_t hash(std::string_view s, hash_t h = BASE_HASH) noexcept {
  auto const ptr = s.data();
  for (auto i = size_t{0ULL}; i < s.size(); ++i) {
    h = hash_combine(h, static_cast<uint8_t>(ptr[i]));
  }
  return h;
}

template <size_t N>
constexpr hash_t hash(const char (&str)[N],
                      hash_t const h = BASE_HASH) noexcept {
  return hash(std::string_view{str, N - 1}, h);
}

template <typename T>
constexpr uint64_t hash(T const& buf, hash_t const h = BASE_HASH) noexcept {
  return buf.size() == 0
             ? h
             : hash(std::string_view{reinterpret_cast<char const*>(&buf[0]),
                                     buf.size()},
                    h);
}

#endif

}  // namespace cista
