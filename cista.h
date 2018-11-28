#pragma once


#include <iostream>
#include <limits>
#include <map>
#include <vector>


#include <cinttypes>
#include <cstring>
#include <string_view>

namespace cista {

struct string {
  using msize_t = uint32_t;

  static msize_t mstrlen(char const* s) {
    return static_cast<msize_t>(std::strlen(s));
  }

  static constexpr struct owning_t {
  } owning{};
  static constexpr struct non_owning_t {
  } non_owning{};

  string() { std::memset(this, 0, sizeof(*this)); }
  ~string() { reset(); }

  string(char const* s, owning_t) : string() { set_owning(s, mstrlen(s)); }
  string(char const* s, non_owning_t) : string() { set_non_owning(s); }
  string(char const* s) : string(s, non_owning) {}  // NOLINT

  string(string const& s) : string() { copy_from(s); }

  string(string&& s) {
    std::memcpy(this, &s, sizeof(*this));
    std::memset(&s, 0, sizeof(*this));
  }

  string& operator=(string const& s) {
    copy_from(s);
    return *this;
  }

  string& operator=(string&& s) {
    std::memcpy(this, &s, sizeof(*this));
    return *this;
  }

  bool is_short() const { return s_.is_short_; }

  void reset() {
    if (!h_.is_short_ && h_.ptr_ != nullptr && h_.self_allocated_) {
      std::free(const_cast<char*>(h_.ptr_));
    }
    std::memset(this, 0, sizeof(*this));
  }

  void set_owning(std::string_view s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(char const* str) { set_owning(str, mstrlen(str)); }

  void set_owning(char const* str, msize_t const size) {
    reset();
    if (str == nullptr || size == 0) {
      return;
    }
    s_.is_short_ = (size <= 15);
    if (s_.is_short_) {
      std::memcpy(s_.s_, str, size);
    } else {
      h_.ptr_ = static_cast<char*>(std::malloc(size));
      if (h_.ptr_ == nullptr) {
        throw std::bad_alloc{};
      }
      h_.size_ = size;
      h_.self_allocated_ = true;
      std::memcpy(const_cast<char*>(h_.ptr_), str, size);
    }
  }

  void set_non_owning(std::string_view v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(char const* str) { set_non_owning(str, mstrlen(str)); }

  void set_non_owning(char const* str, msize_t const size) {
    reset();
    if (str == nullptr || size == 0) {
      return;
    }

    if (size <= 15) {
      return set_owning(str, size);
    }

    h_.is_short_ = false;
    h_.self_allocated_ = false;
    h_.ptr_ = str;
    h_.size_ = size;
  }

  void copy_from(string const& s) {
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

  char const* data() const { return is_short() ? s_.s_ : h_.ptr_; }

  string& operator=(char const* s) {
    set_non_owning(s, mstrlen(s));
    return *this;
  }

  friend bool operator==(string const& a, char const* b) {
    return a.view() == std::string_view{b};
  }

  friend bool operator==(string const& a, string const& b) {
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
    char const* ptr_{nullptr};
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

#include <cinttypes>

namespace cista {

template <typename T>
struct unique_ptr {
  using value_t = T;

  unique_ptr() = default;

  explicit unique_ptr(T* el, bool take_ownership = true)
      : el_{el}, self_allocated_{take_ownership} {}

  unique_ptr(unique_ptr const&) = delete;
  unique_ptr& operator=(unique_ptr const&) = delete;

  unique_ptr(unique_ptr&& o) : el_{o.el_}, self_allocated_{o.self_allocated_} {
    o.el_ = nullptr;
    o.self_allocated_ = false;
  }

  unique_ptr& operator=(unique_ptr&& o) {
    el_ = o.el_;
    self_allocated_ = o.self_allocated_;
    o.el_ = nullptr;
    o.self_allocated = false;
  }

  ~unique_ptr() {
    if (self_allocated_ && el_ != nullptr) {
      delete el_;
    }
  }

  T* get() const { return el_; }
  T* operator->() { return el_; }
  T const* operator->() const { return el_; }

  T* el_{nullptr};
  bool self_allocated_{false};
  uint8_t __fill_0__{0};
  uint16_t __fill_1__{0};
  uint32_t __fill_2__{0};
};

template <typename T, typename... Args>
cista::unique_ptr<T> make_unique(Args&&... args) {
  return cista::unique_ptr<T>{new T{std::forward<Args>(args)...}, true};
}

}  // namespace cista

#include <cassert>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <ostream>
#include <string>
#include <type_traits>

namespace cista {

template <typename T, typename TemplateSizeType = uint32_t>
struct vector {
  using size_type = TemplateSizeType;
  using value_type = T;
  using iterator = T*;
  using const_iterator = T const*;

  static inline TemplateSizeType next_power_of_two(TemplateSizeType n) {
    n--;
    n |= n >> 1u;
    n |= n >> 2u;
    n |= n >> 4u;
    n |= n >> 8u;
    n |= n >> 16u;
    if constexpr (sizeof(TemplateSizeType) > 32) {
      n |= n >> 32u;
    }
    n++;
    return n;
  }

  explicit vector(TemplateSizeType size = 0) { resize(size); }

  explicit vector(const char* str) {
    auto length = static_cast<size_type>(std::strlen(str) + 1);
    reserve(length);
    std::memcpy(el_, str, length);
    used_size_ = length;
  }

  template <typename It>
  vector(It begin_it, It end_it) {
    set(begin_it, end_it);
  }

  vector(vector&& arr) noexcept
      : el_(arr.el_),
        used_size_(arr.used_size_),
        allocated_size_(arr.allocated_size_),
        self_allocated_(arr.self_allocated_) {
    arr.el_ = nullptr;
    arr.used_size_ = 0;
    arr.self_allocated_ = false;
    arr.allocated_size_ = 0;
  }

  vector(vector const& arr) { set(std::begin(arr), std::end(arr)); }

  vector& operator=(vector&& arr) noexcept {
    deallocate();

    el_ = arr.el_;
    used_size_ = arr.used_size_;
    self_allocated_ = arr.self_allocated_;
    allocated_size_ = arr.allocated_size_;

    arr.el_ = nullptr;
    arr.used_size_ = 0;
    arr.self_allocated_ = false;
    arr.allocated_size_ = 0;

    return *this;
  }

  vector& operator=(vector const& arr) {
    set(std::begin(arr), std::end(arr));
    return *this;
  }

  ~vector() { deallocate(); }

  void deallocate() {
    if (!self_allocated_ || el_ == nullptr) {
      return;
    }

    for (auto& el : *this) {
      el.~T();
    }

    std::free(el_);  // NOLINT
    el_ = nullptr;
    used_size_ = 0;
    allocated_size_ = 0;
    self_allocated_ = 0;
  }

  T const* begin() const { return el_; }
  T const* end() const { return el_ + used_size_; }  // NOLINT
  T* begin() { return el_; }
  T* end() { return el_ + used_size_; }  // NOLINT

  std::reverse_iterator<T const*> rbegin() const {
    return std::reverse_iterator<T*>(el_ + size());  // NOLINT
  }
  std::reverse_iterator<T const*> rend() const {
    return std::reverse_iterator<T*>(el_);
  }
  std::reverse_iterator<T*> rbegin() {
    return std::reverse_iterator<T*>(el_ + size());  // NOLINT
  }
  std::reverse_iterator<T*> rend() { return std::reverse_iterator<T*>(el_); }

  friend T const* begin(vector const& a) { return a.begin(); }
  friend T const* end(vector const& a) { return a.end(); }

  friend T* begin(vector& a) { return a.begin(); }
  friend T* end(vector& a) { return a.end(); }

  inline T const& operator[](size_t index) const { return el_[index]; }
  inline T& operator[](size_t index) { return el_[index]; }

  T const& back() const { return el_[used_size_ - 1]; }
  T& back() { return el_[used_size_ - 1]; }

  T& front() { return el_[0]; }
  T const& front() const { return el_[0]; }

  inline TemplateSizeType size() const { return used_size_; }
  inline bool empty() const { return size() == 0; }

  vector& operator=(std::string const& str) {
    *this = vector(str.c_str());
    return *this;
  }

  template <typename It>
  void set(It begin_it, It end_it) {
    auto range_size = std::distance(begin_it, end_it);
    reserve(range_size);

    auto copy_source = begin_it;
    auto copy_target = el_;
    for (; copy_source != end_it; ++copy_source, ++copy_target) {
      new (copy_target) T(*copy_source);
    }

    used_size_ = range_size;
  }

  void push_back(T const& el) {
    reserve(used_size_ + 1);
    new (el_ + used_size_) T(el);
    ++used_size_;
  }

  template <typename... Args>
  T& emplace_back(Args&&... el) {
    reserve(used_size_ + 1);
    new (el_ + used_size_) T(std::forward<Args>(el)...);
    T* ptr = el_ + used_size_;
    ++used_size_;
    return *ptr;
  }

  void resize(size_type size) {
    reserve(size);
    used_size_ = size;
  }

  void clear() {
    used_size_ = 0;
    for (auto& el : *this) {
      el.~T();
    }
  }

  void reserve(TemplateSizeType new_size) {
    new_size = std::max(allocated_size_, new_size);

    if (allocated_size_ >= new_size) {
      return;
    }

    auto next_size = next_power_of_two(new_size);
    auto mem_buf =
        static_cast<T*>(std::malloc(sizeof(T) * next_size));  // NOLINT
    if (mem_buf == nullptr) {
      throw std::bad_alloc();
    }

    if (size() != 0) {
      try {
        auto move_target = mem_buf;
        for (auto& el : *this) {
          new (move_target++) T(std::move(el));
        }

        for (auto& el : *this) {
          el.~T();
        }
      } catch (...) {
        assert(0);
      }
    }

    auto free_me = el_;
    el_ = mem_buf;
    if (self_allocated_) {
      std::free(free_me);  // NOLINT
    }

    self_allocated_ = true;
    allocated_size_ = next_size;
  }

  T* erase(T* pos) {
    T* last = end() - 1;
    while (pos < last) {
      std::swap(*pos, *(pos + 1));
      pos = pos + 1;
    }
    pos->~T();
    --used_size_;
    return end();
  }

  bool contains(T const* el) const { return el >= begin() && el < end(); }

  std::string to_string() const { return std::string(el_); }

  explicit operator std::string() const { return to_string(); }

  T* el_{nullptr};
  TemplateSizeType used_size_{0};
  TemplateSizeType allocated_size_{0};
  bool self_allocated_{false};
  uint8_t __fill_0__{0};
  uint16_t __fill_1__{0};
  uint32_t __fill_2__{0};
};

template <typename T>
inline bool operator==(vector<T> const& a, vector<T> const& b) {
  return a.size() == b.size() &&
         std::equal(std::begin(a), std::end(a), std::begin(b));
}

template <typename T>
inline bool operator<(vector<T> const& a, vector<T> const& b) {
  return std::lexicographical_compare(std::begin(a), std::end(a), std::begin(b),
                                      std::end(b));
}

template <typename T>
inline bool operator<=(vector<T> const& a, vector<T> const& b) {
  return !(a > b);
}

template <typename T>
inline bool operator>(vector<T> const& a, vector<T> const& b) {
  return b < a;
}

template <typename T>
inline bool operator>=(vector<T> const& a, vector<T> const& b) {
  return !(a < b);
}

}  // namespace cista

#include <cinttypes>

namespace cista {

using offset_t = uint64_t;

}  // namespace cista

#include <type_traits>
#include <utility>


#include <type_traits>

namespace cista {

// Source: playfulprogramming.blogspot.de

template <size_t N>
using field_count = std::integral_constant<size_t, N>;

struct wildcard {
  template <typename T,
            typename = std::enable_if_t<!std::is_lvalue_reference<T>::value>>
  operator T &&() const;

  template <typename T,
            typename = std::enable_if_t<std::is_copy_constructible<T>::value>>
  operator T&() const;
};

template <size_t N = 0>
static constexpr const wildcard _{};

template <typename T, size_t... I>
inline constexpr auto is_brace_constructible_(std::index_sequence<I...>, T*)
    -> decltype(T{_<I>...}, std::true_type{}) {
  return {};
}

template <size_t... I>
inline constexpr std::false_type is_brace_constructible_(
    std::index_sequence<I...>, ...) {
  return {};
}

template <typename T, size_t N>
constexpr auto is_brace_constructible()
    -> decltype(is_brace_constructible_(std::make_index_sequence<N>{},
                                        static_cast<T*>(nullptr))) {
  return {};
}

template <typename T, typename U>
struct is_paren_constructible_;

template <typename T, size_t... I>
struct is_paren_constructible_<T, std::index_sequence<I...>>
    : std::is_constructible<T, decltype(_<I>)...> {};

template <typename T, size_t N>
constexpr auto is_paren_constructible()
    -> is_paren_constructible_<T, std::make_index_sequence<N>> {
  return {};
}

#define MAKE_ARITY_FUNC(count)                                              \
  template <typename T,                                                     \
            typename =                                                      \
                std::enable_if_t<is_brace_constructible<T, count>() &&      \
                                 !is_brace_constructible<T, count + 1>() && \
                                 !is_paren_constructible<T, count>()>>      \
  constexpr field_count<count> arity(T&) {                                  \
    return {};                                                              \
  }

MAKE_ARITY_FUNC(1)
MAKE_ARITY_FUNC(2)
MAKE_ARITY_FUNC(3)
MAKE_ARITY_FUNC(4)
MAKE_ARITY_FUNC(5)
MAKE_ARITY_FUNC(6)
MAKE_ARITY_FUNC(7)
MAKE_ARITY_FUNC(8)
MAKE_ARITY_FUNC(9)
MAKE_ARITY_FUNC(10)
MAKE_ARITY_FUNC(11)
MAKE_ARITY_FUNC(12)
MAKE_ARITY_FUNC(13)
MAKE_ARITY_FUNC(14)
MAKE_ARITY_FUNC(15)
MAKE_ARITY_FUNC(16)
MAKE_ARITY_FUNC(17)
MAKE_ARITY_FUNC(18)
MAKE_ARITY_FUNC(19)
MAKE_ARITY_FUNC(20)

}  // namespace cista

#include <tuple>


namespace cista {

template <typename T>
inline auto to_tuple(T&, field_count<0>) {
  return std::tie();
}

template <typename T>
inline auto to_tuple(T& t, field_count<1>) {
  auto& [p1] = t;
  return std::tie(p1);
}

template <typename T>
inline auto to_tuple(T& t, field_count<2>) {
  auto& [p1, p2] = t;
  return std::tie(p1, p2);
}

template <typename T>
inline auto to_tuple(T& t, field_count<3>) {
  auto& [p1, p2, p3] = t;
  return std::tie(p1, p2, p3);
}

template <typename T>
inline auto to_tuple(T& t, field_count<4>) {
  auto& [p1, p2, p3, p4] = t;
  return std::tie(p1, p2, p3, p4);
}

template <typename T>
inline auto to_tuple(T& t, field_count<5>) {
  auto& [p1, p2, p3, p4, p5] = t;
  return std::tie(p1, p2, p3, p4, p5);
}

template <typename T>
inline auto to_tuple(T& t, field_count<6>) {
  auto& [p1, p2, p3, p4, p5, p6] = t;
  return std::tie(p1, p2, p3, p4, p5, p6);
}

template <typename T>
inline auto to_tuple(T& t, field_count<7>) {
  auto& [p1, p2, p3, p4, p5, p6, p7] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7);
}

template <typename T>
inline auto to_tuple(T& t, field_count<8>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8);
}

template <typename T>
inline auto to_tuple(T& t, field_count<9>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

template <typename T>
inline auto to_tuple(T& t, field_count<10>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

template <typename T>
inline auto to_tuple(T& t, field_count<11>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
}

template <typename T>
inline auto to_tuple(T& t, field_count<12>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
}

template <typename T>
inline auto to_tuple(T& t, field_count<13>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
}

template <typename T>
inline auto to_tuple(T& t, field_count<14>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
}

template <typename T>
inline auto to_tuple(T& t, field_count<15>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15);
}

template <typename T>
inline auto to_tuple(T& t, field_count<16>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
         p16] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16);
}

template <typename T>
inline auto to_tuple(T& t, field_count<17>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17);
}

template <typename T>
inline auto to_tuple(T& t, field_count<18>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17,
         p18] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17, p18);
}

template <typename T>
inline auto to_tuple(T& t, field_count<19>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18,
         p19] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17, p18, p19);
}

template <typename T>
inline auto to_tuple(T& t, field_count<20>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18, p19,
         p20] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17, p18, p19, p20);
}

template <typename T>
inline auto to_tuple(T& t) {
  return to_tuple(t, arity(t));
}

// Pointer to member

template <typename T>
inline auto to_ptr_tuple(T&, field_count<0>) {
  return std::make_tuple();
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<1>) {
  auto& [p1] = t;
  return std::make_tuple(&p1);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<2>) {
  auto& [p1, p2] = t;
  return std::make_tuple(&p1, &p2);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<3>) {
  auto& [p1, p2, p3] = t;
  return std::make_tuple(&p1, &p2, &p3);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<4>) {
  auto& [p1, p2, p3, p4] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<5>) {
  auto& [p1, p2, p3, p4, p5] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<6>) {
  auto& [p1, p2, p3, p4, p5, p6] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<7>) {
  auto& [p1, p2, p3, p4, p5, p6, p7] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<8>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<9>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<10>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<11>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<12>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<13>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<14>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<15>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<16>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
         p16] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<17>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<18>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17,
         p18] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<19>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18,
         p19] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18, &p19);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<20>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18, p19,
         p20] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18, &p19,
                         &p20);
}

template <typename T>
inline auto to_ptr_tuple(T& t) {
  return to_ptr_tuple(t, arity(t));
}

}  // namespace cista

namespace cista {

template <typename T, typename Fn>
inline void for_each_ptr_field(T& t, Fn&& fn) {
  if constexpr (std::is_pointer_v<T>) {
    if (t != nullptr) {
      for_each_ptr_field(*t, std::forward<Fn>(fn));
    }
  } else if constexpr (std::is_scalar_v<T>) {
    fn(t);
  } else {
    std::apply([&](auto&&... args) { (fn(args), ...); }, to_ptr_tuple(t));
  }
}

template <typename T, typename Fn>
inline void for_each_field(T& t, Fn&& fn) {
  if constexpr (std::is_pointer_v<T>) {
    if (t != nullptr) {
      for_each_field(*t, std::forward<Fn>(fn));
    }
  } else if constexpr (std::is_scalar_v<T>) {
    fn(t);
  } else {
    std::apply([&](auto&&... args) { (fn(args), ...); }, to_tuple(t));
  }
}

template <typename T, typename Fn>
inline void for_each_field(Fn&& fn) {
  T t{};
  for_each_field<T>(t, std::forward<Fn>(fn));
}

}  // namespace cista

#include <cstdio>
#include <cstring>

#include <stdexcept>

#define cista_verify(A, M)       \
  if (!(A)) {                    \
    throw std::runtime_error(M); \
  }

namespace cista {

constexpr auto const MAX_ALIGN = 16;
using byte_buf = std::vector<uint8_t>;

struct buf {
  uint8_t* addr(offset_t const offset) { return (&buf_[0]) + offset; }
  uint8_t* base() { return &buf_[0]; }

  template <typename T>
  void write(offset_t const pos, T const& val) {
    cista_verify(buf_.size() >= pos + sizeof(val), "out of bounds write");
    std::memcpy(&buf_[pos], &val, sizeof(val));
  }

  offset_t write(void const* ptr, offset_t const size, offset_t alignment = 0) {
    auto aligned_size = size;

    if (alignment != 0 && buf_.size() != 0) {
      auto unaligned_ptr = static_cast<void*>(addr(curr_offset_));
      auto space = static_cast<size_t>(alignment) * 8u;
      auto const aligned_ptr =
          std::align(alignment, size, unaligned_ptr, space);
      auto const new_offset = static_cast<offset_t>(
          aligned_ptr ? static_cast<uint8_t*>(aligned_ptr) - base() : 0);
      auto const adjustment = new_offset - curr_offset_;
      curr_offset_ += adjustment;
      aligned_size += adjustment;
    }

    auto const space_left =
        static_cast<int64_t>(buf_.size()) - static_cast<int64_t>(curr_offset_);
    if (space_left < static_cast<int64_t>(aligned_size)) {
      auto const missing = static_cast<offset_t>(
          static_cast<int64_t>(aligned_size) - space_left);
      buf_.resize(buf_.size() + missing);
    }

    auto const start = curr_offset_;
    std::memcpy(addr(curr_offset_), ptr, size);
    curr_offset_ += size;
    return start;
  }

  byte_buf buf_;
  offset_t curr_offset_{0};
  FILE* f_;
};

}  // namespace cista

#include <cstdio>


#ifdef _MSC_VER
inline FILE* open_file(char const* path, char const* mode) {
  FILE* ptr = nullptr;
  fopen_s(&ptr, path, mode);
  return ptr;
}
#else
inline FILE* open_file(char const* path, char const* mode) {
  return std::fopen(path, mode);
}
#endif

namespace cista {

struct sfile {
  sfile(char const* path, char const* mode) : f_(open_file(path, mode)) {
    cista_verify(f_ != nullptr, "unable to open file");
  }

  ~sfile() {
    if (f_ != nullptr) {
      std::fclose(f_);
    }
    f_ = nullptr;
  }

  template <typename T>
  void write(offset_t const pos, T const& val) {
    std::fseek(f_, static_cast<long>(pos), SEEK_SET);
    auto const w = std::fwrite(reinterpret_cast<unsigned char const*>(&val), 1,
                               sizeof(val), f_);
    cista_verify(w == sizeof(val), "write error");
  }

  offset_t write(void const* ptr, offset_t const size, offset_t alignment) {
    auto curr_offset = size_;
    if (alignment != 0) {
      auto unaligned_ptr = reinterpret_cast<void*>(size_);
      auto space = static_cast<size_t>(alignment) * 8u;
      auto const aligned_ptr =
          std::align(alignment, size, unaligned_ptr, space);
      curr_offset =
          aligned_ptr ? reinterpret_cast<offset_t>(aligned_ptr) : curr_offset;
      std::fseek(f_, static_cast<long>(curr_offset), SEEK_SET);
    } else {
      std::fseek(f_, 0, SEEK_END);
    }
    auto const w = std::fwrite(ptr, 1, size, f_);
    cista_verify(w == size, "write error");
    size_ = curr_offset + size;
    return curr_offset;
  }

  offset_t size_ = 0u;
  FILE* f_;
};

}  // namespace cista

namespace cista {

// =============================================================================
// SERIALIZE
// -----------------------------------------------------------------------------
struct pending_offset {
  void* origin_ptr_;
  offset_t pos_;
};

template <typename Target>
struct serialization_context {
  explicit serialization_context(Target& t) : t_{t} {}

  offset_t write(void const* ptr, offset_t const size, offset_t alignment = 0) {
    return t_.write(ptr, size, alignment);
  }

  template <typename T>
  void write(offset_t const pos, T const& val) {
    t_.write(pos, val);
  }

  std::map<void*, offset_t> offsets_;
  std::vector<pending_offset> pending_;
  Target& t_;
};

template <typename Ctx, typename T>
void serialize(Ctx& c, T const* origin, offset_t const pos) {
  using Type = std::remove_reference_t<std::remove_const_t<T>>;
  if constexpr (!std::is_scalar_v<Type>) {
    cista::for_each_ptr_field(*origin, [&](auto& member) {
      auto const member_offset =
          static_cast<offset_t>(reinterpret_cast<char const*>(member) -
                                reinterpret_cast<char const*>(origin));
      serialize(c, member, pos + member_offset);
    });
  } else if constexpr (std::is_pointer_v<Type>) {
    if (*origin == nullptr) {
      c.write(pos, std::numeric_limits<offset_t>::max());
      return;
    }
    if (auto const it = c.offsets_.find(*origin); it != end(c.offsets_)) {
      c.write(pos, it->second);
    } else {
      c.pending_.emplace_back(pending_offset{*origin, pos});
    }
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, cista::vector<T> const* origin, offset_t const pos) {
  auto const size = sizeof(T) * origin->used_size_;
  auto const start = origin->el_ != nullptr
                         ? c.write(origin->el_, size, std::alignment_of_v<T>)
                         : std::numeric_limits<offset_t>::max();

  c.write(pos + offsetof(cista::vector<T>, el_), start);
  c.write(pos + offsetof(cista::vector<T>, allocated_size_),
          origin->used_size_);
  c.write(pos + offsetof(cista::vector<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    auto i = 0u;
    for (auto it = start; it != start + size; it += sizeof(T)) {
      serialize(c, origin->el_ + i++, it);
    }
  }
}

template <typename Ctx>
void serialize(Ctx& c, cista::string const* origin, offset_t const pos) {
  if (origin->is_short()) {
    return;
  }

  auto const start =
      c.write(origin->data(), origin->size(), std::alignment_of_v<char>);
  c.write(pos + offsetof(cista::string, h_.ptr_), start);
  c.write(pos + offsetof(cista::string, h_.self_allocated_), false);
}

template <typename Ctx, typename T>
void serialize(Ctx& c, cista::unique_ptr<T> const* origin, offset_t const pos) {
  auto const start =
      origin->el_ != nullptr
          ? c.write(origin->el_, sizeof(T), std::alignment_of_v<T>)
          : std::numeric_limits<offset_t>::max();

  c.write(pos + offsetof(cista::unique_ptr<T>, el_), start);
  c.write(pos + offsetof(cista::unique_ptr<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    c.offsets_[origin->el_] = start;
    serialize(c, origin->el_, start);
  }
}

template <typename Target, typename T>
void serialize(Target& t, T& value) {
  serialization_context<Target> c{t};

  serialize(
      c, &value,
      c.write(
          &value, sizeof(value),
          std::alignment_of_v<
              std::remove_reference_t<std::remove_const_t<decltype(value)>>>));

  for (auto& p : c.pending_) {
    if (auto const it = c.offsets_.find(p.origin_ptr_); it != end(c.offsets_)) {
      c.write(p.pos_, it->second);
    } else {
      std::cout << "warning: dangling pointer " << p.origin_ptr_
                << " serialized at offset " << p.pos_ << "\n";
    }
  }
}

template <typename T>
byte_buf serialize(T& el) {
  auto b = buf{};
  serialize(b, el);
  return std::move(b.buf_);
}

// =============================================================================
// DESERIALIZE
// -----------------------------------------------------------------------------
struct deserialization_context {
  deserialization_context(uint8_t* from, uint8_t* to) : from_{from}, to_{to} {}

  template <typename T, typename Ptr>
  T deserialize(Ptr* ptr) const {
    auto const offset = reinterpret_cast<offset_t>(ptr);
    if (offset == std::numeric_limits<offset_t>::max()) {
      return nullptr;
    }
    if (to_ != nullptr && offset >= static_cast<offset_t>(to_ - from_)) {
      throw std::runtime_error("pointer out of bounds");
    }
    return reinterpret_cast<T>(from_ + offset);
  }

  uint8_t *from_, *to_;
};

template <typename T>
void deserialize(deserialization_context const& c, T* el) {
  using written_type_t = std::remove_reference_t<std::remove_const_t<T>>;
  if constexpr (std::is_pointer_v<written_type_t>) {
    *el = c.deserialize<written_type_t>(*el);
  } else if constexpr (std::is_scalar_v<written_type_t>) {
    return;
  } else {
    cista::for_each_ptr_field(*el, [&](auto& f) { deserialize(c, f); });
  }
}

template <typename T>
void deserialize(deserialization_context const& c, cista::vector<T>* el) {
  el->el_ = c.deserialize<T*>(el->el_);
  for (auto& m : *el) {
    deserialize(c, &m);
  }
}

inline void deserialize(deserialization_context const& c, cista::string* el) {
  if (el->is_short()) {
    return;
  } else {
    el->h_.ptr_ = c.deserialize<char*>(el->h_.ptr_);
  }
}

template <typename T>
void deserialize(deserialization_context const& c, cista::unique_ptr<T>* el) {
  el->el_ = c.deserialize<T*>(el->el_);
  deserialize(c, el->el_);
}

template <typename T>
T* deserialize(uint8_t* from, uint8_t* to = nullptr) {
  deserialization_context c{from, to};
  auto const el = reinterpret_cast<T*>(from);
  deserialize(c, el);
  return el;
}

}  // namespace cista


#include <cstdlib>
#include <cstring>


namespace cista {

struct buffer final {
  buffer() : buf_(nullptr), size_(0) {}

  explicit buffer(std::size_t size) : buf_(malloc(size)), size_(size) {
    cista_verify(buf_ != nullptr, "buffer initialization failed");
  }

  explicit buffer(char const* str) : buffer(std::strlen(str)) {
    std::memcpy(buf_, str, size_);
  }

  buffer(char const* str, std::size_t size) : buffer(size) {
    std::memcpy(buf_, str, size_);
  }

  ~buffer() {
    std::free(buf_);
    buf_ = nullptr;
  }

  buffer(buffer const&) = delete;
  buffer& operator=(buffer const&) = delete;

  buffer(buffer&& o) noexcept : buf_(o.buf_), size_(o.size_) {
    o.buf_ = nullptr;
    o.size_ = 0;
  }

  buffer& operator=(buffer&& o) noexcept {
    buf_ = o.buf_;
    size_ = o.size_;
    o.buf_ = nullptr;
    o.size_ = 0;
    return *this;
  }

  inline std::size_t size() const { return size_; }

  inline unsigned char* data() { return static_cast<unsigned char*>(buf_); }
  inline unsigned char const* data() const {
    return static_cast<unsigned char const*>(buf_);
  }

  inline unsigned char* begin() { return data(); }
  inline unsigned char* end() { return data() + size_; }

  void* buf_;
  std::size_t size_;
};

}  // namespace cista

#ifdef _MSC_VER
#include <cstdio>

#define NOMINMAX
#include <windows.h>

namespace cista {

inline HANDLE open_file(char const* path, char const* mode) {
  bool read = std::strcmp(mode, "r") == 0;
  bool write = std::strcmp(mode, "w+") == 0;

  cista_verify(read || write, "invalid open file mode");

  DWORD access = read ? GENERIC_READ : GENERIC_WRITE;
  DWORD create_mode = read ? OPEN_EXISTING : CREATE_ALWAYS;

  return CreateFileA(path, access, 0, nullptr, create_mode,
                     FILE_ATTRIBUTE_NORMAL, nullptr);
}

template <typename Fn>
inline void chunk(unsigned const chunk_size, size_t const total, Fn fn) {
  size_t offset = 0;
  size_t remaining = total;
  while (remaining > 0) {
    auto const curr_chunk_size = static_cast<unsigned>(
        std::min(remaining, static_cast<size_t>(chunk_size)));
    fn(offset, curr_chunk_size);
    offset += curr_chunk_size;
    remaining -= curr_chunk_size;
  }
}

struct file {
  file(char const* path, char const* mode) : f_(open_file(path, mode)) {
    cista_verify(f_ != INVALID_HANDLE_VALUE, "unable to open file");
  }

  ~file() {
    if (f_ != nullptr) {
      CloseHandle(f_);
    }
  }

  size_t size() {
    LARGE_INTEGER filesize;
    GetFileSizeEx(f_, &filesize);
    return filesize.QuadPart;
  }

  buffer content() {
    constexpr auto block_size = 8192u;
    size_t const file_size = size();

    auto b = buffer(file_size);

    chunk(block_size, size(), [&](size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<DWORD>(from);
      overlapped.OffsetHigh = from >> 32u;
      ReadFile(f_, b.data() + from, static_cast<DWORD>(block_size), nullptr,
               &overlapped);
    });

    return b;
  }

  void write(void const* buf, size_t size) {
    constexpr auto block_size = 8192u;
    chunk(block_size, size, [&](size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<DWORD>(from);
      overlapped.OffsetHigh = from >> 32u;
      WriteFile(f_, static_cast<unsigned char const*>(buf) + from, block_size,
                nullptr, &overlapped);
    });
  }

  HANDLE f_;
};
#else
#include <cstdio>

namespace cista {

struct file {
  file(char const* path, char const* mode) : f_(std::fopen(path, mode)) {
    cista_verify(f_ != nullptr, "unable to open file");
  }

  ~file() {
    if (f_ != nullptr) {
      fclose(f_);
    }
    f_ = nullptr;
  }

  size_t size() {
    auto err = std::fseek(f_, 0, SEEK_END);
    cista_verify(!err, "fseek to SEEK_END error");
    auto size = std::ftell(f_);
    std::rewind(f_);
    return static_cast<size_t>(size);
  }

  buffer content() {
    auto file_size = size();
    auto b = buffer(file_size);
    auto bytes_read = std::fread(b.data(), 1, file_size, f_);
    cista_verify(bytes_read == file_size, "file read error");
    return b;
  }

  void write(void const* buf, size_t size) {
    auto bytes_written = std::fwrite(buf, 1, size, f_);
    cista_verify(bytes_written == size, "file write error");
  }

  operator FILE*() { return f_; }

  FILE* f_;
};
#endif

}  // namespace cista
