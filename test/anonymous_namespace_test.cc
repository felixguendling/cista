#include "doctest.h"

#include "cista.h"

namespace {
namespace {
struct type_name {};
}  // namespace
}  // namespace

struct abc {
  struct def {};
};

TEST_CASE("canonicalize typename special cases") {
  CHECK(cista::canonical_type_str<type_name>() == "type_name");
  CHECK(cista::canonical_type_str<abc::def>() == "abc::def");
}