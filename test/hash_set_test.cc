#define DOCTEST_CONFIG_NO_EXCEPTIONS
#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/hash_map.h"
#include "cista/containers/hash_set.h"
#include "cista/hash.h"
#include "cista/serialization.h"
#endif

TEST_CASE("hash_set test delete even") {
  auto const max = 250;
  cista::raw::hash_set<int> uut;
  for (auto j = 0; j < 10; ++j) {
    for (auto i = 0; i < max; ++i) {
      auto const res = uut.emplace(i);
      CHECK(res.second);
      CHECK(*res.first == i);
      CHECK(uut.find(i) != uut.end());
    }
    CHECK(uut.size() == max);
    for (auto i = 0; i < max; ++i) {
      auto const res = uut.emplace(i);
      CHECK(!res.second);
      CHECK(*res.first == i);
      if (i % 2 == 0) {
        uut.erase(i);
      }
    }
    CHECK(uut.size() == max / 2);
    for (auto it = begin(uut); it != end(uut); ++it) {
      CHECK(*it % 2 != 0);
      CHECK(*uut.find(*it) == *it);
      uut.erase(it);
    }
    CHECK(uut.empty());
  }
}

TEST_CASE("hash_set test delete half") {
  auto const max = 250;
  cista::raw::hash_set<int> uut;
  for (auto j = 0; j < 10; ++j) {
    for (auto i = 0; i < max; ++i) {
      auto const res = uut.emplace(i);
      CHECK(res.second);
      CHECK(*res.first == i);
      CHECK(uut.find(i) != uut.end());
    }
    CHECK(uut.size() == max);
    for (auto i = 0; i < max; ++i) {
      auto const res = uut.emplace(i);
      CHECK(!res.second);
      CHECK(*res.first == i);
      if (i >= max / 2) {
        uut.erase(i);
      }
    }
    CHECK(uut.size() == max / 2);
    for (auto it = begin(uut); it != end(uut); ++it) {
      CHECK(*it < max / 2);
      CHECK(*uut.find(*it) == *it);
      uut.erase(it);
    }
    CHECK(uut.empty());
  }
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

TEST_CASE("iterate empty hash_set test") {
  using namespace cista::raw;
  hash_set<vector<string>> v;
  for (auto& e : v) {
    (void)e;
    CHECK(false);
  }
}

TEST_CASE("serialize hash_set test") {
  using namespace cista;
  using namespace cista::raw;

  auto const make_e1 = []() {
    vector<string> e1;
    e1.emplace_back("short");
    e1.emplace_back("long long long long long long long");
    return e1;
  };

  auto const make_e2 = []() {
    vector<string> e2;
    e2.emplace_back("hello");
    e2.emplace_back("world");
    e2.emplace_back("yeah!");
    return e2;
  };

  auto const make_e3 = []() {
    vector<string> e3;
    e3.emplace_back("This");
    e3.emplace_back("is");
    e3.emplace_back("Sparta");
    e3.emplace_back("!!!");
    return e3;
  };

  using serialize_me_t = hash_set<vector<string>>;

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

#ifndef _MSC_VER  // MSVC compiler bug :/
TEST_CASE("string view get") {
  using namespace cista::raw;

  hash_map<std::string, int> s;

  s[{"0"}] = 0;  // Calls std::string constructor with key_t const& overload
  s[string{"1"}] = 1;  // cista::string to std::string_view to std::string
  s["2"] = 2;  // Calls std::string constructor on insertion

  // operator[]
  CHECK(s[{"0"}] == 0);
  CHECK(s[string{"0"}] == 0);
  CHECK(s["0"] == 0);

  CHECK(s[{"1"}] == 1);
  CHECK(s[string{"1"}] == 1);
  CHECK(s["1"] == 1);

  CHECK(s[{"2"}] == 2);
  CHECK(s[string{"2"}] == 2);
  CHECK(s["2"] == 2);

  // .at()
  CHECK(s.at({"0"}) == 0);
  CHECK(s.at(string{"0"}) == 0);
  CHECK(s.at("0") == 0);

  CHECK(s.at({"1"}) == 1);
  CHECK(s.at(string{"1"}) == 1);
  CHECK(s.at("1") == 1);

  CHECK(s.at({"2"}) == 2);
  CHECK(s.at(string{"2"}) == 2);
  CHECK(s.at("2") == 2);

  // .find()
  CHECK(s.find({"0"})->second == 0);
  CHECK(s.find(string{"0"})->second == 0);
  CHECK(s.find("0")->second == 0);

  CHECK(s.find({"1"})->second == 1);
  CHECK(s.find(string{"1"})->second == 1);
  CHECK(s.find("1")->second == 1);

  CHECK(s.find({"2"})->second == 2);
  CHECK(s.find(string{"2"})->second == 2);
  CHECK(s.find("2")->second == 2);

  // .erase()
  CHECK(s.erase({"0"}) == 1);
  CHECK(s.erase(string{"0"}) == 0);
  CHECK(s.erase("0") == 0);

  CHECK(s.erase({"1"}) == 1);
  CHECK(s.erase(string{"1"}) == 0);
  CHECK(s.erase("1") == 0);

  CHECK(s.erase({"2"}) == 1);
  CHECK(s.erase(string{"2"}) == 0);
  CHECK(s.erase("2") == 0);
}
#endif

constexpr auto const kDefault = std::numeric_limits<int>::max();

int make_hash_map_local_struct(int count) {
  struct value {
    std::vector<int> foo_;
    int bar_{kDefault};
  };

  cista::raw::hash_map<int, value> map;
  for (auto i = 0; i < count; ++i) {
    map[i].foo_.push_back(i);
  }

  return static_cast<int>(map.size());
}

TEST_CASE("hash map local struct") {
  CHECK(make_hash_map_local_struct(3) == 3);
}

struct value {
  std::vector<int> foo_;
  int bar_{kDefault};
};

int make_hash_map_global_struct(int count) {
  cista::raw::hash_map<int, value> map;
  for (auto i = 0; i < count; ++i) {
    map[i].foo_.push_back(i);
  }

  return static_cast<int>(map.size());
}

TEST_CASE("hash map global struct") {
  CHECK(make_hash_map_global_struct(3) == 3);
}
