#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers.h"
#endif

TEST_CASE("to_tuple") {
  namespace data = cista::offset;
  data::vector<uint32_t> a;
  a.emplace_back(1u);
  a.emplace_back(2u);
  a.emplace_back(3u);

  data::vector<uint32_t> b;
  b = a;

  CHECK(a == b);
}