#include "doctest.h"

#include "cista.h"

TEST_CASE("offset ptr vector") {
  cista::vector<int, cista::offset_ptr<int>> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  int j = 1;
  for (auto const i : vec) {
    CHECK(i == j++);
  }
}