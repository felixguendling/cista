#pragma once

#include "cista/offset_t.h"

namespace cista {

template <typename T>
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

  offset_ptr(offset_ptr const& o) : offset_ptr{o.get()} {}
  offset_ptr(offset_ptr&& o) : offset_ptr{o.get()} {}
  offset_ptr& operator=(offset_ptr const& o) {
    offset_ = ptr_to_offset(o.get());
  }
  offset_ptr& operator=(offset_ptr&& o) { offset_ = ptr_to_offset(o.get()); }

  offset_t ptr_to_offset(T const* p) const {
    return p == nullptr
               ? NULLPTR_OFFSET
               : static_cast<offset_t>(reinterpret_cast<uint8_t const*>(p) -
                                       reinterpret_cast<uint8_t const*>(this));
  }

  operator T*() { return get(); }
  operator T const*() const { return get(); }
  T& operator*() { return *get(); }
  T const& operator*() const { return *get(); }

  T const* get() const {
    return offset_ == NULLPTR_OFFSET
               ? nullptr
               : reinterpret_cast<T const*>(
                     reinterpret_cast<uint8_t const*>(this) + offset_);
  }
  T* get() {
    return offset_ == NULLPTR_OFFSET
               ? nullptr
               : reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) +
                                      offset_);
  }

  template <typename Int>
  offset_ptr operator+(Int i) const {
    offset_ptr r = *this;
    r.offset_ += i * sizeof(T);
    return r;
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

  offset_t offset_;
};

}  // namespace cista