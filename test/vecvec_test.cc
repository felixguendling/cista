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

TEST_CASE("vecvec sort bucket test") {
  using key = cista::strong<unsigned, struct x_>;
  using data = cista::raw::vecvec<key, char>;

  data d;
  d.emplace_back("hello");

  std::sort(begin(d.at(key{0})), end(d.at(key{0})));

  CHECK_EQ("ehllo", d.at(key{0}).view());
}

TEST_CASE("vecvec bucket emplace_back test") {
  using key = cista::strong<unsigned, struct x_>;
  using data = cista::raw::vecvec<key, char>;

  data d;
  d.emplace_back("hello");
  d.emplace_back("world");
  d.emplace_back("test");

  d[key{0}].push_back('x');
  CHECK_EQ("hellox", d[key{0}].view());
  CHECK_EQ("world", d[key{1}].view());
  CHECK_EQ("test", d[key{2}].view());

  d[key{1}].push_back('x');
  CHECK_EQ("hellox", d[key{0}].view());
  CHECK_EQ("worldx", d[key{1}].view());
  CHECK_EQ("test", d[key{2}].view());

  d[key{2}].push_back('x');
  CHECK_EQ("hellox", d[key{0}].view());
  CHECK_EQ("worldx", d[key{1}].view());
  CHECK_EQ("testx", d[key{2}].view());
}