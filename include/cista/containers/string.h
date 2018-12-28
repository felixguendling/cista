#pragma once

#include <cinttypes>
#include <cstring>
#include <string_view>

#include "cista/containers/offset_ptr.h"

namespace cista {

template <typename Ptr = char const*>
struct basic_string {
  using msize_t = uint32_t;

  static msize_t mstrlen(char const* s) {
    return static_cast<msize_t>(std::strlen(s));
  }

  static constexpr struct owning_t {
  } owning{};
  static constexpr struct non_owning_t {
  } non_owning{};

  basic_string() {
    std::memset(this, 0, sizeof(*this));
    h_.ptr_ = nullptr;
  }
  ~basic_string() { reset(); }

  basic_string(char const* s, owning_t) : basic_string() {
    set_owning(s, mstrlen(s));
  }
  basic_string(char const* s, non_owning_t) : basic_string() {
    set_non_owning(s);
  }
  basic_string(char const* s) : basic_string(s, non_owning) {}  // NOLINT

  basic_string(basic_string const& s) : basic_string() { copy_from(s); }

  basic_string(basic_string&& s) {
    if constexpr (std::is_pointer_v<Ptr>) {
      std::memcpy(this, &s, sizeof(*this));
      std::memset(&s, 0, sizeof(*this));
    } else {
      std::memcpy(this, &s, sizeof(*this));
      if (!s.is_short()) {
        h_.ptr_ = s.h_.ptr_;
        s.h_.ptr_ = nullptr;
      }
    }
  }

  basic_string& operator=(basic_string const& s) {
    copy_from(s);
    return *this;
  }

  basic_string& operator=(basic_string&& s) {
    std::memcpy(this, &s, sizeof(*this));
    if constexpr (std::is_pointer_v<Ptr>) {
      h_.ptr_ = s.h_.ptr_;
    }
    return *this;
  }

  bool is_short() const { return s_.is_short_; }

  void reset() {
    if (!h_.is_short_ && h_.ptr_ != nullptr && h_.self_allocated_) {
      std::free(const_cast<char*>(data()));
    }
    std::memset(this, 0, sizeof(*this));
    h_.ptr_ = nullptr;
  }

  void set_owning(std::string_view s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(char const* str) { set_owning(str, mstrlen(str)); }

  void set_owning(char const* str, msize_t const len) {
    reset();
    if (str == nullptr || len == 0) {
      return;
    }
    s_.is_short_ = (len <= 15);
    if (s_.is_short_) {
      std::memcpy(s_.s_, str, len);
    } else {
      h_.ptr_ = static_cast<char*>(std::malloc(len));
      if (h_.ptr_ == nullptr) {
        throw std::bad_alloc{};
      }
      h_.size_ = len;
      h_.self_allocated_ = true;
      std::memcpy(const_cast<char*>(data()), str, len);
    }
  }

  void set_non_owning(std::string_view v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(char const* str) { set_non_owning(str, mstrlen(str)); }

  void set_non_owning(char const* str, msize_t const len) {
    reset();
    if (str == nullptr || len == 0) {
      return;
    }

    if (len <= 15) {
      return set_owning(str, len);
    }

    h_.is_short_ = false;
    h_.self_allocated_ = false;
    h_.ptr_ = str;
    h_.size_ = len;
  }

  void copy_from(basic_string const& s) {
    reset();
    if (s.is_short()) {
      std::memcpy(this, &s, sizeof(s));
    } else if (s.h_.self_allocated_) {
      set_owning(s.data(), s.size());
    } else {
      set_non_owning(s.data(), s.size());
    }
  }

  std::string_view view() const { return std::string_view{data(), size()}; }

  char const* data() const {
    if constexpr (std::is_pointer_v<Ptr>) {
      return is_short() ? s_.s_ : h_.ptr_;
    } else {
      return is_short() ? s_.s_ : h_.ptr_.get();
    }
  }

  basic_string& operator=(char const* s) {
    set_non_owning(s, mstrlen(s));
    return *this;
  }

  friend bool operator==(basic_string const& a, char const* b) {
    return a.view() == std::string_view{b};
  }

  friend bool operator==(basic_string const& a, basic_string const& b) {
    return a.view() == b.view();
  }

  msize_t size() const {
    if (is_short()) {
      auto const pos = static_cast<char const*>(std::memchr(s_.s_, '\0', 15));
      if (pos == nullptr) {
        return 15;
      } else {
        return static_cast<msize_t>(pos - s_.s_);
      }
    } else {
      return h_.size_;
    }
  }

  struct heap {
    bool is_short_{false};
    bool self_allocated_{false};
    uint8_t __fill_2__{0};
    uint8_t __fill_3__{0};
    uint32_t size_{0};
    Ptr ptr_{nullptr};
  };

  struct stack {
    bool is_short_{true};
    char s_[15]{0};
  };

  union {
    heap h_;
    stack s_;
  };
};

}  // namespace cista
