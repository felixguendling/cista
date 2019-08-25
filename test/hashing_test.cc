#include <queue>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/hash_map.h"
#include "cista/containers/string.h"
#endif

namespace data = cista::raw;

struct key {
  unsigned i_;
  data::string s_;
};

TEST_CASE("automatic hashing and equality check") {
  data::hash_map<key, int> m;
  for (auto i = 0U; i < 100; ++i) {
    m.emplace(key{i, std::to_string(i)}, i + 1);
  }
  CHECK(m.size() == 100);
  for (auto const& [k, v] : m) {
    CHECK(k.i_ == v - 1);
    CHECK(k.i_ == std::stoi(k.s_.str()));
  }
}