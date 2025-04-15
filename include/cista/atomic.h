#pragma once

#include <cinttypes>
#include <algorithm>
#include <atomic>

#if defined(_MSC_VER)
#include "intrin.h"
#endif

namespace cista {

inline std::uint64_t fetch_or(std::uint64_t& block, std::uint64_t const mask) {
#if defined(_MSC_VER)
  return _InterlockedOr64(reinterpret_cast<std::int64_t*>(&block), mask);
#elif defined(__cpp_lib_atomic_ref)
  return std::atomic_ref{block}.fetch_or(mask);
#else
  return __atomic_or_fetch(&block, mask, __ATOMIC_RELAXED);
#endif
}

inline std::uint64_t fetch_and(std::uint64_t& block, std::uint64_t const mask) {
#if defined(_MSC_VER)
  return _InterlockedAnd64(reinterpret_cast<std::int64_t*>(&block), mask);
#elif defined(__cpp_lib_atomic_ref)
  return std::atomic_ref{block}.fetch_and(mask);
#else
  return __atomic_and_fetch(&block, mask, __ATOMIC_RELAXED);
#endif
}

inline std::int16_t fetch_min(std::int16_t& block, std::int16_t const val) {
  // UB due to aliasing but `std::atomic_ref` is not there yet.
  auto const a = reinterpret_cast<std::atomic_int16_t*>(&block);
  auto old = a->load();
  if (old > val) {
    while (!a->compare_exchange_weak(old, std::min(old, val),
                                     std::memory_order_release,
                                     std::memory_order_relaxed)) {
      ;
    }
  }
  return old;
}

inline std::int16_t fetch_max(std::int16_t& block, std::int16_t const val) {
  // UB due to aliasing but `std::atomic_ref` is not there yet.
  auto const a = reinterpret_cast<std::atomic_int16_t*>(&block);
  auto old = a->load();
  if (old > val) {
    while (!a->compare_exchange_weak(old, std::max(old, val),
                                     std::memory_order_release,
                                     std::memory_order_relaxed)) {
      ;
    }
  }
  return old;
}

}  // namespace cista