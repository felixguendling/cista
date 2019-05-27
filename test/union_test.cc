#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

TEST_CASE("union_test") {
  union union_type {
    int32_t a_;
    float b_;
  };

  cista::byte_buf buf;

  {
    union_type obj;
    obj.b_ = 33.4F;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const deserialize = cista::deserialize<union_type>(buf);
  CHECK(deserialize->b_ == 33.4F);
}