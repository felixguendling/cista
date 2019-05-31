#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/offset_ptr.h"
#endif

TEST_CASE("offset ptr to void test") {
  auto const a = 77;
  auto const ptr = cista::offset_ptr<void>{&a};
  auto const null = cista::offset_ptr<void>{nullptr};
  CHECK(*reinterpret_cast<int const*>(ptr.get()) == 77);
  CHECK(!static_cast<bool>(null));
  CHECK(static_cast<bool>(ptr));
}