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
