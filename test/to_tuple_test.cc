#include "doctest.h"

#include "cista.h"

struct a {
  int i = 1;
  int j = 2;
  double d = 100.0;
  std::string s = "hello world";
};

TEST_CASE("to_tuple") {
  a instance;

  CHECK(instance.i == 1);
  CHECK(instance.j == 2);
  CHECK(instance.d == 100.0);
  CHECK(instance.s == "hello world");

  std::get<0>(cista::to_tuple(instance)) = 5;
  std::get<1>(cista::to_tuple(instance)) = 7;
  std::get<2>(cista::to_tuple(instance)) = 2.0;
  std::get<3>(cista::to_tuple(instance)) = "yeah";

  CHECK(instance.i == 5);
  CHECK(instance.j == 7);
  CHECK(instance.d == 2.0);
  CHECK(instance.s == "yeah");
}
