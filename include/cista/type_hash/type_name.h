#pragma once

// Credits: Manu Sánchez (@Manu343726)
// https://github.com/Manu343726/ctti/blob/master/include/ctti/detail/pretty_function.hpp

#include <string>
#include <string_view>

#if defined(_MSC_VER)
#define CISTA_SIG __FUNCSIG__
#else  // Assume GCC/Clang
#define CISTA_SIG __PRETTY_FUNCTION__
#endif

namespace cista {

inline void canonicalize_type_name(std::string& s) {
  constexpr std::string_view const struct_str = "struct ";

  auto pos = std::size_t{};
  while ((pos = s.find(struct_str, pos)) != std::string::npos) {
    s.erase(pos, struct_str.length());
  }

  for (pos = s.find(','); pos != std::string::npos;
       pos = s.find(',', pos + 1)) {
    if (pos >= s.length() - 1 || s[pos + 1] == ' ') {
      continue;
    }
    s.insert(size_t{pos + 1}, 1, ' ');
  }
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
      "std::string_view cista::type_str() [with T = ";
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
  constexpr auto const base = type_str<T>();
  std::string s{base.data(), base.length()};
  canonicalize_type_name(s);
  return s;
}

}  // namespace cista

#undef CISTA_SIG