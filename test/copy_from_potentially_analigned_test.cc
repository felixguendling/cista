#include <string>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

TEST_CASE("copy from aligned") {
  max_align_t t;
  static_assert(sizeof(uint16_t) < sizeof(max_align_t));

  auto const mem = reinterpret_cast<uint16_t*>(&t);
  *mem = 155;

  auto const buf =
      std::string_view{reinterpret_cast<char*>(mem), sizeof(uint16_t)};
  CHECK(155 == cista::copy_from_potentially_unaligned<uint16_t>(buf));
}

TEST_CASE("copy from unaligned") {
  max_align_t t;
  static_assert(sizeof(uint16_t) < sizeof(max_align_t));

  auto const value = uint16_t{155};
  auto const storage = reinterpret_cast<uint8_t*>(&t) + 1;
  std::memcpy(storage, &value, sizeof(value));

  auto const buf =
      std::string_view{reinterpret_cast<char const*>(storage), sizeof(value)};
  CHECK(value == cista::copy_from_potentially_unaligned<uint16_t>(buf));
}
