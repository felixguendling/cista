#include "doctest.h"

#include "cista.h"

struct serialize_me {
  uint32_t v1_ : 10;
  uint32_t v2_ : 10;
  uint32_t v3_ : 10;
  uint32_t v4_ : 1;
  uint32_t v5_ : 1;
};

template <typename Ctx>
void serialize(Ctx&, serialize_me const*, cista::offset_t const) {}

void deserialize(cista::deserialization_context const&, serialize_me*) {}

TEST_CASE("custom struct test") {
  cista::byte_buf buf;

  {
    serialize_me obj{1, 2, 3, 0, 1};
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const serialized =
      cista::deserialize<serialize_me>(&buf[0], &buf[0] + buf.size());
  CHECK(1 == serialized->v1_);
  CHECK(2 == serialized->v2_);
  CHECK(3 == serialized->v3_);
  CHECK(0 == serialized->v4_);
  CHECK(1 == serialized->v5_);
}
