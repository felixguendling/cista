#pragma once

#include <cinttypes>

#include "cista/containers/offset_ptr.h"

namespace cista {

template <typename T, typename Ptr = T*>
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
    o.self_allocated_ = false;
    return *this;
  }

  ~unique_ptr() {
    if (self_allocated_ && el_ != nullptr) {
      delete get();
    }
  }

  T* get() { return el_; }
  T const* get() const { return el_; }
  T* operator->() { return el_; }
  T& operator*() { return *el_; }
  T const& operator*() const { return *el_; }
  T const* operator->() const { return el_; }

  Ptr el_{nullptr};
  bool self_allocated_{false};
  uint8_t __fill_0__{0};
  uint16_t __fill_1__{0};
  uint32_t __fill_2__{0};
};

template <typename T>
using o_unique_ptr = unique_ptr<T, offset_ptr<T>>;

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return cista::unique_ptr<T>{new T{std::forward<Args>(args)...}, true};
}

template <typename T, typename... Args>
o_unique_ptr<T> make_o_unique(Args&&... args) {
  return cista::o_unique_ptr<T>{new T{std::forward<Args>(args)...}, true};
}

}  // namespace cista
