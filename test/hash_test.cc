#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/type_hash/type_name.h"
#endif

TEST_CASE("type_hash<int>") { CHECK(cista::type_str<int>() == "int"); }