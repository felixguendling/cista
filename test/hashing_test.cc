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

struct hash_override {
  cista::hash_t hash() const { return i_; }
  unsigned i_;
};

struct std_hash_key {
  explicit std_hash_key(unsigned i) : i_{i} {}
  unsigned i_;
};

struct key {
  int i_;
  data::string s_;
};

namespace std {
template <>
class hash<std_hash_key> {
public:
  size_t operator()(std_hash_key const& s) const { return s.i_; }
};
}  // namespace std

TEST_CASE("std::hash override") {
  auto k = std_hash_key{4};
  CHECK(cista::hashing<std_hash_key>{}(k) == 4);
}

TEST_CASE("hash() override") {
  hash_override k{7};
  CHECK(cista::hashing<hash_override>{}(k) == 7);
}

TEST_CASE("automatic hash validation") {
  key k{3U, data::string{"1234"}};
  CHECK(cista::hashing<key>{}(k) ==
        cista::hash(data::string{"1234"},
                    cista::hash_combine(cista::BASE_HASH, 3U)));
}

TEST_CASE("automatic hashing and equality check") {
  data::hash_map<data::vector<key>, int> m{
      {data::vector<key>{{-2, std::to_string(-2)},
                         {-1, std::to_string(-1)},
                         {0, std::to_string(0)}},
       1},
      {data::vector<key>{{-1, std::to_string(-1)},
                         {0, std::to_string(0)},
                         {1, std::to_string(1)}},
       2}};

  for (auto i = 0; i <= 100; ++i) {
    m.emplace(data::vector<key>{{i, std::to_string(i)},
                                {i + 1, std::to_string(i + 1)},
                                {i + 2, std::to_string(i + 2)}},
              i + 3);
  }

  CHECK(m.size() == 103);
  CHECK(m[{{100, std::to_string(100)},
           {101, std::to_string(101)},
           {102, std::to_string(102)}}] == 103);
  CHECK(m.at({{100, std::to_string(100)},
              {101, std::to_string(101)},
              {102, std::to_string(102)}}) == 103);
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