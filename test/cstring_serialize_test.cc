#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

constexpr auto const LONG_STR = "aaahelloworldtestaaa";
constexpr auto const SHORT_STR = "ahelloworldtest";

TEST_CASE("cstring serialization long_str") {
  cista::raw::string s = LONG_STR;

  cista::byte_buf buf = cista::serialize(s);
  auto const serialized =
      cista::deserialize<cista::raw::string>(&buf[0], &buf[0] + buf.size());
  CHECK(*serialized == std::string_view{LONG_STR});
}

TEST_CASE("cstring serialization short_str") {
  cista::raw::string s = SHORT_STR;

  cista::byte_buf buf = cista::serialize(s);
  CHECK(buf.size() == sizeof(cista::raw::string));

  auto const serialized =
      cista::deserialize<cista::raw::string>(&buf[0], &buf[0] + buf.size());
  CHECK(*serialized == std::string_view{SHORT_STR});
}