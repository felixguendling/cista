#include <tuple>

#include "doctest.h"

#include "cista.h"

struct a {
  int i_ = 1;
  int j_ = 2;
  double d_ = 100.0;
  std::string s_ = "hello world";
};

TEST_CASE("to_tuple") {
  a instance;

  CHECK(instance.i_ == 1);
  CHECK(instance.j_ == 2);
  CHECK(instance.d_ == 100.0);
  CHECK(instance.s_ == "hello world");

  auto t = cista::to_tuple(instance);
  std::get<0>(t) = 5;
  std::get<1>(t) = 7;
  std::get<2>(t) = 2.0;
  std::get<3>(t) = "yeah";

  CHECK(instance.i_ == 5);
  CHECK(instance.j_ == 7);
  CHECK(instance.d_ == 2.0);
  CHECK(instance.s_ == "yeah");
}
