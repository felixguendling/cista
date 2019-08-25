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

TEST_CASE("automatic hash validation") {
  key k{3U, data::string{"1234"}};
  CHECK(cista::hashing<key>{}(k) ==
        cista::hash("1234", cista::hash_combine(cista::BASE_HASH, 3U)));
}

TEST_CASE("automatic hashing and equality check") {
  data::hash_map<key, int> m;
  for (auto i = 0U; i < 100; ++i) {
    m.emplace(key{i, data::string{std::to_string(i), data::string::owning}},
              i + 1);
  }
  CHECK(m.size() == 100);
  for (auto const& [k, v] : m) {
    CHECK(k.i_ == v - 1);
    CHECK(k.i_ == std::stoi(k.s_.str()));
  }
}