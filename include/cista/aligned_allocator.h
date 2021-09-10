#pragma once

#include <cinttypes>
#include <stdexcept>

#include "cista/aligned_alloc.h"

namespace cista {

template <typename T, std::size_t N = 16>
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
  inline aligned_allocator(const aligned_allocator<T2, N>&) noexcept {}

  inline ~aligned_allocator() noexcept = default;

  inline pointer adress(reference r) noexcept { return &r; }

  inline const_pointer adress(const_reference r) const noexcept { return &r; }

  inline pointer allocate(size_type n) {
    auto const ptr =
        static_cast<pointer>(CISTA_ALIGNED_ALLOC(N, n * sizeof(value_type)));
    if (ptr == nullptr) {
      throw std::bad_alloc{};
    }
    return ptr;
  }

  inline void deallocate(pointer p, size_type) { CISTA_ALIGNED_FREE(N, p); }

  inline void construct(pointer p, const value_type& wert) {
    new (p) value_type(wert);
  }

  inline void destroy(pointer p) { p->~value_type(); }

  inline size_type max_size() const noexcept {
    return size_type(-1) / sizeof(value_type);
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
