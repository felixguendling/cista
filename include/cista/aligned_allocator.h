#pragma once

#include <cinttypes>
#include <stdexcept>

#include "cista/aligned_alloc.h"

namespace cista {

template <typename T, std::size_t N = 16>
struct aligned_allocator {
  typedef T value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  typedef T* pointer;
  typedef const T* const_pointer;

  typedef T& reference;
  typedef const T& const_reference;

  inline aligned_allocator() noexcept {}

  template <typename T2>
  inline aligned_allocator(const aligned_allocator<T2, N>&) noexcept {}

  inline ~aligned_allocator() noexcept {}

  inline pointer adress(reference r) { return &r; }

  inline const_pointer adress(const_reference r) const { return &r; }

  inline pointer allocate(size_type n) {
    auto const ptr =
        static_cast<pointer>(CISTA_ALIGNED_ALLOC(N, n * sizeof(value_type)));
    if (ptr == nullptr) {
      throw std::bad_alloc{};
    }
    return ptr;
  }

  inline void deallocate(pointer p, size_type) { CISTA_ALIGNED_FREE(p); }

  inline void construct(pointer p, const value_type& wert) {
    new (p) value_type(wert);
  }

  inline void destroy(pointer p) { p->~value_type(); }

  inline size_type max_size() const noexcept {
    return size_type(-1) / sizeof(value_type);
  }

  template <typename T2>
  struct rebind {
    typedef aligned_allocator<T2, N> other;
  };

  bool operator!=(aligned_allocator<T, N> const&) const { return false; }
  bool operator==(aligned_allocator<T, N> const&) const { return true; }
};

}  // namespace cista