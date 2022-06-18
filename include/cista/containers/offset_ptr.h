#pragma once

#if defined(__has_include) && __cplusplus >= 202002L
#if __has_include(<bit>)
#include <bit>
#endif
#endif
#include <cstring>
#include <type_traits>

#include "cista/offset_t.h"
#include "cista/strong.h"

namespace cista {

#if __cpp_lib_bit_cast
inline offset_t to_offset(void const* ptr) {
  return std::bit_cast<offset_t>(ptr);
}
#else
inline offset_t to_offset(void const* ptr) {
  offset_t r;
  std::memcpy(&r, &ptr, sizeof(ptr));
  return r;
}
#endif

template <typename T, typename Enable = void>
struct offset_ptr {
  offset_ptr() noexcept = default;
  offset_ptr(std::nullptr_t) noexcept : offset_{NULLPTR_OFFSET} {}
  offset_ptr(T const* p) noexcept : offset_{ptr_to_offset(p)} {}

  offset_ptr& operator=(T const* p) noexcept {
    offset_ = ptr_to_offset(p);
    return *this;
  }
  offset_ptr& operator=(std::nullptr_t) noexcept {
    offset_ = NULLPTR_OFFSET;
    return *this;
  }

  offset_ptr(offset_ptr const& o) noexcept : offset_{ptr_to_offset(o.get())} {}
  offset_ptr(offset_ptr&& o) noexcept : offset_{ptr_to_offset(o.get())} {}
  offset_ptr& operator=(offset_ptr const& o) noexcept {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }
  offset_ptr& operator=(offset_ptr&& o) noexcept {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }

  ~offset_ptr() noexcept = default;

  offset_t ptr_to_offset(T const* p) const noexcept {
    return p == nullptr ? NULLPTR_OFFSET
                        : static_cast<offset_t>(to_offset(p) - to_offset(this));
  }

  explicit operator bool() const noexcept { return offset_ != NULLPTR_OFFSET; }
  explicit operator void*() const noexcept { return get(); }
  explicit operator void const*() const noexcept { return get(); }
  operator T*() const noexcept { return get(); }
  T& operator*() const noexcept { return *get(); }
  T* operator->() const noexcept { return get(); }
  T& operator[](size_t const i) const noexcept { return get()[i]; }

  T* get() const noexcept {
    auto const ptr =
        offset_ == NULLPTR_OFFSET
            ? nullptr
            : reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset_);
    return ptr;
  }

  template <typename Int>
  T* operator+(Int i) const noexcept {
    return get() + i;
  }

  template <typename Int>
  T* operator-(Int i) const noexcept {
    return get() - i;
  }

  template <typename X, typename Tag>
  constexpr T* operator+(strong<X, Tag> const& s) {
    return get() + s.v_;
  }

  template <typename X, typename Tag>
  constexpr T* operator-(strong<X, Tag> const& s) {
    return get() - s.v_;
  }

  offset_ptr& operator++() noexcept {
    offset_ = ptr_to_offset(get() + 1);
    return *this;
  }

  offset_ptr& operator--() noexcept {
    offset_ = ptr_to_offset(get() - 1);
    return *this;
  }

  offset_ptr operator++(int) const noexcept { return offset_ptr{get() + 1}; }
  offset_ptr operator--(int) const noexcept { return offset_ptr{get() - 1}; }

  offset_t offset_{NULLPTR_OFFSET};
};

template <typename T>
struct offset_ptr<T, std::enable_if_t<std::is_same_v<void, T>>> {
  offset_ptr() noexcept = default;
  offset_ptr(std::nullptr_t) noexcept : offset_{NULLPTR_OFFSET} {}
  offset_ptr(T const* p) noexcept : offset_{ptr_to_offset(p)} {}

  offset_ptr& operator=(T const* p) noexcept {
    offset_ = ptr_to_offset(p);
    return *this;
  }
  offset_ptr& operator=(std::nullptr_t) noexcept {
    offset_ = NULLPTR_OFFSET;
    return *this;
  }

  offset_ptr(offset_ptr const& o) noexcept : offset_{ptr_to_offset(o.get())} {}
  offset_ptr(offset_ptr&& o) noexcept : offset_{ptr_to_offset(o.get())} {}
  offset_ptr& operator=(offset_ptr const& o) noexcept {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }
  offset_ptr& operator=(offset_ptr&& o) noexcept {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }

  offset_t ptr_to_offset(T const* p) const noexcept {
    return p == nullptr ? NULLPTR_OFFSET
                        : static_cast<offset_t>(to_offset(p) - to_offset(this));
  }

  operator bool() const noexcept { return offset_ != NULLPTR_OFFSET; }
  explicit operator void*() const noexcept { return get(); }
  explicit operator void const*() const noexcept { return get(); }
  T* get() const noexcept {
    auto const ptr =
        offset_ == NULLPTR_OFFSET
            ? nullptr
            : reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset_);
    return ptr;
  }

  friend bool operator==(std::nullptr_t, offset_ptr const& o) noexcept {
    return o.offset_ == NULLPTR_OFFSET;
  }
  friend bool operator==(offset_ptr const& o, std::nullptr_t) noexcept {
    return o.offset_ == NULLPTR_OFFSET;
  }
  friend bool operator!=(std::nullptr_t, offset_ptr const& o) noexcept {
    return o.offset_ != NULLPTR_OFFSET;
  }
  friend bool operator!=(offset_ptr const& o, std::nullptr_t) noexcept {
    return o.offset_ != NULLPTR_OFFSET;
  }

  offset_t offset_{NULLPTR_OFFSET};
};

template <class T>
struct is_pointer_helper : std::false_type {};

template <class T>
struct is_pointer_helper<T*> : std::true_type {};

template <class T>
struct is_pointer_helper<offset_ptr<T>> : std::true_type {};

template <class T>
constexpr bool is_pointer_v = is_pointer_helper<std::remove_cv_t<T>>::value;

template <class T>
struct remove_pointer_helper {
  using type = T;
};

template <class T>
struct remove_pointer_helper<T*> {
  using type = T;
};

template <class T>
struct remove_pointer_helper<offset_ptr<T>> {
  using type = T;
};

template <class T>
struct remove_pointer : remove_pointer_helper<std::remove_cv_t<T>> {};

template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

}  // namespace cista
