#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

TEST_CASE("struct serialization") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      cista::raw::string d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, cista::raw::string{"testtes"}}};
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me));

  auto const serialized =
      cista::deserialize<serialize_me>(&buf[0], &buf[0] + buf.size());
  CHECK(serialized->a_ == 1);
  CHECK(serialized->j_.b_ == 2);
  CHECK(serialized->j_.c_ == 3);
  CHECK(serialized->j_.d_ == cista::raw::string{"testtes"});
}
