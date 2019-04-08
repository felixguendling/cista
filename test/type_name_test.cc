constexpr auto msvc =
    R"(struct cista::basic_vector<struct cista::basic_unique_ptr<struct graphns::offset::node,struct cista::offset_ptr<struct graphns::offset::node> >,struct cista::offset_ptr<struct cista::basic_unique_ptr<struct graphns::offset::node,struct cista::offset_ptr<struct graphns::offset::node> > >,unsigned int>)";

constexpr auto clang =
    R"(cista::basic_vector<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> >, cista::offset_ptr<cista::basic_unique_ptr<graphns::offset::node, cista::offset_ptr<graphns::offset::node> > >, unsigned int>)";

// constexpr gcc =