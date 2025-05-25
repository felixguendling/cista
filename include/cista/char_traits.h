#pragma once

#include <ios>

namespace cista {

template <typename CharT>
struct generic_char_traits {
  using char_type = CharT;
  using int_type = int;
  using off_type = std::streamoff;
  using pos_type = std::streampos;
  using state_type = std::mbstate_t;

  static inline void assign(char_type& a, char_type const& b) noexcept {
    a = b;
  }

  static inline bool eq(char_type const a, char_type const b) { return a == b; }
  static inline bool lt(char_type const a, char_type const b) { return a < b; }

  static inline int compare(char_type const* a, char_type const* b,
                            std::size_t size) {
    for (; size; --size, ++a, ++b) {
      if (lt(*a, *b)) {
        return -1;
      }
      if (lt(*b, *a)) {
        return 1;
      }
    }
    return 0;
  }

  static inline size_t length(const char_type* data) {
    auto length = std::size_t{0U};
    for (; !eq(*data, char_type{0}); ++data) {
      ++length;
    }
    return length;
  }

  static inline char_type const* find(char_type const* data, std::size_t size,
                                      char_type const& val) {
    for (; size; --size) {
      if (eq(*data, val)) {
        return data;
      }
      ++data;
    }
    return nullptr;
  }

  static inline char_type* move(char_type* dst, char_type const* src,
                                std::size_t size) {
    if (size == 0) {
      return dst;
    }

    char_type* beginning{dst};

    if (dst < src) {
      for (; size; --size, ++dst, ++src) {
        assign(*dst, *src);
      }
    } else if (src < dst) {
      dst += size;
      src += size;

      for (; size; --size) {
        assign(*--dst, *--src);
      }
    }

    return beginning;
  }

  static inline char_type* copy(char_type* dst, char_type const* src,
                                std::size_t size) {
    char_type* beginning{dst};

    for (; size; --size, ++dst, ++src) {
      assign(*dst, *src);
    }

    return beginning;
  }

  static inline char_type* assign(char_type* dst, std::size_t size,
                                  char_type const value) {
    char_type* beginning{dst};

    for (; size; --size, ++dst) {
      assign(*dst, value);
    }

    return beginning;
  }

  static inline int_type not_eof(int_type const value) {
    return eq_int_type(value, eof()) ? ~eof() : value;
  }

  static inline char_type to_char_type(int_type const value) {
    return char_type(value);
  }

  static inline int_type to_int_type(char_type const value) {
    return int_type(value);
  }

  static inline bool eq_int_type(int_type const a, int_type const b) {
    return a == b;
  }

  static inline int_type eof() { return static_cast<int_type>(EOF); }
};

template <typename CharT>
struct gen_char_traits_helper {
  using value_type = generic_char_traits<CharT>;
};

template <typename CharT>
struct std_char_traits_helper {
  using value_type = std::char_traits<CharT>;
};

template <typename CharT>
using char_traits = typename std::conditional_t<
    std::disjunction_v<std::is_same<CharT, char>, std::is_same<CharT, wchar_t>,
#if defined(__cpp_lib_char8_t)
                       std::is_same<CharT, char8_t>,
#endif
                       std::is_same<CharT, char16_t>,
                       std::is_same<CharT, char32_t>>,
    std_char_traits_helper<CharT>, gen_char_traits_helper<CharT>>::value_type;

}  // namespace cista
