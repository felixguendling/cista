#include <fstream>
#include <iostream>

#include "cista.h"

#include "doctest.h"

using namespace cista;

TEST_CASE("file") {
  struct test {
    int a = 1;
    int b = 2;
    int c = 3;
  } t;

  struct test1 {
    uint64_t number = 77;
  } t1;

  {
    sfile f{"test.bin", "wb"};
    auto start = f.write(&t, sizeof(t), std::alignment_of_v<test>);
    for_each_field(t, [start, i = 11, &t, &f](auto&& m) mutable {
      f.write(start + static_cast<offset_t>(reinterpret_cast<char const*>(&m) -
                                            reinterpret_cast<char const*>(&t)),
              i++);
    });

    start = f.write(&t1, sizeof(t1), std::alignment_of_v<test1>);
    CHECK(start == 16);
  }

  int i;
  std::ifstream f{"test.bin", std::ios_base::binary};
  for (int j = 0; j < 3; ++j) {
    f.read(reinterpret_cast<char*>(&i), sizeof(i));
    CHECK(i == 11 + j);
  }

  f.read(reinterpret_cast<char*>(&i), sizeof(i));

  uint64_t number;
  f.read(reinterpret_cast<char*>(&number), sizeof(number));
  CHECK(number == 77);
}
