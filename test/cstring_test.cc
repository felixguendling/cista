#include <iostream>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/cstring.h"
#include "cista/hash.h"
#endif

using cista::raw::cstring;

constexpr auto const CORNER_CASE_SHORT_14 = "01234567891234";
constexpr auto const CORNER_CASE_SHORT_15 = "012345678912345";
constexpr auto const CORNER_CASE_LONG_16 = "0123456789123456";
constexpr auto const LONG_STR = "hello world hello world";
constexpr auto const SHORT_STR = "hello world";

TEST_CASE("cstring init") {
  auto s = cstring{};
  CHECK(s.is_short());
  CHECK(s.size() == 0);
  CHECK(s.data() != nullptr);
}

TEST_CASE("cstring long short corner 14") {
  auto s = cstring{CORNER_CASE_SHORT_14, cstring::owning};
  CHECK(s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_SHORT_14));
  CHECK(s.view() == CORNER_CASE_SHORT_14);
}

TEST_CASE("cstring long short corner 15") {
  auto s = cstring{CORNER_CASE_SHORT_15, cstring::owning};
  CHECK(s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_SHORT_15));
  CHECK(s.view() == CORNER_CASE_SHORT_15);
}

TEST_CASE("cstring long short corner 16") {
  auto s = cstring{CORNER_CASE_LONG_16, cstring::owning};
  CHECK(!s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_LONG_16));
  CHECK(s.view() == CORNER_CASE_LONG_16);
}

TEST_CASE("cstring long short") {
  auto s = cstring{SHORT_STR, cstring::owning};
  CHECK(s.view() == SHORT_STR);
  CHECK(s.is_short());

  s.set_owning(CORNER_CASE_LONG_16);
  CHECK(!s.is_short());
  CHECK(s.view() == CORNER_CASE_LONG_16);

  s.set_owning(LONG_STR);
  CHECK(!s.is_short());
  CHECK(s.view() == LONG_STR);
}

TEST_CASE("cstring dealloc long to short") {
  cstring s = "one two";
  CHECK(s.size() == std::strlen("one two"));
  CHECK(s.is_short());
  s.set_non_owning("");
}

TEST_CASE("cstring copy assign and copy construct") {
  auto s0 = cstring{LONG_STR, cstring::owning};
  auto s1 = cstring{s0};
  CHECK(s0 == s1);
  CHECK(s1.view() == LONG_STR);

  cstring s2;
  s2 = s0;
  CHECK(s0 == s2);
  CHECK(s2.view() == LONG_STR);
}

TEST_CASE("cstring hash") {
  auto str = cstring{""};
  auto h = cista::hash(str, cista::BASE_HASH);
  CHECK(cista::BASE_HASH == h);
}
