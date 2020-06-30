#pragma once

#include <type_traits>

#include "cista/offset_t.h"

namespace cista {

template <typename T, typename Enable = void>
struct offset_ptr {
  offset_ptr() = default;
  offset_ptr(std::nullptr_t) : offset_{NULLPTR_OFFSET} {}
  offset_ptr(T const* p) : offset_{ptr_to_offset(p)} {}

  offset_ptr& operator=(T const* p) {
    offset_ = ptr_to_offset(p);
    return *this;
  }
  offset_ptr& operator=(std::nullptr_t) {
    offset_ = NULLPTR_OFFSET;
    return *this;
  }

  offset_ptr(offset_ptr const& o) : offset_{ptr_to_offset(o.get())} {}
  offset_ptr(offset_ptr&& o) noexcept : offset_{ptr_to_offset(o.get())} {}
  offset_ptr& operator=(offset_ptr const& o) {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }
  offset_ptr& operator=(offset_ptr&& o) noexcept {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }

  ~offset_ptr() = default;

  offset_t ptr_to_offset(T const* p) const {
    return p == nullptr
               ? NULLPTR_OFFSET
               : static_cast<offset_t>(reinterpret_cast<intptr_t>(p) -
                                       reinterpret_cast<intptr_t>(this));
  }

  explicit operator bool() const { return offset_ != NULLPTR_OFFSET; }
  explicit operator void*() const { return get(); }
  explicit operator void const*() const { return get(); }
  operator T*() const { return get(); }
  T& operator*() const { return *get(); }
  T* operator->() const { return get(); }
  T& operator[](size_t const i) const { return *(get() + i); }

  T* get() const {
    auto const ptr =
        offset_ == NULLPTR_OFFSET
            ? nullptr
            : reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset_);
    return ptr;
  }

  template <typename Int>
  T* operator+(Int i) const {
    return get() + i;
  }

  template <typename Int>
  T* operator-(Int i) const {
    return get() - i;
  }

  offset_ptr& operator++() {
    offset_ = ptr_to_offset(get() + 1);
    return *this;
  }

  offset_ptr& operator--() {
    offset_ = ptr_to_offset(get() - 1);
    return *this;
  }

  offset_ptr operator++(int) const { return offset_ptr{get() + 1}; }
  offset_ptr operator--(int) const { return offset_ptr{get() - 1}; }

  offset_t offset_{NULLPTR_OFFSET};
};

template <typename T>
struct offset_ptr<T, std::enable_if_t<std::is_same_v<void, T>>> {
  offset_ptr() = default;
  offset_ptr(std::nullptr_t) : offset_{NULLPTR_OFFSET} {}
  offset_ptr(T const* p) : offset_{ptr_to_offset(p)} {}

  offset_ptr& operator=(T const* p) {
    offset_ = ptr_to_offset(p);
    return *this;
  }
  offset_ptr& operator=(std::nullptr_t) {
    offset_ = NULLPTR_OFFSET;
    return *this;
  }

  offset_ptr(offset_ptr const& o) : offset_{ptr_to_offset(o.get())} {}
  offset_ptr(offset_ptr&& o) : offset_{ptr_to_offset(o.get())} {}
  offset_ptr& operator=(offset_ptr const& o) {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }
  offset_ptr& operator=(offset_ptr&& o) {
    offset_ = ptr_to_offset(o.get());
    return *this;
  }

  offset_t ptr_to_offset(T const* p) const {
    return p == nullptr
               ? NULLPTR_OFFSET
               : static_cast<offset_t>(reinterpret_cast<intptr_t>(p) -
                                       reinterpret_cast<intptr_t>(this));
  }

  operator bool() const { return offset_ != NULLPTR_OFFSET; }
  explicit operator void*() const { return get(); }
  explicit operator void const*() const { return get(); }
  T* get() const {
    auto const ptr =
        offset_ == NULLPTR_OFFSET
            ? nullptr
            : reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset_);
    return ptr;
  }

  friend bool operator==(std::nullptr_t, offset_ptr const& o) {
    return o.offset_ == NULLPTR_OFFSET;
  }
  friend bool operator==(offset_ptr const& o, std::nullptr_t) {
    return o.offset_ == NULLPTR_OFFSET;
  }
  friend bool operator!=(std::nullptr_t, offset_ptr const& o) {
    return o.offset_ != NULLPTR_OFFSET;
  }
  friend bool operator!=(offset_ptr const& o, std::nullptr_t) {
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
  typedef T type;
};

template <class T>
struct remove_pointer_helper<T*> {
  typedef T type;
};

template <class T>
struct remove_pointer_helper<offset_ptr<T>> {
  typedef T type;
};

template <class T>
struct remove_pointer : remove_pointer_helper<std::remove_cv_t<T>> {};

template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

}  // namespace cista
