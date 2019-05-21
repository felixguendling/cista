#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/offset_ptr.h"
#endif

TEST_CASE("offset ptr to void test") {
  auto const a = 77;
  auto const ptr = cista::offset_ptr<void>{&a};
  CHECK(*reinterpret_cast<int const*>(ptr.get()) == 77);
}