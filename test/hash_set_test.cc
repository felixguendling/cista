#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/hash_map.h"
#include "cista/containers/hash_set.h"
#endif

TEST_CASE("hash_set test") {
  auto const max = 250;
  cista::hash_set<int> uut;
  for (auto i = 0; i < max; ++i) {
    auto const res = uut.emplace(i);
    CHECK(res.second);
    CHECK(*res.first == i);
    CHECK(uut.find(i) != uut.end());
  }
  for (auto i = 0; i < max; ++i) {
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

TEST_CASE("hash_map test") {
  auto const max = 250;
  cista::hash_map<int, int> uut;
  for (auto i = 0; i < max; ++i) {
    auto const res = uut.emplace(i, i + 1);
    CHECK(res.second);
    CHECK(res.first->first == i);
    CHECK(res.first->second == i + 1);
    CHECK(uut.find(i) != uut.end());
  }
  for (auto i = 0; i < max; ++i) {
    auto const res = uut.emplace(i, i + 1);
    CHECK(!res.second);
    CHECK(res.first->first == i);
    CHECK(res.first->second == i + 1);
    if (i % 2 == 0) {
      uut.erase(i);
    }
  }
  for (auto it = begin(uut); it != end(uut); ++it) {
    CHECK(it->first % 2 != 0);
    CHECK(uut.find(it->first)->first == it->first);
    CHECK(uut.find(it->first)->second == it->second);
    uut.erase(it);
  }
  CHECK(uut.empty());
}