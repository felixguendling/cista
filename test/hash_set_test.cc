#define DOCTEST_CONFIG_NO_EXCEPTIONS
#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers.h"
#include "cista/hash.h"
#include "cista/serialization.h"
#endif

TEST_CASE("hash_set test") {
  auto const max = 250;
  cista::raw::hash_set<int> uut;
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
  cista::raw::hash_map<int, int> uut;
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

TEST_CASE("serialize hash_set test") {
  using namespace cista;
  using namespace cista::raw;

  struct hash {
    size_t operator()(vector<string> const& v) {
      auto hash = cista::BASE_HASH;
      for (auto const& s : v) {
        hash = cista::hash(s, hash);
      }
      return hash;
    }
  };

  struct eq {
    bool operator()(vector<string> const& a, vector<string> const& b) {
      if (a.size() != b.size()) {
        return false;
      }
      for (auto ia = a.begin(), ib = b.begin(); ia != a.end(); ++ia, ++ib) {
        if (*ia != *ib) {
          return false;
        }
      }
      return true;
    }
  };

  auto const make_e1 = []() {
    vector<string> e1;
    e1.emplace_back(string{"short", string::owning_t{}});
    e1.emplace_back(
        string{"long long long long long long long", string::owning_t{}});
    return e1;
  };

  auto const make_e2 = []() {
    vector<string> e2;
    e2.emplace_back(string{"hello", string::owning_t{}});
    e2.emplace_back(string{"world", string::owning_t{}});
    e2.emplace_back(string{"yeah!", string::owning_t{}});
    return e2;
  };

  auto const make_e3 = []() {
    vector<string> e3;
    e3.emplace_back(string{"This", string::non_owning_t{}});
    e3.emplace_back(string{"is", string::non_owning_t{}});
    e3.emplace_back(string{"Sparta", string::non_owning_t{}});
    e3.emplace_back(string{"!!!", string::non_owning_t{}});
    return e3;
  };

  using serialize_me_t = hash_set<vector<string>, hash, eq>;

  byte_buf buf;

  {
    serialize_me_t s;
    s.emplace(make_e1());
    s.emplace(make_e2());
    s.emplace(make_e3());
    buf = serialize(s);
  }  // EOL s

  auto const deserialized = deserialize<serialize_me_t>(buf);

  CHECK(deserialized->size() == 3U);

  CHECK(deserialized->find(make_e1()) != deserialized->end());
  CHECK(deserialized->find(make_e2()) != deserialized->end());
  CHECK(deserialized->find(make_e3()) != deserialized->end());

  CHECK(*deserialized->find(make_e1()) == make_e1());
  CHECK(*deserialized->find(make_e2()) == make_e2());
  CHECK(*deserialized->find(make_e3()) == make_e3());
}