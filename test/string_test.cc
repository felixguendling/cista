#include <iostream>

#include "cista.h"

#include "doctest.h"

using cista::string;

constexpr auto const CORNER_CASE_SHORT_14 = "01234567891234";
constexpr auto const CORNER_CASE_SHORT_15 = "012345678912345";
constexpr auto const CORNER_CASE_LONG_16 = "0123456789123456";
constexpr auto const LONG = "hello world hello world";
constexpr auto const SHORT = "hello world";

TEST_CASE("string init") {
  auto s = string{};
  CHECK(!s.is_short());
  CHECK(s.size() == 0);
  CHECK(s.data() == nullptr);
}

TEST_CASE("string long short corner 14") {
  auto s = string{CORNER_CASE_SHORT_14, cista::string::owning};
  CHECK(s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_SHORT_14));
  CHECK(s.view() == CORNER_CASE_SHORT_14);
}

TEST_CASE("string long short corner 15") {
  auto s = string{CORNER_CASE_SHORT_15, string::owning};
  CHECK(s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_SHORT_15));
  CHECK(s.view() == CORNER_CASE_SHORT_15);
}

TEST_CASE("string long short corner 16") {
  auto s = string{CORNER_CASE_LONG_16, string::owning};
  CHECK(!s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_LONG_16));
  CHECK(s.view() == CORNER_CASE_LONG_16);
}

TEST_CASE("string long short") {
  auto s = string{SHORT, string::owning};
  CHECK(s.view() == SHORT);
  CHECK(s.is_short());

  s.set_owning(CORNER_CASE_LONG_16);
  CHECK(!s.is_short());
  CHECK(s.view() == CORNER_CASE_LONG_16);

  s.set_owning(LONG);
  CHECK(!s.is_short());
  CHECK(s.view() == LONG);
}

TEST_CASE("string dealloc long to short") {
  string s = "one two";
  CHECK(s.size() == std::strlen("one two"));
  CHECK(s.is_short());
  s.set_non_owning("");
}

TEST_CASE("string copy assign and copy construct") {
  auto s0 = string{LONG, string::owning};
  auto s1 = string{s0};
  CHECK(s0 == s1);
  CHECK(s1.view() == LONG);

  string s2;
  s2 = s0;
  CHECK(s0 == s2);
  CHECK(s2.view() == LONG);
}
