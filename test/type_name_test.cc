#include "doctest.h"

#include "cista.h"

TEST_CASE("canonicalize type name test") {
  std::string msvc =
      R"(struct cista::basic_vector<struct cista::basic_unique_ptr<struct graphns::offset::node,struct cista::offset_ptr<struct graphns::offset::node> >,struct cista::offset_ptr<struct cista::basic_unique_ptr<struct graphns::offset::node,struct cista::offset_ptr<struct graphns::offset::node> > >,unsigned int>)";

  std::string clang =
      R"(cista::basic_vector<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> >, cista::offset_ptr<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> > >, unsigned int>)";

  std::string gcc =
      R"(cista::basic_vector<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> >, cista::offset_ptr<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> > >, unsigned int>)";

  std::string a = msvc;
  cista::canonicalize_type_name(a);
  CHECK(a == clang);
  CHECK(a == gcc);
}

// constexpr gcc =