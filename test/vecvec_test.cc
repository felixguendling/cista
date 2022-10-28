#include <memory>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/vecvec.h"
#endif

TEST_CASE("vecvec insert begin test") {
  using key = cista::strong<unsigned, struct x_>;
  struct value {
    double lat_, lng_;
  };
  using data = cista::raw::vecvec<key, value>;

  data d;
  CHECK(d.size() == 0U);
  CHECK(d.empty());

  d.emplace_back(
      std::initializer_list<value>{value{1.0, 1.0}, value{2.0, 2.0}});
  CHECK(d.size() == 1);
  CHECK(d[key{0}].size() == 2);
}

TEST_CASE("vecvec string test") {
  using key = cista::strong<unsigned, struct x_>;
  using data = cista::raw::vecvec<key, char>;

  data d;
  d.emplace_back("hello");
  CHECK(d[key{0U}].view() == "hello");
}