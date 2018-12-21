#define CISTA_PTR_TYPE raw
#include "graph_test.impl.cpp"
TEST_CASE("graph raw serialize file") { graph_serialization_file(); }
TEST_CASE("graph raw serialize buf") { graph_serialization_buf(); }