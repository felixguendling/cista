#include "doctest.h"

#include <cinttypes>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

using namespace cista;
using namespace cista::raw;

struct serialize_me {
  uint32_t v1_ : 10;
  uint32_t v2_ : 10;
  uint32_t v3_ : 10;
  uint32_t v4_ : 1;
  uint32_t v5_ : 1;
};

template <typename Ctx>
void serialize(Ctx&, serialize_me const*, offset_t const) {}

template <typename Ctx>
void deserialize(Ctx const&, serialize_me*) {}

TEST_CASE("custom struct test") {
  byte_buf buf;

  {
    serialize_me obj{1, 2, 3, 0, 1};
    buf = serialize(obj);
  }  // EOL obj

  auto const serialized = unchecked_deserialize<serialize_me>(buf);
  CHECK(1 == serialized->v1_);
  CHECK(2 == serialized->v2_);
  CHECK(3 == serialized->v3_);
  CHECK(0 == serialized->v4_);
  CHECK(1 == serialized->v5_);
}
