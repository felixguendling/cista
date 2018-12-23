#pragma once

#include <cinttypes>

#include "cista/containers/offset_ptr.h"

namespace cista {

template <typename T, typename Ptr = T*>
struct basic_unique_ptr {
  using value_t = T;

  basic_unique_ptr() = default;

  explicit basic_unique_ptr(T* el, bool take_ownership = true)
      : el_{el}, self_allocated_{take_ownership} {}

  basic_unique_ptr(basic_unique_ptr const&) = delete;
  basic_unique_ptr& operator=(basic_unique_ptr const&) = delete;

  basic_unique_ptr(basic_unique_ptr&& o)
      : el_{o.el_}, self_allocated_{o.self_allocated_} {
    o.el_ = nullptr;
    o.self_allocated_ = false;
  }

  basic_unique_ptr& operator=(basic_unique_ptr&& o) {
    el_ = o.el_;
    self_allocated_ = o.self_allocated_;
    o.el_ = nullptr;
    o.self_allocated_ = false;
    return *this;
  }

  ~basic_unique_ptr() {
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

}  // namespace cista
