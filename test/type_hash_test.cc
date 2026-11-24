#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/unique_ptr.h"
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

namespace data = cista::offset;

namespace rec_hash_test {

struct B;
struct C;

struct A {
  data::unique_ptr<B> m1_;
  B const* m2_;
};

struct B {
  B const* m1_;
  A const* m2_;
  C const* m3_;
};

struct C {
  A const* m1_;
};

};  // namespace rec_hash_test

TEST_CASE("hash int struct != int") {
  CHECK(cista::type_hash<hash_test::s1>() != cista::type_hash<hash_test::s2>());
}

TEST_CASE("hash test struct field order") {
  auto const is32 = sizeof(cista::offset_t) == 4U;
  CHECK((is32 ? 17656074068566830560ULL : 2060989771674233508ULL) ==
        cista::type_hash<hash_test::s1>());
  CHECK((is32 ? 9596506313732747147ULL : 8501584041991634637ULL) ==
        cista::type_hash<hash_test::s2>());
}

TEST_CASE("recursive type hash does work") {
  CHECK((sizeof(cista::offset_t) == 4U
             ? 14245511089706209772ULL
             : 6208585983120472160ULL) == cista::type_hash<rec_hash_test::A>());
}