#pragma once

#include <cassert>
#include <cinttypes>
#include <cstring>

#include <ostream>
#include <string>
#include <string_view>

#include "cista/containers/ptr.h"
#include "cista/exception.h"
#include "cista/type_traits.h"

namespace cista {

// This class is a generic string container that stores an extra \0 byte post
// the last byte of the valid data. This makes sure the pointer returned by
// data() can be passed as a C-string.
//
// The content stored within this container can contain binary data, that is,
// any number of \0 bytes is permitted within [data(), data() + size()).
template <typename Ptr = char const*>
struct generic_cstring {
  using msize_t = std::uint32_t;
  using value_type = char;

  static msize_t mstrlen(char const* s) noexcept {
    return static_cast<msize_t>(std::strlen(s));
  }

  static constexpr struct owning_t {
  } owning{};
  static constexpr struct non_owning_t {
  } non_owning{};

  constexpr generic_cstring() noexcept {}
  ~generic_cstring() noexcept { reset(); }

  generic_cstring(std::string_view s, owning_t const) { set_owning(s); }
  generic_cstring(std::string_view s, non_owning_t const) { set_non_owning(s); }
  generic_cstring(std::string const& s, owning_t const) { set_owning(s); }
  generic_cstring(std::string const& s, non_owning_t const) {
    set_non_owning(s);
  }
  generic_cstring(char const* s, owning_t const) { set_owning(s); }
  generic_cstring(char const* s, non_owning_t const) { set_non_owning(s); }

  char* begin() noexcept { return data(); }
  char* end() noexcept { return data() + size(); }
  char const* begin() const noexcept { return data(); }
  char const* end() const noexcept { return data() + size(); }

  friend char const* begin(generic_cstring const& s) { return s.begin(); }
  friend char* begin(generic_cstring& s) { return s.begin(); }
  friend char const* end(generic_cstring const& s) { return s.end(); }
  friend char* end(generic_cstring& s) { return s.end(); }

  bool is_short() const noexcept { return s_.remaining_ >= 0; }

  bool is_owning() const { return is_short() || h_.self_allocated_; }

  void reset() noexcept {
    if (!is_short() && h_.self_allocated_) {
      std::free(data());
    }
    s_ = stack{};
  }

  void set_owning(std::string const& s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(std::string_view s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(char const* str) {
    assert(str != nullptr);
    set_owning(str, mstrlen(str));
  }

  static constexpr msize_t short_length_limit = 15U;

  void set_owning(char const* str, msize_t const len) {
    assert(str != nullptr || len == 0U);
    reset();
    if (len == 0U) {
      return;
    }
    s_.remaining_ = static_cast<int8_t>(
        std::max(static_cast<int32_t>(short_length_limit - len), -1));
    if (is_short()) {
      std::memcpy(s_.s_, str, len);
    } else {
      h_ = heap(len, owning);
      std::memcpy(data(), str, len);
    }
  }

  void set_non_owning(std::string const& v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(std::string_view v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(char const* str) {
    set_non_owning(str, str != nullptr ? mstrlen(str) : 0);
  }

  void set_non_owning(char const* str, msize_t const len) {
    assert(str != nullptr || len == 0U);
    reset();
    h_ = heap(str, len, non_owning);
  }

  void move_from(generic_cstring&& s) noexcept {
    std::memcpy(static_cast<void*>(this), &s, sizeof(*this));
    if constexpr (std::is_pointer_v<Ptr>) {
      std::memset(static_cast<void*>(&s), 0, sizeof(*this));
    } else if (!s.is_short()) {
      h_.ptr_ = s.h_.ptr_;
      s.s_ = stack{};
    }
  }

  void copy_from(generic_cstring const& s) {
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
  std::string_view view() const noexcept { return {data(), size()}; }
  std::string str() const { return {data(), size()}; }

  operator std::string_view() const { return view(); }

  char& operator[](std::size_t const i) noexcept { return data()[i]; }
  char const& operator[](std::size_t const i) const noexcept {
    return data()[i];
  }

  friend std::ostream& operator<<(std::ostream& out, generic_cstring const& s) {
    return out << s.view();
  }

  friend bool operator==(generic_cstring const& a,
                         generic_cstring const& b) noexcept {
    return a.view() == b.view();
  }

  friend bool operator!=(generic_cstring const& a,
                         generic_cstring const& b) noexcept {
    return a.view() != b.view();
  }

  friend bool operator<(generic_cstring const& a,
                        generic_cstring const& b) noexcept {
    return a.view() < b.view();
  }

  friend bool operator>(generic_cstring const& a,
                        generic_cstring const& b) noexcept {
    return a.view() > b.view();
  }

  friend bool operator<=(generic_cstring const& a,
                         generic_cstring const& b) noexcept {
    return a.view() <= b.view();
  }

  friend bool operator>=(generic_cstring const& a,
                         generic_cstring const& b) noexcept {
    return a.view() >= b.view();
  }

  friend bool operator==(generic_cstring const& a,
                         std::string_view b) noexcept {
    return a.view() == b;
  }

  friend bool operator!=(generic_cstring const& a,
                         std::string_view b) noexcept {
    return a.view() != b;
  }

  friend bool operator<(generic_cstring const& a, std::string_view b) noexcept {
    return a.view() < b;
  }

  friend bool operator>(generic_cstring const& a, std::string_view b) noexcept {
    return a.view() > b;
  }

  friend bool operator<=(generic_cstring const& a,
                         std::string_view b) noexcept {
    return a.view() <= b;
  }

  friend bool operator>=(generic_cstring const& a,
                         std::string_view b) noexcept {
    return a.view() >= b;
  }

  friend bool operator==(std::string_view a,
                         generic_cstring const& b) noexcept {
    return a == b.view();
  }

  friend bool operator!=(std::string_view a,
                         generic_cstring const& b) noexcept {
    return a != b.view();
  }

  friend bool operator<(std::string_view a, generic_cstring const& b) noexcept {
    return a < b.view();
  }

  friend bool operator>(std::string_view a, generic_cstring const& b) noexcept {
    return a > b.view();
  }

  friend bool operator<=(std::string_view a,
                         generic_cstring const& b) noexcept {
    return a <= b.view();
  }

  friend bool operator>=(std::string_view a,
                         generic_cstring const& b) noexcept {
    return a >= b.view();
  }

  friend bool operator==(generic_cstring const& a, char const* b) noexcept {
    return a.view() == std::string_view{b};
  }

  friend bool operator!=(generic_cstring const& a, char const* b) noexcept {
    return a.view() != std::string_view{b};
  }

  friend bool operator<(generic_cstring const& a, char const* b) noexcept {
    return a.view() < std::string_view{b};
  }

  friend bool operator>(generic_cstring const& a, char const* b) noexcept {
    return a.view() > std::string_view{b};
  }

  friend bool operator<=(generic_cstring const& a, char const* b) noexcept {
    return a.view() <= std::string_view{b};
  }

  friend bool operator>=(generic_cstring const& a, char const* b) noexcept {
    return a.view() >= std::string_view{b};
  }

  friend bool operator==(char const* a, generic_cstring const& b) noexcept {
    return std::string_view{a} == b.view();
  }

  friend bool operator!=(char const* a, generic_cstring const& b) noexcept {
    return std::string_view{a} != b.view();
  }

  friend bool operator<(char const* a, generic_cstring const& b) noexcept {
    return std::string_view{a} < b.view();
  }

  friend bool operator>(char const* a, generic_cstring const& b) noexcept {
    return std::string_view{a} > b.view();
  }

  friend bool operator<=(char const* a, generic_cstring const& b) noexcept {
    return std::string_view{a} <= b.view();
  }

  friend bool operator>=(char const* a, generic_cstring const& b) noexcept {
    return std::string_view{a} >= b.view();
  }

  char const* internal_data() const noexcept {
    if constexpr (std::is_pointer_v<Ptr>) {
      return is_short() ? s_.s_ : h_.ptr_;
    } else {
      return is_short() ? s_.s_ : h_.ptr_.get();
    }
  }

  char* data() noexcept { return const_cast<char*>(internal_data()); }
  char const* data() const noexcept { return internal_data(); }

  msize_t size() const noexcept { return is_short() ? s_.size() : h_.size(); }

  struct heap {
    Ptr ptr_{nullptr};
    std::uint32_t size_{0};
    bool self_allocated_{false};
    char __fill__[sizeof(uintptr_t) == 8 ? 2 : 6]{0};
    int8_t minus_one_{-1};  // The offset of this field needs to match the
                            // offset of stack::remaining_ below.

    heap() = default;
    heap(msize_t len, owning_t) {
      char* mem = static_cast<char*>(std::malloc(len + 1));
      if (mem == nullptr) {
        throw_exception(std::bad_alloc{});
      }
      mem[len] = '\0';
      ptr_ = mem;
      size_ = len;
      self_allocated_ = true;
    }
    heap(Ptr ptr, msize_t len, non_owning_t) {
      ptr_ = ptr;
      size_ = len;
    }

    msize_t size() const { return size_; }
  };

  struct stack {
    char s_[short_length_limit]{0};
    int8_t remaining_{
        short_length_limit};  // The remaining capacity the inline buffer still
                              // has. A negative value indicates the buffer is
                              // not inline. In case the inline buffer is fully
                              // occupied, this field also serves as a null
                              // terminator.

    msize_t size() const {
      assert(remaining_ >= 0);
      return short_length_limit - static_cast<msize_t>(remaining_);
    }
  };

  union {
    heap h_;
    stack s_{};
  };
};

template <typename Ptr>
struct basic_cstring : public generic_cstring<Ptr> {
  using base = generic_cstring<Ptr>;

  using base::base;
  using base::operator std::string_view;

  friend std::ostream& operator<<(std::ostream& out, basic_cstring const& s) {
    return out << s.view();
  }

  explicit operator std::string() const { return {base::data(), base::size()}; }

  basic_cstring(std::string_view s) : base{s, base::owning} {}
  basic_cstring(std::string const& s) : base{s, base::owning} {}
  basic_cstring(char const* s) : base{s, base::owning} {}
  basic_cstring(char const* s, typename base::msize_t const len)
      : base{s, len, base::owning} {}

  basic_cstring(basic_cstring const& o) : base{o.view(), base::owning} {}
  basic_cstring(basic_cstring&& o) { base::move_from(std::move(o)); }

  basic_cstring& operator=(basic_cstring const& o) {
    base::set_owning(o.data(), o.size());
    return *this;
  }

  basic_cstring& operator=(basic_cstring&& o) {
    base::move_from(std::move(o));
    return *this;
  }

  basic_cstring& operator=(char const* s) {
    base::set_owning(s);
    return *this;
  }
  basic_cstring& operator=(std::string const& s) {
    base::set_owning(s);
    return *this;
  }
  basic_cstring& operator=(std::string_view s) {
    base::set_owning(s);
    return *this;
  }
};

template <typename Ptr>
struct basic_cstring_view : public generic_cstring<Ptr> {
  using base = generic_cstring<Ptr>;

  using base::base;
  using base::operator std::string_view;

  friend std::ostream& operator<<(std::ostream& out,
                                  basic_cstring_view const& s) {
    return out << s.view();
  }

  basic_cstring_view(std::string_view s) : base{s, base::non_owning} {}
  basic_cstring_view(std::string const& s) : base{s, base::non_owning} {}
  basic_cstring_view(char const* s) : base{s, base::non_owning} {}
  basic_cstring_view(char const* s, typename base::msize_t const len)
      : base{s, len, base::non_owning} {}

  basic_cstring_view(basic_cstring_view const& o) {
    base::set_non_owning(o.data(), o.size());
  }
  basic_cstring_view(basic_cstring_view&& o) {
    base::set_non_owning(o.data(), o.size());
  }
  basic_cstring_view& operator=(basic_cstring_view const& o) {
    base::set_non_owning(o.data(), o.size());
    return *this;
  }
  basic_cstring_view& operator=(basic_cstring_view&& o) {
    base::set_non_owning(o.data(), o.size());
    return *this;
  }

  basic_cstring_view& operator=(char const* s) {
    base::set_non_owning(s);
    return *this;
  }
  basic_cstring_view& operator=(std::string_view s) {
    base::set_non_owning(s);
    return *this;
  }
  basic_cstring_view& operator=(std::string const& s) {
    base::set_non_owning(s);
    return *this;
  }
};

template <typename Ptr>
struct is_string_helper<generic_cstring<Ptr>> : std::true_type {};

template <typename Ptr>
struct is_string_helper<basic_cstring<Ptr>> : std::true_type {};

template <typename Ptr>
struct is_string_helper<basic_cstring_view<Ptr>> : std::true_type {};

namespace raw {
using generic_cstring = generic_cstring<offset::ptr<char const>>;
using cstring = basic_cstring<offset::ptr<char const>>;
}  // namespace raw

namespace offset {
using generic_cstring = generic_cstring<offset::ptr<char const>>;
using cstring = basic_cstring<offset::ptr<char const>>;
}  // namespace offset

}  // namespace cista