#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/hash_map.h"
#include "cista/containers/string.h"
#endif

namespace data = cista::offset;

TEST_CASE("leak_check") {
  data::hash_map<data::string, uint8_t> m{{"longlonglonglonglonglonglong", 0},
                                          {"short", 0}};
}