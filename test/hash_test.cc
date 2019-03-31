#include "doctest.h"

#include "cista.h"

TEST_CASE("hash test") {
  struct test {
    int i;
    int* j;
  };

  CHECK(cista::type_hash<int>() != cista::type_hash<test>());
}

TEST_CASE("hash test") {
  struct s1 {
    int i;
    struct {
      int j;
    } k;
  };
  struct s2 {
    struct {
      int i;
    } j;
    int k;
  };

  CHECK(cista::type_hash<s1>() != cista::type_hash<s2>());
}