#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/vector.h"
#include "cista/equal_to.h"
#include "cista/is_iterable.h"
#include "cista/reflection/printable.h"
#include "cista/serialization.h"
#include "cista/type_hash/type_name.h"
#endif

TEST_CASE("insert begin test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(begin(v), 0);

  CHECK(v == vector<int>{0, 1, 2, 3});
}

TEST_CASE("insert end test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(end(v), 4);

  CHECK(v == vector<int>{1, 2, 3, 4});
}

TEST_CASE("insert middle test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(begin(v) + 1, 4);

  CHECK(v == vector<int>{1, 4, 2, 3});
}

TEST_CASE("range insert begin test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{8, 9};
  v.insert(begin(v), begin(w), end(w));

  CHECK(v == vector<int>{8, 9, 1, 2, 3});
}

TEST_CASE("range insert end test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{8, 9};
  v.insert(end(v), begin(w), end(w));

  CHECK(v == vector<int>{1, 2, 3, 8, 9});
}

TEST_CASE("range insert middle test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{8, 9};
  v.insert(begin(v) + 1, begin(w), end(w));

  CHECK(v == vector<int>{1, 8, 9, 2, 3});
}

TEST_CASE("range insert nothing") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{};
  v.insert(begin(v) + 1, begin(w), end(w));

  CHECK(v == vector<int>{1, 2, 3});
}

TEST_CASE("erase duplicates") {
  using cista::raw::vector;

  auto v = vector<int>{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 9};
  std::sort(begin(v), end(v));
  v.erase(std::unique(begin(v), end(v)), end(v));

  CHECK(v == vector<int>{1, 2, 3, 4, 5, 6, 9});
}

TEST_CASE("iterable comparison") {
  std::vector<int> std_v{1, 2, 3};
  cista::raw::vector<int> cista_v{1, 2, 3};
  std::set<int> std_s{1, 2, 3};
  CHECK(cista::equal_to<decltype(std_v)>{}(std_v, cista_v));
  CHECK(cista::equal_to<decltype(std_s)>{}(std_s, cista_v));
}

TEST_CASE("to_vec") {
  namespace data = cista::raw;

  cista::buf<std::vector<uint8_t>> buf;
  for (auto i = 0U; i != 3; ++i) {  // Serialize.
    buf.reset();

    auto const v = std::vector<double>({1.0, 2.0});
    auto o = data::to_vec(v);
    CHECK(o == data::to_vec(v, [](auto&& e) { return e; }));
    CHECK(o == data::to_vec(begin(v), end(v), [](auto&& e) { return e; }));
    cista::serialize(buf, o);
  }

  // Deserialize.
  std::string s{reinterpret_cast<char*>(&buf.buf_[0]), buf.buf_.size()};
  auto const& deserialized =
      *cista::deserialize<data::vector<double>>(&s[0], &s[0] + s.size());
  REQUIRE(deserialized.size() == 2);
  CHECK(deserialized[0] == 1.0);
  CHECK(deserialized[1] == 2.0);
}

TEST_CASE("to_indexed_vec") {
  namespace data = cista::raw;

  std::vector<unsigned char> buf;
  {  // Serialize.
    auto const v = std::vector<double>({1.0, 2.0});
    auto o = data::to_indexed_vec(v);
    CHECK(o == data::to_indexed_vec(v, [](auto&& e) { return e; }));
    CHECK(o ==
          data::to_indexed_vec(begin(v), end(v), [](auto&& e) { return e; }));
    buf = cista::serialize(o);
  }

  // Deserialize.
  auto const& deserialized =
      *cista::deserialize<data::indexed_vector<double>>(buf);
  REQUIRE(deserialized.size() == 2);
  CHECK(deserialized[0] == 1.0);
  CHECK(deserialized[1] == 2.0);
}