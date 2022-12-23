#pragma once

#include <memory>

#include "cista/next_power_of_2.h"

namespace cista {

template <typename T1, typename T2>
T1 to_next_multiple(T1 const n, T2 const multiple) noexcept {
  auto const r = n % multiple;
  return r == 0 ? n : n + multiple - r;
}

}  // namespace cista

#if CISTA_USE_MIMALLOC

#include "mimalloc.h"
#define CISTA_ALIGNED_ALLOC(alignment, size)                                  \
  (mi_malloc_aligned(                                                         \
      cista::to_next_multiple((size), cista::next_power_of_two((alignment))), \
      cista::next_power_of_two((alignment))))
#define CISTA_ALIGNED_FREE(alignment, ptr) \
  (mi_free_aligned((ptr), cista::next_power_of_two((alignment))))

#elif defined(_MSC_VER)

#define CISTA_ALIGNED_ALLOC(alignment, size) \
  (_aligned_malloc((size), cista::next_power_of_two((alignment))))
#define CISTA_ALIGNED_FREE(alignment, ptr) (_aligned_free((ptr)))

#elif defined(_LIBCPP_HAS_C11_FEATURES) || defined(_GLIBCXX_HAVE_ALIGNED_ALLOC)

#include <memory>
#define CISTA_ALIGNED_ALLOC(alignment, size) \
  (std::aligned_alloc(                       \
      cista::next_power_of_two((alignment)), \
      cista::to_next_multiple((size), cista::next_power_of_two((alignment)))))
#define CISTA_ALIGNED_FREE(alignment, ptr) std::free((ptr))

#else

#include <cstdlib>
#define CISTA_ALIGNED_ALLOC(alignment, size) (std::malloc((size)))
#define CISTA_ALIGNED_FREE(alignment, ptr) (std::free((ptr)))

#endif
