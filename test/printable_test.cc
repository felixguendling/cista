#include <sstream>

#include "doctest.h"

#include "cista.h"

struct a {
  MAKE_PRINTABLE(a)
  int i_ = 1;
  int j_ = 2;
  double d_ = 100.0;
  std::string s_ = "hello world";
};

TEST_CASE("printable") {
  a instance;
  std::stringstream ss;
  ss << instance;
  CHECK(ss.str() == "{1, 2, 100, hello world}");
}
