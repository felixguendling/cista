#include "doctest.h"

#include "cista.h"

TEST_CASE("type_hash<int>") { CHECK(cista::type_str<int>() == "int"); }