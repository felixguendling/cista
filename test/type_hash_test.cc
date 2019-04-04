#include "doctest.h"

#include "cista.h"

TEST_CASE("hash test") {
  struct test {
    int i;
    int* j;
  } a;

  int b;
  CHECK(cista::type_hash(a) != cista::type_hash(b));
}

TEST_CASE("hash test") {
  struct s1 {
    int i;
    struct {
      int j;
    } k;
  } a;
  struct s2 {
    struct {
      int i;
    } j;
    int k;
  } b;
  CHECK(cista::type_hash(a) != cista::type_hash(b));
  CHECK(0xC2E1EA98D8A77A15 == cista::type_hash(a));
  CHECK(0x5576E826CC0B117D == cista::type_hash(b));
}