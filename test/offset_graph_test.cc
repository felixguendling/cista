#define CISTA_PTR_TYPE offset
#include "graph_test.impl.cpp"
TEST_CASE("graph offset serialize file") { graph_serialization_file(); }
TEST_CASE("graph offset serialize buf") { graph_serialization_buf(); }