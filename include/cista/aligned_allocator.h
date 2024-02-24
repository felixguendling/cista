#pragma once

#include <cinttypes>
#include <stdexcept>

#include "cista/aligned_alloc.h"
#include "cista/exception.h"

namespace cista {

template <typename T, std::size_t N = 16U>
struct aligned_allocator {
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using pointer = T*;
  using const_pointer = const T*;

  using reference = T&;
  using const_reference = const T&;

  aligned_allocator() noexcept = default;

  template <typename T2>
  aligned_allocator(const aligned_allocator<T2, N>&) noexcept {}

  ~aligned_allocator() noexcept = default;

  pointer adress(reference r) const noexcept { return &r; }

  const_pointer adress(const_reference r) const noexcept { return &r; }

  pointer allocate(size_type const n) const {
    auto const ptr =
        static_cast<pointer>(CISTA_ALIGNED_ALLOC(N, n * sizeof(value_type)));
    if (ptr == nullptr) {
      throw_exception(std::bad_alloc{});
    }
    return ptr;
  }

  void deallocate(pointer p, size_type) const { CISTA_ALIGNED_FREE(N, p); }

  void construct(pointer p, const value_type& wert) const {
    new (p) value_type(wert);
  }

  void destroy(pointer p) const { p->~value_type(); }

  size_type max_size() const noexcept {
    return size_type(~0U) / sizeof(value_type);
  }

  template <typename T2>
  struct rebind {
    using other = aligned_allocator<T2, N>;
  };

  bool operator!=(aligned_allocator<T, N> const&) const noexcept {
    return false;
  }
  bool operator==(aligned_allocator<T, N> const&) const noexcept {
    return true;
  }
};

}  // namespace cista
