#include "doctest.h"

#include "cista.h"

TEST_CASE("crc64") {
  CHECK(cista::crc64(std::string_view{"123456789"}) == 0xe9c6d914c4b8d9ca);
}