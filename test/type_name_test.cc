#include "doctest.h"

#include "cista.h"

#ifdef _MSC_VER
TEST_CASE("canonicalize type name test") {
  std::string msvc =
      R"(struct cista::basic_vector<struct cista::basic_unique_ptr<struct graphns::offset::node,struct cista::offset_ptr<struct graphns::offset::node> >,struct cista::offset_ptr<struct cista::basic_unique_ptr<struct graphns::offset::node,struct cista::offset_ptr<struct graphns::offset::node> > >,unsigned int>)";

  std::string clang =
      R"(cista::basic_vector<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> >, cista::offset_ptr<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> > >, unsigned int>)";

  std::string gcc =
      R"(cista::basic_vector<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> >, cista::offset_ptr<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> > >, unsigned int>)";

  cista::canonicalize_type_name(msvc);

  CHECK(clang == gcc);
  CHECK(clang == msvc);

  CHECK(4753065846318391081ULL == cista::hash(msvc));
  CHECK(4753065846318391081ULL == cista::hash(clang));
  CHECK(4753065846318391081ULL == cista::hash(gcc));
}
#endif