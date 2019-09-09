#include "doctest.h"

#include <set>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/vector.h"
#include "cista/is_iterable.h"
#include "cista/reflection/printable.h"
#include "cista/type_hash/type_name.h"
#endif

TEST_CASE("insert begin test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(begin(v), 0);

  CHECK(v == vector<int>{0, 1, 2, 3});
}

TEST_CASE("insert end test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(end(v), 4);

  CHECK(v == vector<int>{1, 2, 3, 4});
}

TEST_CASE("insert middle test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(begin(v) + 1, 4);

  CHECK(v == vector<int>{1, 4, 2, 3});
}

TEST_CASE("iterable comparison") {
  std::vector<int> std_v{1, 2, 3};
  cista::raw::vector<int> cista_v{1, 2, 3};
  std::set<int> std_s{1, 2, 3};
  CHECK(std_v == cista_v);
  CHECK(std_s == cista_v);
}
