#include "doctest.h"

#include "cista.h"

struct test {
  int i;
  int* j;
};

TEST_CASE("hash test") {
  CHECK(cista::type_hash<int>() != cista::type_hash<test>());
}