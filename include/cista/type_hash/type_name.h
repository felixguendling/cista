#pragma once

// Credits: Manu Sánchez (@Manu343726)
// https://github.com/Manu343726/ctti/blob/master/include/ctti/detail/pretty_function.hpp

#include <string>
#include <string_view>

namespace cista {

#if defined(_MSC_VER)
#define CISTA_SIG __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define CISTA_SIG __PRETTY_FUNCTION__
#else
#error unsupported compiler
#endif

inline void remove_all(std::string& s, std::string_view substr) {
  auto pos = std::size_t{};
  while ((pos = s.find(substr, pos)) != std::string::npos) {
    s.erase(pos, substr.length());
  }
}

inline void canonicalize_type_name(std::string& s) {
  remove_all(s, "{anonymous}::");  // GCC
  remove_all(s, "(anonymous namespace)::");  // Clang
  remove_all(s, "`anonymous-namespace'::");  // MSVC
  remove_all(s, "struct");  // MSVC "struct my_struct" vs "my_struct"
  remove_all(s, "const");  // MSVC "char const*"" vs "const char*"
  remove_all(s, " ");  // MSVC
}

template <typename T>
constexpr std::string_view type_str() {
#if defined(__clang__)
  constexpr std::string_view prefix =
      "std::string_view cista::type_str() [T = ";
  constexpr std::string_view suffix = "]";
#elif defined(_MSC_VER)
  constexpr std::string_view prefix =
      "class std::basic_string_view<char,struct std::char_traits<char> > "
      "__cdecl cista::type_str<";
  constexpr std::string_view suffix = ">(void)";
#else
  constexpr std::string_view prefix =
      "constexpr std::string_view cista::type_str() [with T = ";
  constexpr std::string_view suffix =
      "; std::string_view = std::basic_string_view<char>]";
#endif

  auto sig = std::string_view{CISTA_SIG};
  sig.remove_prefix(prefix.size());
  sig.remove_suffix(suffix.size());
  return sig;
}

template <typename T>
std::string canonical_type_str() {
  auto const base = type_str<T>();
  std::string s{base.data(), base.length()};
  canonicalize_type_name(s);
  return s;
}

}  // namespace cista

#undef CISTA_SIG
