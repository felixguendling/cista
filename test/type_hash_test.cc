#include "doctest.h"

#include "cista.h"

namespace hash_test {
static struct s1 {
  int i;
  struct {
    int j;
  } k;
} a;

static struct s2 {
  struct {
    int i;
  } j;
  int k;
} b;
}  // namespace hash_test

TEST_CASE("hash int struct != int") {
  int b;
  CHECK(cista::type_hash(hash_test::a) != cista::type_hash(b));
}

TEST_CASE("hash test struct field order") {
  CHECK(cista::type_hash(hash_test::a) !=
        cista::type_hash(hash_test::b));
  CHECK(3410441071354815250ULL == cista::type_hash(hash_test::a));
  CHECK(3410439971843187039ULL == cista::type_hash(hash_test::b));
}