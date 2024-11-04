#pragma once

#include <variant>

#include "cista/buffer.h"
#include "cista/containers/unique_ptr.h"
#include "cista/mmap.h"
#include "cista/targets/buf.h"

namespace cista {

using memory_holder = std::variant<buf<mmap>, buffer, byte_buf>;

template <typename T>
struct wrapped {
  wrapped() = default;
  wrapped(memory_holder mem, T* el) : mem_{std::move(mem)}, el_{el} {
    el_.self_allocated_ = false;
  }
  explicit wrapped(raw::unique_ptr<T> el) : el_{std::move(el)} {}

  void reset() {
    el_->~T();
    if (std::holds_alternative<buffer>(mem_)) {
      std::get<buffer>(mem_) = buffer{};
    } else if (std::holds_alternative<byte_buf>(mem_)) {
      std::get<byte_buf>(mem_) = byte_buf{};
    }
  }

  friend bool operator==(wrapped const& x, std::nullptr_t) {
    return x.el_ == nullptr;
  }

  operator bool() const { return el_.get() != nullptr; }
  T* get() const noexcept { return el_.get(); }
  T* operator->() noexcept { return el_.get(); }
  T const* operator->() const noexcept { return el_.get(); }
  T& operator*() noexcept { return *el_; }
  T const& operator*() const noexcept { return *el_; }

  memory_holder mem_;
  raw::unique_ptr<T> el_;
};

template <typename T>
wrapped(memory_holder, T*) -> wrapped<T>;

template <typename T>
wrapped(memory_holder, raw::unique_ptr<T>) -> wrapped<T>;

}  // namespace cista
