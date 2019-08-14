#include <memory>

#include "cista/next_power_of_2.h"

namespace cista {

template <typename T>
T to_next_multiple(T const n, T const multiple) {
  auto const r = n % multiple;
  return r == 0 ? n : n + multiple - r;
}

}  // namespace cista

#if defined(_MSC_VER)
#define CISTA_ALIGNED_ALLOC(alignment, size) \
  (_aligned_malloc((size), cista::next_power_of_two((alignment))))
#define CISTA_ALIGNED_FREE(ptr) (_aligned_free((ptr)))
#elif defined(__APPLE__)
#include <cstdlib>
#define CISTA_ALIGNED_ALLOC(alignment, size) (std::malloc((size)))
#define CISTA_ALIGNED_FREE(ptr) (std::free((ptr)))
#else
#include <memory>
#define CISTA_ALIGNED_ALLOC(alignment, size) \
  (std::aligned_alloc(                       \
      cista::next_power_of_two((alignment)), \
      cista::to_next_multiple((size), cista::next_power_of_two((alignment)))))
#define CISTA_ALIGNED_FREE(ptr) std::free((ptr))
#endif