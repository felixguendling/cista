#define CISTA_PTR_TYPE offset
#include "./security_test.impl.cpp"

TEST_CASE("sec offset test value overflow") { test_sec_value_overflow(); }
TEST_CASE("sec offset test string overflow") { test_sec_string_overflow(); }
TEST_CASE("sec offset test vector overflow") { test_sec_vector_overflow(); }
TEST_CASE("sec offset test unique ptr overflow unset") {
  test_sec_unique_ptr_overflow_unset();
}
TEST_CASE("sec offset test unique ptr overflow set") {
  test_sec_unique_ptr_overflow_set();
}
