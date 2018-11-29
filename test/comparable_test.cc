#include "doctest.h"

#include "cista.h"

struct a {
  MAKE_COMPARABLE()
  int i = 1;
  int j = 2;
  double d = 100.0;
  std::string s = "hello world";
};

TEST_CASE("comparable") {
  a inst1, inst2;

  CHECK(inst1 == inst2);
  CHECK(!(inst1 != inst2));
  CHECK(inst1 <= inst2);
  CHECK(inst1 >= inst2);
  CHECK(!(inst1 < inst2));
  CHECK(!(inst1 > inst2));

  inst1.j = 1;

  CHECK(!(inst1 == inst2));
  CHECK(inst1 != inst2);
  CHECK(inst1 <= inst2);
  CHECK(!(inst1 >= inst2));
  CHECK(inst1 < inst2);
  CHECK(!(inst1 > inst2));
}
