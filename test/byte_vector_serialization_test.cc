#include <cstdint>
#include <limits>

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
    if (!source.empty()) {
      source[0] = std::numeric_limits<typename Vector::value_type>::max();
      source[1] = std::numeric_limits<typename Vector::value_type>::min();
    }
    auto bytes = cista::serialize<Mode>(source);
    auto const loaded =
        cista::deserialize<Vector, Mode | cista::mode::DEEP_CHECK>(bytes);
    REQUIRE(loaded != nullptr);
    CHECK(*loaded == source);
  }
}
}  // namespace

TEST_CASE_TEMPLATE(
    "integer vector serialization round trips", Vector, byte_vector,
    cista::offset::vector<std::int8_t>, cista::offset::vector<std::uint16_t>,
    cista::offset::vector<std::int16_t>, cista::offset::vector<std::uint32_t>,
    cista::offset::vector<std::int32_t>, cista::offset::vector<std::uint64_t>,
    cista::offset::vector<std::int64_t>, cista::offset::vector<char>,
    cista::offset::vector<bool>, cista::raw::vector<std::uint8_t>,
    cista::raw::vector<std::int8_t>, cista::raw::vector<std::uint16_t>,
    cista::raw::vector<std::int16_t>, cista::raw::vector<std::uint32_t>,
    cista::raw::vector<std::int32_t>, cista::raw::vector<std::uint64_t>,
    cista::raw::vector<std::int64_t>, cista::raw::vector<char>,
    cista::raw::vector<bool>) {
  check_round_trip<Vector, cista::mode::NONE>();
  check_round_trip<Vector, cista::mode::SERIALIZE_BIG_ENDIAN>();
}

TEST_CASE_TEMPLATE("integer vector serialized byte order", T, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
  auto const value = static_cast<T>(0x0102030405060708ULL);
  cista::offset::vector<T> source{value};
  auto const little = cista::serialize(source);
  auto const big = cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(source);
  for (auto i = 0U; i != sizeof(T); ++i) {
    CHECK(little[little.size() - sizeof(T) + i] ==
          static_cast<std::uint8_t>(value >> (i * 8U)));
    CHECK(big[big.size() - sizeof(T) + i] ==
          static_cast<std::uint8_t>(value >> ((sizeof(T) - 1U - i) * 8U)));
  }
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

TEST_CASE_TEMPLATE("integer vector deserialization rejects malformed buffers",
                   Vector, byte_vector, cista::offset::vector<std::uint16_t>,
                   cista::offset::vector<std::uint32_t>,
                   cista::offset::vector<std::uint64_t>) {
  Vector source{1, 2, 3};
  auto bytes = cista::serialize(source);
  SUBCASE("truncated payload") { bytes.pop_back(); }
  SUBCASE("inconsistent sizes") {
    reinterpret_cast<Vector*>(bytes.data())->used_size_ = 2;
  }
  CHECK_THROWS((cista::deserialize<Vector, cista::mode::DEEP_CHECK>(bytes)));
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
