#include <queue>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/hash_map.h"
#include "cista/containers/string.h"
#include "cista/containers/vector.h"
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
  data::hash_map<data::vector<key>, int> m;
  for (auto i = 0U; i < 100; ++i) {
    auto v = data::vector<key>{};
    v.emplace_back(key{i, std::to_string(i)});
    v.emplace_back(key{i + 1, std::to_string(i + 1)});
    v.emplace_back(key{i + 2, std::to_string(i + 2)});
    m.emplace(v, i + 3);
  }
  CHECK(m.size() == 100);
  for (auto const& [k, v] : m) {
    CHECK(k.size() == 3U);
    CHECK(k.at(0).i_ == k.at(0).i_);
    CHECK(k.at(0).i_ == std::stoi(k.at(0).s_.str()));
    CHECK(k.at(1).i_ == k.at(1).i_);
    CHECK(k.at(1).i_ == std::stoi(k.at(1).s_.str()));
    CHECK(k.at(2).i_ == k.at(2).i_);
    CHECK(k.at(2).i_ == std::stoi(k.at(2).s_.str()));
    CHECK(k.at(0).i_ == v - 3);
  }
}