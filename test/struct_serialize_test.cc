#include "cista.h"

#include "doctest.h"

TEST_CASE("struct serialization") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      cista::string d_;
    } j;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, cista::string{"testtes"}}};
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me));

  auto const serialized =
      cista::deserialize<serialize_me>(&buf[0], &buf[0] + buf.size());
  CHECK(serialized->a_ == 1);
  CHECK(serialized->j.b_ == 2);
  CHECK(serialized->j.c_ == 3);
  CHECK(serialized->j.d_ == cista::string{"testtes"});
}
