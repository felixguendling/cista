#include <string>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

TEST_CASE("tuple get") {
  auto t = cista::tuple{1, .1, 'a'};
  cista::get<2>(t) = 'B';
  CHECK(cista::get<0>(t) == 1);
  CHECK(cista::get<1>(t) == .1);
  CHECK(cista::get<2>(t) == 'B');
  CHECK(cista::tuple_size_v<decltype(t)> == 3U);
  cista::apply([](auto&... f) { ((f = {}), ...); }, t);
  CHECK(cista::get<0>(t) == 0);
  CHECK(cista::get<1>(t) == 0);
  CHECK(cista::get<2>(t) == '\0');
}

TEST_CASE("tuple get with same types") {
  auto t = cista::tuple{1, 42, 1336, 11};

  cista::get<2>(t) += 1;

  CHECK(cista::get<0>(t) == 1);
  CHECK(cista::get<1>(t) == 42);
  CHECK(cista::get<2>(t) == 1337);
  CHECK(cista::get<3>(t) == 11);
  CHECK(cista::tuple_size_v<decltype(t)> == 4U);
  cista::apply([](auto&... f) { ((f = {}), ...); }, t);
  CHECK(cista::get<0>(t) == 0);
  CHECK(cista::get<1>(t) == 0);
  CHECK(cista::get<2>(t) == 0);
  CHECK(cista::get<3>(t) == 0);
}

TEST_CASE("tuple get with same type") {
  auto t = cista::tuple{1, 42, 1336, 11};
  auto& [t0, t1, t2, t3] = t;
  t2 += 1;
  CHECK(t0 == 1);
  CHECK(t1 == 42);
  CHECK(t2 == 1337);
  CHECK(t3 == 11);
}

TEST_CASE("tuple comparison") {
  auto a = cista::tuple{2, 'a'};
  auto b = cista::tuple{1, 'b'};
  CHECK(!(a == b));
  CHECK(a != b);
  CHECK(b < a);
  CHECK(b <= a);
  CHECK(a > b);
  CHECK(!(b >= a));
}

TEST_CASE("tuple serialize") {
  namespace data = cista::offset;
  using serialize_me = cista::tuple<
      data::vector<cista::tuple<data::string, data::hash_set<data::string>>>,
      int>;
  constexpr auto const MODE = cista::mode::WITH_VERSION;

  cista::byte_buf buf;
  {
    serialize_me obj;
    std::get<0>(obj).emplace_back(
        cista::tuple{data::string{"hello"},
                     data::hash_set<data::string>{
                         data::string{"1"},
                         data::string{"this is a very very very long string"},
                         data::string{"3"}}});
    std::get<0>(obj).emplace_back(
        cista::tuple{data::string{"world"},
                     data::hash_set<data::string>{
                         data::string{"4"},
                         data::string{"this is a very very very long string"},
                         data::string{"6"}}});
    std::get<1>(obj) = 55;
    buf = cista::serialize<MODE>(obj);
  }  // EOL obj

  auto const& serialized =
      *cista::unchecked_deserialize<serialize_me, MODE>(buf);
  CHECK(std::get<0>(serialized).at(0) ==
        cista::tuple{data::string{"hello"},
                     data::hash_set<data::string>{
                         data::string{"1"},
                         data::string{"this is a very very very long string"},
                         data::string{"3"}}});
  CHECK(std::get<0>(serialized).at(1) ==
        cista::tuple{data::string{"world"},
                     data::hash_set<data::string>{
                         data::string{"4"},
                         data::string{"this is a very very very long string"},
                         data::string{"6"}}});
  CHECK(std::get<1>(serialized) == 55);
}
