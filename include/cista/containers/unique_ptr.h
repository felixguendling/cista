#pragma once

#include <cinttypes>
#include <cstddef>

#include <utility>

#include "cista/containers/ptr.h"

namespace cista {

template <typename T, typename Ptr = T*>
struct basic_unique_ptr {
  using value_t = T;

  basic_unique_ptr() = default;

  explicit basic_unique_ptr(T* el, bool take_ownership = true)
      : el_{el}, self_allocated_{take_ownership} {}

  basic_unique_ptr(basic_unique_ptr const&) = delete;
  basic_unique_ptr& operator=(basic_unique_ptr const&) = delete;

  basic_unique_ptr(basic_unique_ptr&& o) noexcept
      : el_{o.el_}, self_allocated_{o.self_allocated_} {
    o.el_ = nullptr;
    o.self_allocated_ = false;
  }

  basic_unique_ptr& operator=(basic_unique_ptr&& o) noexcept {
    el_ = o.el_;
    self_allocated_ = o.self_allocated_;
    o.el_ = nullptr;
    o.self_allocated_ = false;
    return *this;
  }

  basic_unique_ptr(std::nullptr_t) noexcept {}
  basic_unique_ptr& operator=(std::nullptr_t) {
    reset();
    return *this;
  }

  ~basic_unique_ptr() { reset(); }

  void reset() {
    if (self_allocated_ && el_ != nullptr) {
      delete el_;
      el_ = nullptr;
      self_allocated_ = false;
    }
  }

  explicit operator bool() const noexcept { return el_ != nullptr; }

  friend bool operator==(basic_unique_ptr const& a, std::nullptr_t) noexcept {
    return a.el_ == nullptr;
  }
  friend bool operator==(std::nullptr_t, basic_unique_ptr const& a) noexcept {
    return a.el_ == nullptr;
  }
  friend bool operator!=(basic_unique_ptr const& a, std::nullptr_t) noexcept {
    return a.el_ != nullptr;
  }
  friend bool operator!=(std::nullptr_t, basic_unique_ptr const& a) noexcept {
    return a.el_ != nullptr;
  }

  T* get() const noexcept { return el_; }
  T* operator->() noexcept { return el_; }
  T& operator*() noexcept { return *el_; }
  T const& operator*() const noexcept { return *el_; }
  T const* operator->() const noexcept { return el_; }

  Ptr el_{nullptr};
  bool self_allocated_{false};
  uint8_t __fill_0__{0};
  uint16_t __fill_1__{0};
  uint32_t __fill_2__{0};
};

namespace raw {

template <typename T>
using unique_ptr = basic_unique_ptr<T, ptr<T>>;

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>{new T{std::forward<Args>(args)...}, true};
}

}  // namespace raw

namespace offset {

template <typename T>
using unique_ptr = basic_unique_ptr<T, ptr<T>>;

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>{new T{std::forward<Args>(args)...}, true};
}

}  // namespace offset

}  // namespace cista
