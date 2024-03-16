#include <array>
#include <memory>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/mmap_vec.h"
#include "cista/containers/vecvec.h"
#include "cista/strong.h"
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

TEST_CASE("vecvec resize test") {
  using key = cista::strong<unsigned, struct x_>;
  using data = cista::raw::vecvec<key, char>;

  data d;
  d.emplace_back("hello");
  d.emplace_back("world");
  d.emplace_back("test");

  d.resize(5);

  auto const expected = std::array<std::string_view, 5>{
      std::string_view{"hello"}, std::string_view{"world"},
      std::string_view{"test"}, std::string_view{""}, std::string_view{""}};
  for (auto i = 0U; i != d.size(); ++i) {
    CHECK((d[key{i}].view() == expected[i]));
  }

  d.resize(1);
  auto const expected_1 =
      std::array<std::string_view, 1>{std::string_view{"hello"}};
  for (auto i = 0U; i != d.size(); ++i) {
    CHECK((d[key{i}].view() == expected_1[i]));
  }
}

TEST_CASE("vecvec mmap") {
  using key = cista::strong<unsigned, struct x_>;
  using idx_t = cista::mmap_vec<cista::base_t<key>>;
  using data_t = cista::mmap_vec<char>;

  auto idx = idx_t{cista::mmap{std::tmpnam(nullptr)}};
  auto data = data_t{cista::mmap{std::tmpnam(nullptr)}};
  auto d =
      cista::basic_vecvec<key, data_t, idx_t>{std::move(data), std::move(idx)};

  d.emplace_back("hello");
  d.emplace_back("world");
  d.emplace_back("test");

  CHECK_EQ("hello", d[key{0}].view());
  CHECK_EQ("world", d[key{1}].view());
  CHECK_EQ("test", d[key{2}].view());

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