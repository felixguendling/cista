#include "doctest.h"

#include "cista.h"

TEST_CASE("offset ptr vector") {
  cista::o_vector<int> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  int j = 1;
  for (auto const i : vec) {
    CHECK(i == j++);
  }
}

TEST_CASE("offset ptr serialize") {
  cista::byte_buf buf;
  {
    cista::o_vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    buf = serialize(vec);
  }

  auto const vec = reinterpret_cast<cista::o_vector<int>*>(&buf[0]);
  int j = 1;
  for (auto const i : *vec) {
    CHECK(i == j++);
  }
}