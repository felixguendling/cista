#pragma once

#include <cinttypes>
#include <cstring>

#include <ostream>
#include <string>
#include <string_view>

#include "cista/containers/ptr.h"
#include "cista/exception.h"
#include "cista/type_traits.h"

namespace cista {

template <typename Ptr = char const*>
struct generic_string {
  using CharT = typename std::remove_const_t<remove_pointer_t<Ptr>>;
  using msize_t = std::uint32_t;
  using value_type = CharT;

  static msize_t mstrlen(CharT const* s) noexcept {
    return static_cast<msize_t>(std::char_traits<CharT>::length(s));
  }

  static constexpr struct owning_t {
  } owning{};
  static constexpr struct non_owning_t {
  } non_owning{};

  constexpr generic_string() noexcept {}
  ~generic_string() noexcept { reset(); }

  generic_string(std::basic_string_view<CharT> s, owning_t const) {
    set_owning(s);
  }
  generic_string(std::basic_string_view<CharT> s, non_owning_t const) {
    set_non_owning(s);
  }
  generic_string(std::basic_string<CharT> const& s, owning_t const) {
    set_owning(s);
  }
  generic_string(std::basic_string<CharT> const& s, non_owning_t const) {
    set_non_owning(s);
  }
  generic_string(CharT const* s, owning_t const) { set_owning(s, mstrlen(s)); }
  generic_string(CharT const* s, non_owning_t const) { set_non_owning(s); }
  generic_string(CharT const* s, msize_t const len, owning_t const) {
    set_owning(s, len);
  }
  generic_string(CharT const* s, msize_t const len, non_owning_t const) {
    set_non_owning(s, len);
  }

  generic_string(generic_string&& o) { move_from(std::move(o)); }
  generic_string(generic_string const& o) { copy_from(o); }

  generic_string& operator=(generic_string&& o) {
    move_from(std::move(o));
    return *this;
  }
  generic_string& operator=(generic_string const& o) {
    copy_from(o);
    return *this;
  }

  CharT* begin() noexcept { return data(); }
  CharT* end() noexcept { return data() + size(); }
  CharT const* begin() const noexcept { return data(); }
  CharT const* end() const noexcept { return data() + size(); }

  friend CharT const* begin(generic_string const& s) { return s.begin(); }
  friend CharT* begin(generic_string& s) { return s.begin(); }
  friend CharT const* end(generic_string const& s) { return s.end(); }
  friend CharT* end(generic_string& s) { return s.end(); }

  bool is_short() const noexcept { return s_.is_short_; }

  void reset() noexcept {
    if (!h_.is_short_ && h_.ptr_ != nullptr && h_.self_allocated_) {
      std::free(data());
    }
    h_ = heap{};
  }

  void set_owning(std::basic_string<CharT> const& s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(std::basic_string_view<CharT> s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(CharT const* str) { set_owning(str, mstrlen(str)); }

  static constexpr msize_t short_length_limit = 15U / sizeof(CharT);

  void set_owning(CharT const* str, msize_t const len) {
    reset();
    if (str == nullptr || len == 0U) {
      return;
    }
    s_.is_short_ = (len <= short_length_limit);
    if (s_.is_short_) {
      std::memcpy(s_.s_, str, len * sizeof(CharT));
      for (auto i = len; i < short_length_limit; ++i) {
        s_.s_[i] = 0;
      }
    } else {
      h_.ptr_ = static_cast<CharT*>(std::malloc(len * sizeof(CharT)));
      if (h_.ptr_ == nullptr) {
        throw_exception(std::bad_alloc{});
      }
      h_.size_ = len;
      h_.self_allocated_ = true;
      std::memcpy(data(), str, len * sizeof(CharT));
    }
  }

  void set_non_owning(std::basic_string<CharT> const& v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(std::basic_string_view<CharT> v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(CharT const* str) { set_non_owning(str, mstrlen(str)); }

  void set_non_owning(CharT const* str, msize_t const len) {
    reset();
    if (str == nullptr || len == 0U) {
      return;
    }

    if (len <= short_length_limit) {
      return set_owning(str, len);
    }

    h_.is_short_ = false;
    h_.self_allocated_ = false;
    h_.ptr_ = str;
    h_.size_ = len;
  }

  void move_from(generic_string&& s) noexcept {
    if (&s == this) {
      return;
    }
    reset();
    std::memcpy(static_cast<void*>(this), &s, sizeof(*this));
    if constexpr (std::is_pointer_v<Ptr>) {
      std::memset(static_cast<void*>(&s), 0, sizeof(*this));
    } else {
      if (!s.is_short()) {
        h_.ptr_ = s.h_.ptr_;
        s.h_.ptr_ = nullptr;
        s.h_.size_ = 0U;
      }
    }
  }

  void copy_from(generic_string const& s) {
    if (&s == this) {
      return;
    }
    reset();
    if (s.is_short()) {
      std::memcpy(static_cast<void*>(this), &s, sizeof(s));
    } else if (s.h_.self_allocated_) {
      set_owning(s.data(), s.size());
    } else {
      set_non_owning(s.data(), s.size());
    }
  }

  bool empty() const noexcept { return size() == 0U; }
  std::basic_string_view<CharT> view() const noexcept {
    return {data(), size()};
  }
  std::basic_string<CharT> str() const { return {data(), size()}; }

  operator std::basic_string_view<CharT>() const { return view(); }

  CharT& operator[](std::size_t const i) noexcept { return data()[i]; }
  CharT const& operator[](std::size_t const i) const noexcept {
    return data()[i];
  }

  friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& out,
                                               generic_string const& s) {
    return out << s.view();
  }

  friend bool operator==(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() == b.view();
  }

  friend bool operator!=(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() != b.view();
  }

  friend bool operator<(generic_string const& a,
                        generic_string const& b) noexcept {
    return a.view() < b.view();
  }

  friend bool operator>(generic_string const& a,
                        generic_string const& b) noexcept {
    return a.view() > b.view();
  }

  friend bool operator<=(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() <= b.view();
  }

  friend bool operator>=(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() >= b.view();
  }

  friend bool operator==(generic_string const& a,
                         std::basic_string_view<CharT> b) noexcept {
    return a.view() == b;
  }

  friend bool operator!=(generic_string const& a,
                         std::basic_string_view<CharT> b) noexcept {
    return a.view() != b;
  }

  friend bool operator<(generic_string const& a,
                        std::basic_string_view<CharT> b) noexcept {
    return a.view() < b;
  }

  friend bool operator>(generic_string const& a,
                        std::basic_string_view<CharT> b) noexcept {
    return a.view() > b;
  }

  friend bool operator<=(generic_string const& a,
                         std::basic_string_view<CharT> b) noexcept {
    return a.view() <= b;
  }

  friend bool operator>=(generic_string const& a,
                         std::basic_string_view<CharT> b) noexcept {
    return a.view() >= b;
  }

  friend bool operator==(std::basic_string_view<CharT> a,
                         generic_string const& b) noexcept {
    return a == b.view();
  }

  friend bool operator!=(std::basic_string_view<CharT> a,
                         generic_string const& b) noexcept {
    return a != b.view();
  }

  friend bool operator<(std::basic_string_view<CharT> a,
                        generic_string const& b) noexcept {
    return a < b.view();
  }

  friend bool operator>(std::basic_string_view<CharT> a,
                        generic_string const& b) noexcept {
    return a > b.view();
  }

  friend bool operator<=(std::basic_string_view<CharT> a,
                         generic_string const& b) noexcept {
    return a <= b.view();
  }

  friend bool operator>=(std::basic_string_view<CharT> a,
                         generic_string const& b) noexcept {
    return a >= b.view();
  }

  friend bool operator==(generic_string const& a, CharT const* b) noexcept {
    return a.view() == std::basic_string_view<CharT>{b};
  }

  friend bool operator!=(generic_string const& a, CharT const* b) noexcept {
    return a.view() != std::basic_string_view<CharT>{b};
  }

  friend bool operator<(generic_string const& a, CharT const* b) noexcept {
    return a.view() < std::basic_string_view<CharT>{b};
  }

  friend bool operator>(generic_string const& a, CharT const* b) noexcept {
    return a.view() > std::basic_string_view<CharT>{b};
  }

  friend bool operator<=(generic_string const& a, CharT const* b) noexcept {
    return a.view() <= std::basic_string_view<CharT>{b};
  }

  friend bool operator>=(generic_string const& a, CharT const* b) noexcept {
    return a.view() >= std::basic_string_view<CharT>{b};
  }

  friend bool operator==(CharT const* a, generic_string const& b) noexcept {
    return std::basic_string_view<CharT>{a} == b.view();
  }

  friend bool operator!=(CharT const* a, generic_string const& b) noexcept {
    return std::basic_string_view<CharT>{a} != b.view();
  }

  friend bool operator<(CharT const* a, generic_string const& b) noexcept {
    return std::basic_string_view<CharT>{a} < b.view();
  }

  friend bool operator>(CharT const* a, generic_string const& b) noexcept {
    return std::basic_string_view<CharT>{a} > b.view();
  }

  friend bool operator<=(CharT const* a, generic_string const& b) noexcept {
    return std::basic_string_view<CharT>{a} <= b.view();
  }

  friend bool operator>=(CharT const* a, generic_string const& b) noexcept {
    return std::basic_string_view<CharT>{a} >= b.view();
  }

  CharT const* internal_data() const noexcept {
    if constexpr (std::is_pointer_v<Ptr>) {
      return is_short() ? s_.s_ : h_.ptr_;
    } else {
      return is_short() ? s_.s_ : h_.ptr_.get();
    }
  }

  CharT* data() noexcept { return const_cast<CharT*>(internal_data()); }
  CharT const* data() const noexcept { return internal_data(); }

  msize_t size() const noexcept {
    if (is_short()) {
      auto const pos =
          std::char_traits<CharT>::find(s_.s_, short_length_limit, CharT(0));
      return (pos != nullptr) ? static_cast<msize_t>(pos - s_.s_)
                              : short_length_limit;
    }
    return h_.size_;
  }

  generic_string& erase(msize_t const pos, msize_t const n) {
    if (!is_short() && !h_.self_allocated_) {
      set_owning(view());
    }
    auto const size_before = size();
    std::memmove(data() + pos, data() + pos + n,
                 (size_before - (pos + n)) * sizeof(CharT));
    std::memset(data() + size_before - n, 0U, n * sizeof(CharT));
    if (!is_short()) {
      h_.size_ = size_before - n;
    }
    return *this;
  }

  constexpr bool starts_with(generic_string const& s) const noexcept {
    return starts_with(s.data(), static_cast<msize_t>(s.size()));
  }
  constexpr bool starts_with(
      std::basic_string_view<CharT> const& sv) const noexcept {
    return starts_with(sv.data(), static_cast<msize_t>(sv.size()));
  }
  constexpr bool starts_with(CharT const* s) const noexcept {
    return starts_with(s, mstrlen(s));
  }
  constexpr bool starts_with(CharT const* s, msize_t size_s) const noexcept {
    if (size_s > size()) {
      return false;
    }
    if (size_s == 0) {
      return true;
    }
    if (empty()) {
      return false;
    }
    return !std::memcmp(s, data(), size_s * sizeof(CharT));
  }
  constexpr bool starts_with(CharT ch) const noexcept {
    if (empty()) {
      return false;
    }
    return data()[0] == ch;
  }

  constexpr bool ends_with(generic_string const& s) const noexcept {
    return ends_with(s.data(), static_cast<msize_t>(s.size()));
  }
  constexpr bool ends_with(
      std::basic_string_view<CharT> const& sv) const noexcept {
    return ends_with(sv.data(), static_cast<msize_t>(sv.size()));
  }
  constexpr bool ends_with(CharT const* s) const noexcept {
    return ends_with(s, mstrlen(s));
  }
  constexpr bool ends_with(CharT const* s, msize_t size_s) const noexcept {
    if (size_s > size()) {
      return false;
    }
    if (size_s == 0) {
      return true;
    }
    if (empty()) {
      return false;
    }
    return !std::memcmp(s, data() + size() - size_s, size_s * sizeof(CharT));
  }
  constexpr bool ends_with(CharT ch) const noexcept {
    if (size() == 0) {
      return false;
    }
    return data()[size() - 1] == ch;
  }

  struct heap {
    bool is_short_{false};
    bool self_allocated_{false};
    std::uint16_t __fill__{0};
    std::uint32_t size_{0};
    Ptr ptr_{nullptr};
  };

  struct stack {
    union {
      bool is_short_{true};
      CharT __fill__;
    };
    CharT s_[short_length_limit]{0};
  };

  union {
    heap h_{};
    stack s_;
  };
};

template <typename Ptr>
struct basic_string : public generic_string<Ptr> {
  using base = generic_string<Ptr>;
  using CharT = typename base::CharT;

  using base::base;
  operator std::basic_string_view<CharT>() const { return base::view(); }

  friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& out,
                                               basic_string const& s) {
    return out << s.view();
  }

  explicit operator std::basic_string<CharT>() const {
    return {base::data(), base::size()};
  }

  basic_string(std::basic_string_view<CharT> s) : base{s, base::owning} {}
  basic_string(std::basic_string<CharT> const& s) : base{s, base::owning} {}
  basic_string(CharT const* s) : base{s, base::owning} {}
  basic_string(CharT const* s, typename base::msize_t const len)
      : base{s, len, base::owning} {}

  basic_string(basic_string const& o) : base{o.view(), base::owning} {}
  basic_string(basic_string&& o) { base::move_from(std::move(o)); }

  basic_string& operator=(basic_string const& o) {
    if (&o == this) {
      return *this;
    }
    base::set_owning(o.data(), o.size());
    return *this;
  }

  basic_string& operator=(basic_string&& o) {
    base::move_from(std::move(o));
    return *this;
  }

  basic_string& operator=(CharT const* s) {
    base::set_owning(s);
    return *this;
  }
  basic_string& operator=(std::basic_string<CharT> const& s) {
    base::set_owning(s);
    return *this;
  }
  basic_string& operator=(std::basic_string_view<CharT> s) {
    base::set_owning(s);
    return *this;
  }
};

template <typename Ptr>
struct basic_string_view : public generic_string<Ptr> {
  using base = generic_string<Ptr>;
  using CharT = typename base::CharT;

  using base::base;
  operator std::basic_string_view<CharT>() const { return base::view(); }

  friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& out,
                                               basic_string_view const& s) {
    return out << s.view();
  }

  basic_string_view(std::basic_string_view<CharT> s)
      : base{s, base::non_owning} {}
  basic_string_view(std::basic_string<CharT> const& s)
      : base{s, base::non_owning} {}
  basic_string_view(CharT const* s) : base{s, base::non_owning} {}
  basic_string_view(CharT const* s, typename base::msize_t const len)
      : base{s, len, base::non_owning} {}

  basic_string_view(basic_string_view const& o) {
    base::set_non_owning(o.data(), o.size());
  }
  basic_string_view(basic_string_view&& o) {
    base::set_non_owning(o.data(), o.size());
  }
  basic_string_view& operator=(basic_string_view const& o) {
    base::set_non_owning(o.data(), o.size());
    return *this;
  }
  basic_string_view& operator=(basic_string_view&& o) {
    base::set_non_owning(o.data(), o.size());
    return *this;
  }

  basic_string_view& operator=(CharT const* s) {
    base::set_non_owning(s);
    return *this;
  }
  basic_string_view& operator=(std::basic_string_view<CharT> s) {
    base::set_non_owning(s);
    return *this;
  }
  basic_string_view& operator=(std::basic_string<CharT> const& s) {
    base::set_non_owning(s);
    return *this;
  }
};

template <typename Ptr>
struct is_string_helper<generic_string<Ptr>> : std::true_type {};

template <typename Ptr>
struct is_string_helper<basic_string<Ptr>> : std::true_type {};

template <typename Ptr>
struct is_string_helper<basic_string_view<Ptr>> : std::true_type {};

namespace raw {
using generic_string = generic_string<ptr<char const>>;
using generic_u16string = cista::generic_string<ptr<char16_t const>>;
using generic_u32string = cista::generic_string<ptr<char32_t const>>;

using string = basic_string<ptr<char const>>;
using u16string = basic_string<ptr<char16_t const>>;
using u32string = basic_string<ptr<char32_t const>>;

using string_view = basic_string_view<ptr<char const>>;
using u16string_view = basic_string_view<ptr<char16_t const>>;
using u32string_view = basic_string_view<ptr<char32_t const>>;

#ifdef __cpp_char8_t
using generic_u8string = cista::generic_string<ptr<char8_t const>>;
using u8string = basic_string<ptr<char8_t const>>;
using u8string_view = basic_string_view<ptr<char8_t const>>;
#endif
}  // namespace raw

namespace offset {
using generic_string = generic_string<ptr<char const>>;
using generic_u16string = cista::generic_string<ptr<char16_t const>>;
using generic_u32string = cista::generic_string<ptr<char32_t const>>;

using string = basic_string<ptr<char const>>;
using u16string = basic_string<ptr<char16_t const>>;
using u32string = basic_string<ptr<char32_t const>>;

using string_view = basic_string_view<ptr<char const>>;
using u16string_view = basic_string_view<ptr<char16_t const>>;
using u32string_view = basic_string_view<ptr<char32_t const>>;

#ifdef __cpp_char8_t
using generic_u8string = cista::generic_string<ptr<char8_t const>>;
using u8string = basic_string<ptr<char8_t const>>;
using u8string_view = basic_string_view<ptr<char8_t const>>;
#endif
}  // namespace offset

template <typename Ptr>
auto format_as(cista::basic_string<Ptr> const& s) {
  return s.view();
}

}  // namespace cista

#if defined(CISTA_FMT)

#include "fmt/ranges.h"

namespace fmt {
template <typename Ptr, typename Char>
struct range_format_kind<cista::basic_string<Ptr>, Char, void>
    : std::false_type {};
}  // namespace fmt

#endif