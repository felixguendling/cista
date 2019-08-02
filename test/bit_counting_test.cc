#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/bit_counting.h"
#endif

TEST_CASE("count 32bit 0") {
  uint64_t t = 0;
  CHECK(cista::trailing_zeros(t) == 64);
  CHECK(cista::leading_zeros(t) == 64);
}

TEST_CASE("count 64bit 1") {
  uint64_t t = 0'1;
  CHECK(cista::trailing_zeros(t) == 0);
  CHECK(cista::leading_zeros(t) == 63);
}

TEST_CASE("count 64bit higher") {
  uint64_t t = std::numeric_limits<uint32_t>::max();
  t += 1;
  CHECK(cista::trailing_zeros(t) == 32);
  CHECK(cista::leading_zeros(t) == 31);
}

TEST_CASE("count 64bit any") {
  uint64_t t = 7ULL << 30ULL;
  CHECK(cista::trailing_zeros(t) == 30);
  CHECK(cista::leading_zeros(t) == 31);
}

TEST_CASE("count 32bit 0") {
  uint32_t t = 0;
  CHECK(cista::trailing_zeros(t) == 32);
  CHECK(cista::leading_zeros(t) == 32);
}

TEST_CASE("count 32bit 1") {
  uint32_t t = 0'1;
  CHECK(cista::trailing_zeros(t) == 0);
  CHECK(cista::leading_zeros(t) == 31);
}

TEST_CASE("count 32bit highest") {
  uint32_t t = 1U << 31U;
  CHECK(cista::trailing_zeros(t) == 31);
  CHECK(cista::leading_zeros(t) == 0);
}

TEST_CASE("count 32bit any") {
  uint32_t t = 7U << 30U;
  CHECK(cista::trailing_zeros(t) == 30);
  CHECK(cista::leading_zeros(t) == 0);
}