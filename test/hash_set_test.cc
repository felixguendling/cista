#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/hash_set.h"
#endif

TEST_CASE("hash_set test") {
  cista::hash_set<int> uut;
  for (auto i = 0; i < 1000; ++i) {
    auto const res = uut.emplace(i);
    CHECK(res.second);
    CHECK(*res.first == i);
  }
  for (auto i = 0; i < 1000; ++i) {
    auto const res = uut.emplace(i);
    CHECK(!res.second);
    CHECK(*res.first == i);
    if (i % 2 == 0) {
      uut.erase(i);
    }
  }
  for (auto it = begin(uut); it != end(uut); ++it) {
    CHECK(*it % 2 != 0);
    CHECK(*uut.find(*it) == *it);
    uut.erase(it);
  }
  CHECK(uut.empty());
}