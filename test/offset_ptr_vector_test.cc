#include "doctest.h"

#include "cista.h"

TEST_CASE("offset vector serialize") {
  cista::byte_buf buf;
  {
    cista::o_vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    int j = 1;
    for (auto const i : vec) {
      CHECK(i == j++);
    }

    buf = serialize(vec);
  }

  auto const vec = reinterpret_cast<cista::o_vector<int>*>(&buf[0]);
  int j = 1;
  for (auto const i : *vec) {
    CHECK(i == j++);
  }
}

TEST_CASE("offset string serialize") {
  constexpr auto const s = "The quick brown fox jumps over the lazy dog";

  cista::byte_buf buf;
  {
    cista::o_string str{s};
    buf = serialize(str);
  }

  auto const str = reinterpret_cast<cista::o_string*>(&buf[0]);
  CHECK(*str == s);
}