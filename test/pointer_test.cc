#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace data = cista::raw;

TEST_CASE("pointer serialization") {
  struct serialize_me {
    data::unique_ptr<int> i_{data::make_unique<int>(77)};
    data::ptr<int> raw_{i_.get()};
  };

  cista::byte_buf buf;

  {
    serialize_me obj;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const deserialized = cista::deserialize<serialize_me>(buf);
  CHECK(deserialized->raw_ == deserialized->i_.get());
  CHECK(*deserialized->raw_ == 77);
  CHECK(*deserialized->i_.get() == 77);
}
