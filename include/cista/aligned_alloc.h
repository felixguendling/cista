#include <memory>

#if defined(_MSC_VER)
#define CISTA_ALIGNED_ALLOC(alignment, size) \
  (_aligned_malloc((size), (alignment)))
#elif defined(__APPLE__)
#include <cstdlib>
#define CISTA_ALIGNED_ALLOC(alignment, size) (std::malloc((size)))
#else
#include <memory>
#define CISTA_ALIGNED_ALLOC(alignment, size) \
  (std::aligned_alloc((alignment), (size)))
#endif