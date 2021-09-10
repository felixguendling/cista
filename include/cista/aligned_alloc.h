#pragma once

#include <memory>

#include "cista/next_power_of_2.h"
#include "mimalloc.h"

namespace cista {

template <typename T1, typename T2>
T1 to_next_multiple(T1 const n, T2 const multiple) noexcept {
  auto const r = n % multiple;
  return r == 0 ? n : n + multiple - r;
}

}  // namespace cista

#define CISTA_ALIGNED_ALLOC(alignment, size)                                  \
  (mi_malloc_aligned(                                                         \
      cista::to_next_multiple((size), cista::next_power_of_two((alignment))), \
      cista::next_power_of_two((alignment))))
#define CISTA_ALIGNED_FREE(alignment, ptr) \
  (mi_free_aligned((ptr), cista::next_power_of_two((alignment))))
