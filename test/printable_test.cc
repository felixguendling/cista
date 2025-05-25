#include <sstream>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/reflection/printable.h"
#endif

namespace {

enum class Color : int { RED, BLUE, GREEN };

struct a {
  CISTA_PRINTABLE(a, )
  int i_ = 1;
  int j_ = 2;
  double d_ = 100.0;
  std::string s_ = "hello world";
  Color c_ = Color::BLUE;
};

}  // namespace

TEST_CASE("printable") {
  a instance;
  std::stringstream ss;
  ss << instance;
  CHECK(ss.str() == "{1, 2, 100, hello world, 1}");
}
