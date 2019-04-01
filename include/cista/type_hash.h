// Credits:
// nameof by Daniil Goncharov (@Neargye): https://github.com/Neargye/nameof
// hash by Jonathan Müller (@foonathan): https://github.com/foonathan/string_id

#pragma once

#include <cstddef>
#include <limits>
#include <string_view>
#include <type_traits>

#include "cista/decay.h"
#include "cista/reflection/for_each_field.h"

#define CISTA_NAMEOF_ENUM_MAX_SEARCH_DEPTH 128

namespace cista {

using hash_t = std::uint64_t;

namespace detail {

template <typename T>
struct identity final {
  using type = T;
};

constexpr bool is_name_char(char c) noexcept {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr bool is_bracket_char(char c) noexcept {
  return c == ')' || c == '}' || c == '>' || c == '(' || c == '{' || c == '<';
}

constexpr std::string_view pretty_name(std::string_view name,
                                       bool with_suffix) noexcept {
  for (std::size_t i = name.size(), h = 0, s = 0; i > 0; --i) {
    if (h == 0 && !is_name_char(name[i - 1]) && !is_bracket_char(name[i - 1])) {
      ++s;
      continue;
    }

    if (name[i - 1] == ')' || name[i - 1] == '}') {
      ++h;
      ++s;
      continue;
    } else if (name[i - 1] == '(' || name[i - 1] == '{') {
      --h;
      ++s;
      continue;
    }

    if (h == 0) {
      name.remove_suffix(s);
      break;
    } else {
      ++s;
      continue;
    }
  }

  std::size_t s = 0;
  for (std::size_t i = name.size(), h = 0; i > 0; --i) {
    if (name[i - 1] == '>') {
      ++h;
      ++s;
      continue;
    } else if (name[i - 1] == '<') {
      --h;
      ++s;
      continue;
    }

    if (h == 0) {
      break;
    } else {
      ++s;
      continue;
    }
  }

  for (std::size_t i = name.size() - s; i > 0; --i) {
    if (!is_name_char(name[i - 1])) {
      name.remove_prefix(i);
      break;
    }
  }
  name.remove_suffix(with_suffix ? 0 : s);

  return name;
}

template <typename T>
constexpr std::string_view nameof_type_impl() noexcept {
#if defined(__clang__)
  std::string_view name{__PRETTY_FUNCTION__};
  constexpr auto prefix =
      sizeof(
          "std::string_view nameof::detail::nameof_type_impl() [T = "
          "nameof::detail::identity<") -
      1;
  constexpr auto suffix = sizeof(">]") - 1;
#elif defined(__GNUC__)
  std::string_view name{__PRETTY_FUNCTION__};
  constexpr auto prefix =
      sizeof(
          "constexpr std::string_view nameof::detail::nameof_type_impl() [with "
          "T = nameof::detail::identity<") -
      1;
  constexpr auto suffix =
      sizeof(">; std::string_view = std::basic_string_view<char>]") - 1;
#elif defined(_MSC_VER)
  std::string_view name{__FUNCSIG__};
  constexpr auto prefix =
      sizeof(
          "class std::basic_string_view<char,struct std::char_traits<char> > "
          "__cdecl nameof::detail::nameof_type_impl<struct "
          "nameof::detail::identity<") -
      1;
  constexpr auto suffix = sizeof(">>(void) noexcept") - 1;
#else
  return {};  // Unsupported compiler.
#endif

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
  name.remove_prefix(prefix);
  name.remove_suffix(suffix);
#if defined(_MSC_VER)
  if (name.size() > sizeof("enum") && name[0] == 'e' && name[1] == 'n' &&
      name[2] == 'u' && name[3] == 'm' && name[4] == ' ') {
    name.remove_prefix(sizeof("enum"));
  }
  if (name.size() > sizeof("class") && name[0] == 'c' && name[1] == 'l' &&
      name[2] == 'a' && name[3] == 's' && name[4] == 's' && name[5] == ' ') {
    name.remove_prefix(sizeof("class"));
  }
  if (name.size() > sizeof("struct") && name[0] == 's' && name[1] == 't' &&
      name[2] == 'r' && name[3] == 'u' && name[4] == 'c' && name[5] == 't' &&
      name[6] == ' ') {
    name.remove_prefix(sizeof("struct"));
  }
#endif
  while (name.back() == ' ') {
    name.remove_suffix(1);
  }

  return name;
#endif
}

template <auto V>
constexpr std::string_view nameof_enum_impl() noexcept {
  static_assert(std::is_enum_v<decltype(V)>);
#if defined(__clang__)
  std::string_view name{__PRETTY_FUNCTION__};
  constexpr auto suffix = sizeof("]") - 1;
#elif defined(__GNUC__) && __GNUC__ >= 9
  std::string_view name{__PRETTY_FUNCTION__};
  constexpr auto suffix =
      sizeof("; std::string_view = std::basic_string_view<char>]") - 1;
#elif defined(_MSC_VER)
  std::string_view name{__FUNCSIG__};
  constexpr auto suffix = sizeof(">(void) noexcept") - 1;
#else
  return {};  // Unsupported compiler.
#endif

#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 9) || \
    defined(_MSC_VER)
  name.remove_suffix(suffix);
  for (std::size_t i = name.size(); i > 0; --i) {
    if (!is_name_char(name[i - 1])) {
      name.remove_prefix(i);
      break;
    }
  }

  if (name.front() >= '0' && name.front() <= '9') {
    return {};  // Enum variable does not have name.
  } else {
    return name;
  }
#endif
}

template <typename E, int V>
struct nameof_enum_impl_t final {
  constexpr std::string_view operator()(int value) const noexcept {
    static_assert(std::is_enum_v<E>);
    if constexpr (V > std::numeric_limits<std::underlying_type_t<E>>::max()) {
      return {};  // Enum variable out of range.
    }

    switch (value - V) {
      case 0: return nameof_enum_impl<static_cast<E>(V)>();
      case 1: return nameof_enum_impl<static_cast<E>(V + 1)>();
      case 2: return nameof_enum_impl<static_cast<E>(V + 2)>();
      case 3: return nameof_enum_impl<static_cast<E>(V + 3)>();
      case 4: return nameof_enum_impl<static_cast<E>(V + 4)>();
      case 5: return nameof_enum_impl<static_cast<E>(V + 5)>();
      case 6: return nameof_enum_impl<static_cast<E>(V + 6)>();
      case 7: return nameof_enum_impl<static_cast<E>(V + 7)>();
      default: return nameof_enum_impl_t<E, V + 8>{}(value);
    }
  }
};

template <typename E>
struct nameof_enum_impl_t<E, CISTA_NAMEOF_ENUM_MAX_SEARCH_DEPTH> final {
  constexpr std::string_view operator()(int) const noexcept {
    static_assert(std::is_enum_v<E>);
    return {};  // Enum variable out of range
                // CISTA_NAMEOF_ENUM_MAX_SEARCH_DEPTH.
  }
};

template <typename T, typename = std::enable_if_t<!std::is_reference_v<T>>>
constexpr std::string_view nameof_impl(std::string_view name,
                                       bool with_suffix) noexcept {
  return pretty_name(name, with_suffix);
}

constexpr std::string_view nameof_raw_impl(std::string_view name) noexcept {
  return name;
}

// nameof_enum used to obtain the simple (unqualified) string enum name of enum
// variable.
template <typename T,
          typename = std::enable_if_t<std::is_enum_v<std::decay_t<T>>>>
constexpr std::string_view nameof_enum(T value) noexcept {
  constexpr bool s = std::is_signed_v<std::underlying_type_t<std::decay_t<T>>>;
  constexpr int min = s ? -CISTA_NAMEOF_ENUM_MAX_SEARCH_DEPTH : 0;
  return detail::nameof_enum_impl_t<std::decay_t<T>, min>{}(
      static_cast<int>(value));
}

// nameof_enum used to obtain the simple (unqualified) string enum name of
// static storage enum variable.
template <auto V, typename = std::enable_if_t<
                      std::is_enum_v<std::decay_t<decltype(V)>>>>
constexpr std::string_view nameof_enum() noexcept {
  return detail::nameof_enum_impl<V>();
}

// nameof_type used to obtain the string name of type.
template <typename T>
constexpr std::string_view nameof_type() noexcept {
  return detail::nameof_type_impl<detail::identity<T>>();
}

}  // namespace detail

// See http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
template <typename T>
constexpr hash_t hash_combine(hash_t const hash, T const& val) {
  constexpr hash_t prime = 1099511628211ull;
  return (hash ^ val) * prime;
}

constexpr hash_t fnv1a_hash(std::string_view s, hash_t hash) noexcept {
  return s.empty() ? hash : fnv1a_hash(s.substr(1), hash_combine(hash, s[0]));
}

constexpr hash_t fnv1a_hash(std::string_view s = "") {
  constexpr hash_t basis = 14695981039346656037ull;
  return fnv1a_hash(s, basis);
}

}  // namespace cista