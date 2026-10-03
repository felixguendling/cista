#include <cstdint>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace {
using byte_vector = cista::offset::vector<std::uint8_t>;

template <typename Vector, cista::mode Mode>
void check_round_trip() {
  for (auto const size : {0U, 256U, 65536U}) {
    Vector source;
    for (auto i = 0U; i != size; ++i) {
      source.push_back(static_cast<typename Vector::value_type>(i));
    }
    auto bytes = cista::serialize<Mode>(source);
    auto const loaded =
        cista::deserialize<Vector, Mode | cista::mode::DEEP_CHECK>(bytes);
    REQUIRE(loaded != nullptr);
    CHECK(*loaded == source);
  }
}
}  // namespace

TEST_CASE_TEMPLATE("byte vector serialization round trips", Vector, byte_vector,
                   cista::offset::vector<std::int8_t>,
                   cista::offset::vector<char>, cista::offset::vector<bool>,
                   cista::raw::vector<std::uint8_t>,
                   cista::raw::vector<std::int8_t>, cista::raw::vector<char>,
                   cista::raw::vector<bool>) {
  check_round_trip<Vector, cista::mode::NONE>();
  check_round_trip<Vector, cista::mode::SERIALIZE_BIG_ENDIAN>();
}

TEST_CASE("byte vector recurse visits every element") {
  byte_vector bytes{1, 2, 3};
  auto context = 0;
  auto visited = 0;
  cista::recurse(context, &bytes, [&](auto* byte) {
    ++visited;
    ++*byte;
  });
  CHECK(visited == 3);
  CHECK(bytes == byte_vector{2, 3, 4});
}

TEST_CASE("byte vector deserialization rejects malformed buffers") {
  byte_vector source{1, 2, 3};
  auto bytes = cista::serialize(source);
  SUBCASE("truncated payload") { bytes.pop_back(); }
  SUBCASE("inconsistent sizes") {
    reinterpret_cast<byte_vector*>(bytes.data())->used_size_ = 2;
  }
  CHECK_THROWS(
      (cista::deserialize<byte_vector, cista::mode::DEEP_CHECK>(bytes)));
}

TEST_CASE("indexed byte vector preserves borrowed element pointers") {
  struct data {
    cista::offset::indexed_vector<std::uint8_t> bytes;
    cista::offset::ptr<std::uint8_t> selected;
  } source;
  source.bytes = {1, 2, 3};
  source.selected = &source.bytes[1];
  auto bytes = cista::serialize(source);
  auto const loaded = cista::deserialize<data, cista::mode::DEEP_CHECK>(bytes);
  REQUIRE(loaded != nullptr);
  CHECK(loaded->selected.get() == &loaded->bytes[1]);
  CHECK(*loaded->selected == 2);
}
