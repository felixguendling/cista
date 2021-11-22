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
  CHECK(std_bits.to_string() == cista_bits.to_string());
  auto const mod_3_std = std_bits;
  auto const mod_3_cista = cista_bits;

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
  CHECK(std_bits.to_string() == cista_bits.to_string());
  auto const mod_5_std = std_bits;
  auto const mod_5_cista = cista_bits;

  CHECK((~mod_3_std).to_string() == (~mod_3_cista).to_string());

  auto mod_15_cista = mod_5_cista;
  mod_15_cista &= mod_3_cista;
  auto mod_15_std = mod_5_std;
  mod_15_std &= mod_3_std;
  CHECK(mod_15_cista.to_string() == mod_15_std.to_string());

  auto mod_3_or_5_cista = mod_5_cista;
  mod_3_or_5_cista |= mod_3_cista;
  auto mod_3_or_5_std = mod_5_std;
  mod_3_or_5_std |= mod_3_std;
  CHECK(mod_3_or_5_cista.to_string() == mod_3_or_5_std.to_string());

  auto x = mod_3_or_5_cista;
  mod_3_or_5_cista ^= mod_15_cista;
  auto y = mod_3_or_5_std;
  mod_3_or_5_std ^= mod_15_std;
  CHECK(x.to_string() == y.to_string());

  mod_3_or_5_cista >>= 67U;
  mod_3_or_5_std >>= 67U;
  CHECK(mod_3_or_5_cista.to_string() == mod_3_or_5_std.to_string());

  mod_3_or_5_cista <<= 67U;
  mod_3_or_5_std <<= 67U;
  CHECK(mod_3_or_5_cista.to_string() == mod_3_or_5_std.to_string());
}

TEST_CASE("shift right") {
  auto const z =
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "00000";
  auto a = cista::bitset<77>{std::string_view{z}};
  auto b = std::bitset<77>{std::string{z}};
  a >>= 48;
  b >>= 48;
  CHECK(a.to_string() == b.to_string());
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