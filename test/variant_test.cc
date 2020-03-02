#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/variant.h"
#endif

namespace data = cista::offset;

TEST_CASE("complex union") {
  data::variant<short, char> v{short{1}}, o{char{'b'}};
  v.apply([](auto&& e) { CHECK(e == 1); });
  v = char{'a'};
  v = o;
  v.apply([](auto&& e) { CHECK(e == 'b'); });
}