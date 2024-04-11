#include "doctest.h"

#include <array>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/variant.h"
#include "cista/mmap.h"
#include "cista/serialization.h"
#include "cista/targets/file.h"
#endif

namespace data = cista::offset;

namespace variant_test {
enum entry_idx_t {
  A_DEFAULT_CTOR,
  A_COPY_CTOR,
  A_MOVE_CTOR,
  A_MOVE_ASSIGN,
  A_COPY_ASSIGN,
  A_DTOR,
  B_DEFAULT_CTOR,
  B_COPY_CTOR,
  B_MOVE_CTOR,
  B_MOVE_ASSIGN,
  B_COPY_ASSIGN,
  B_DTOR,
  NUM_ENTRIES
};
static std::array<int, NUM_ENTRIES> entries;

struct a {
  a() { ++entries[A_DEFAULT_CTOR]; }
  a(a const&) { ++entries[A_COPY_CTOR]; }
  a(a&&) { ++entries[A_MOVE_CTOR]; }
  a& operator=(a&&) {
    ++entries[A_MOVE_ASSIGN];
    return *this;
  }
  a& operator=(a const&) {
    ++entries[A_COPY_ASSIGN];
    return *this;
  }
  ~a() { ++entries[A_DTOR]; }
};

struct b {
  b() { ++entries[B_DEFAULT_CTOR]; }
  b(b const&) { ++entries[B_COPY_CTOR]; }
  b(b&&) { ++entries[B_MOVE_CTOR]; }
  b& operator=(b&&) {
    ++entries[B_MOVE_ASSIGN];
    return *this;
  }
  b& operator=(b const&) {
    ++entries[B_COPY_ASSIGN];
    return *this;
  }
  ~b() { ++entries[B_DTOR]; }
};

}  // namespace variant_test

using namespace variant_test;

TEST_CASE("variant basic methods") {
  data::variant<a, b> v{a{}}, u{b{}};

  CHECK(entries == std::array<int, NUM_ENTRIES>{1, 0, 1, 0, 0, 1,  //
                                                1, 0, 1, 0, 0, 1});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, a>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 0,  //
                                                0, 0, 0, 0, 0, 0});
  v = b{};
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 1,  //
                                                1, 0, 1, 0, 0, 1});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, b>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 0,  //
                                                0, 0, 0, 0, 0, 0});

  v = a{};
  CHECK(entries == std::array<int, NUM_ENTRIES>{1, 0, 1, 0, 0, 1,  //
                                                0, 0, 0, 0, 0, 1});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, a>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 0,  //
                                                0, 0, 0, 0, 0, 0});

  v = u;
  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, b>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 1,  //
                                                0, 1, 0, 0, 0, 0});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v = a{};
  u = b{};
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.swap(u);
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 1, 0, 0, 1,  //
                                                0, 0, 2, 0, 0, 2});

  v = a{};
  u = a{};
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  v.swap(u);
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 1, 2, 0, 1,  //
                                                0, 0, 0, 0, 0, 0});
}

TEST_CASE("variant comparison") {
  data::variant<int, std::string> v{1}, u{std::string{"hello"}};
  static_assert(cista::variant_size_v<decltype(v)> == 2);

  CHECK(v.index() == 0);
  CHECK(u.index() == 1);
  CHECK(cista::holds_alternative<int>(v));
  CHECK(cista::holds_alternative<std::string>(u));

  CHECK(u > v);
  CHECK(v < u);
  CHECK(u >= v);
  CHECK(v <= u);
  CHECK(v != u);
  CHECK(!(v == u));

  u = 0;
  CHECK(u < v);
  CHECK(v > u);
  CHECK(v >= u);
  CHECK(u <= v);
  CHECK(v != u);
  CHECK(!(v == u));

  CHECK(10 == (v.emplace<int>(0) = 10));
  CHECK(20 == (v.emplace<0>(0) = 20));

  v = 0;
  CHECK(!(u < v));
  CHECK(!(v > u));
  CHECK(v >= u);
  CHECK(u <= v);
  CHECK(v == u);
  CHECK(!(v != u));
}

TEST_CASE("variant get") {
  data::variant<int, std::string> v{1}, u{std::string{"hello"}};

  CHECK(cista::get_if<0>(u) == nullptr);
  CHECK(cista::get_if<1>(v) == nullptr);
  CHECK(*cista::get_if<0>(v) == 1);
  CHECK(*cista::get_if<1>(u) == std::string{"hello"});

  v.swap(u);

  CHECK(std::get<int>(u) == 1);
  CHECK(std::get<std::string>(v) == "hello");
  CHECK(std::get<0>(u) == 1);
  CHECK(std::get<1>(v) == "hello");
}

TEST_CASE("variant get const") {
  data::variant<int, std::string> const v{1}, u{std::string{"hello"}};

  CHECK(cista::get_if<0>(u) == nullptr);
  CHECK(cista::get_if<1>(v) == nullptr);
  CHECK(*cista::get_if<0>(v) == 1);
  CHECK(*cista::get_if<1>(u) == std::string{"hello"});

  CHECK(std::get<int>(v) == 1);
  CHECK(std::get<std::string>(u) == "hello");
  CHECK(std::get<0>(v) == 1);
  CHECK(std::get<1>(u) == "hello");
}

TEST_CASE("variant serialization") {
  namespace data = cista::offset;
  constexpr auto const MODE =  // opt. versioning + check sum
      cista::mode::WITH_VERSION | cista::mode::WITH_INTEGRITY;

  struct pos {
    int x, y;
  };
  using property = data::variant<data::string, std::int64_t>;
  using pos_map =  // Automatic deduction of hash & equality
      data::hash_map<data::vector<pos>, data::hash_set<property>>;

  {  // Serialize.
    auto positions = pos_map{
        {{{1, 2}, {3, 4}},
         {property{data::string{"hello"}}, property{std::int64_t{123}}}},
        {{{5, 6}, {7, 8}},
         {property{std::int64_t{456}}, property{data::string{"world"}}}}};
    cista::buf mmap{cista::mmap{"data"}};
    cista::serialize<MODE>(mmap, positions);
  }

  // Deserialize.
  auto b = cista::file("data", "r").content();
  auto positions = cista::deserialize<pos_map, MODE>(b);

  // Check.
  CHECK(positions->size() == 2);
  auto const one = positions->find(pos_map::key_type{{1, 2}, {3, 4}});
  auto const two = positions->find(pos_map::key_type{{5, 6}, {7, 8}});
  CHECK(one != positions->end());
  CHECK(two != positions->end());
  CHECK(one->second.find(property{data::string{"hello"}}) != end(one->second));
  CHECK(two->second.find(property{std::int64_t{456}}) != end(two->second));
}

TEST_CASE("variant get_if") {
  namespace CISTA = cista::offset;
  struct Test;

  using Variant =
      CISTA::variant<bool, std::int64_t, std::uint64_t, double, CISTA::string,
                     CISTA::unique_ptr<int>, CISTA::unique_ptr<Test>>;

  struct Test : public CISTA::vector<Variant> {};

  auto t1 = Variant{true};
  auto const result = cista::get_if<bool>(t1);
  REQUIRE(result != nullptr);
  CHECK(*result == true);

  auto const t2 = Variant{CISTA::string{"hello test test test test"}};
  auto const str = cista::get_if<CISTA::string>(t2);
  REQUIRE(str != nullptr);
  CHECK(*str == "hello test test test test");
}

TEST_CASE("std visit") {
  std::visit(
      [](auto&& x) {
        if constexpr (std::is_same_v<int, std::decay_t<decltype(x)>>) {
          CHECK(true);
        } else {
          CHECK(false);
        }
      },
      cista::variant<int, float>{1});
}
