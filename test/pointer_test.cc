#include "cista.h"

#include "doctest.h"

TEST_CASE("pointer serialization") {
  struct serialize_me {
    cista::unique_ptr<int> i_{cista::make_unique<int>(77)};
    int* raw_{i_.get()};
  };

  cista::byte_buf buf;

  {
    serialize_me obj;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const serialized =
      cista::deserialize<serialize_me>(&buf[0], &buf[0] + buf.size());
  CHECK(serialized->raw_ == serialized->i_.get());
  CHECK(*serialized->raw_ == 77);
  CHECK(*serialized->i_.get() == 77);
}
