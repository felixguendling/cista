#define CISTA_PTR_TYPE raw
#include "./security_test.impl.cpp"

TEST_CASE("sec raw test value overflow") { test_sec_value_overflow(); }
TEST_CASE("sec raw test string overflow") { test_sec_string_overflow(); }
TEST_CASE("sec raw test vector overflow") { test_sec_vector_overflow(); }
TEST_CASE("sec raw test unique ptr overflow unset") {
  test_sec_unique_ptr_overflow_unset();
}
TEST_CASE("sec raw test unique ptr overflow set") {
  test_sec_unique_ptr_overflow_set();
}
