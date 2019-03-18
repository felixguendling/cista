#include "doctest.h"

#include "cista.h"

struct single_member_struct {
  int i_{7};
};

TEST_CASE("single member struct test") {
  single_member_struct s;
  cista::for_each_field(s, [](auto&& f) { CHECK(f == 7); });
}