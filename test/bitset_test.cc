#include "doctest.h"

#include <bitset>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/bitset.h"
#include "cista/serialization.h"
#endif

TEST_CASE("bitset") {
  constexpr auto const size = 136;

  auto s = std::string{};
  for (int i = 0U; i != size; ++i) {
    s.push_back(i % 3 == 0U ? '1' : '0');
  }

  auto std_bits = std::bitset<size>{&s[0], s.size()};
  auto cista_bits = cista::bitset<size>{std::string_view{s}};

  CHECK(std_bits.size() == cista_bits.size());

  for (auto i = 0U; i != size; ++i) {
    CHECK(std_bits.test(size - i - 1U) == (i % 3U == 0U));
    CHECK(cista_bits.test(size - i - 1U) == (i % 3U == 0U));
  }

  for (auto i = 0U; i != size; ++i) {
    cista_bits.set(i, i % 5 == 0);
    std_bits.set(i, i % 5 == 0);
  }

  for (auto i = 0U; i != size; ++i) {
    CHECK(std_bits.test(i) == (i % 5 == 0U));
    CHECK(cista_bits.test(i) == (i % 5 == 0U));
    CHECK(std_bits[i] == (i % 5 == 0U));
    CHECK(cista_bits[i] == (i % 5 == 0U));
  }
}

TEST_CASE("serialize") {
  constexpr auto const mode =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  constexpr auto const size = 512;
  using bitfield = cista::bitset<size>;

  cista::byte_buf buf;

  auto s = std::string{};
  for (int i = 0U; i != size; ++i) {
    s.push_back(i % 5 == 0U ? '1' : '0');
  }

  {
    auto obj = cista::bitset<size>{std::string_view{s}};
    buf = cista::serialize<mode>(obj);
  }  // EOL obj

  auto const& deserialized = *cista::unchecked_deserialize<bitfield, mode>(buf);
  CHECK(deserialized == bitfield{std::string_view(s)});
}