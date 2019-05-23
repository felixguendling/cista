#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/type_hash/type_hash.h"
#endif

namespace hash_test {
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
}  // namespace hash_test

TEST_CASE("hash int struct != int") {
  CHECK(cista::type_hash<hash_test::s1>() != cista::type_hash<hash_test::s2>());
}

TEST_CASE("hash test struct field order") {
  CHECK(6255727812064617762ULL == cista::type_hash<hash_test::s1>());
  CHECK(12953799533913957455ULL == cista::type_hash<hash_test::s2>());
}