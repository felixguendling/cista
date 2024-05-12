#include <iostream>
#include <vector>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/string.h"
#include "cista/hash.h"
#endif

using cista::raw::string;

constexpr auto const CORNER_CASE_SHORT_14 = "01234567891234";
constexpr auto const CORNER_CASE_SHORT_15 = "012345678912345";
constexpr auto const CORNER_CASE_LONG_16 = "0123456789123456";
constexpr auto const LONG_STR = "hello world hello world";
constexpr auto const SHORT_STR = "hello world";

TEST_CASE("string init") {
  auto s = string{};
  CHECK(!s.is_short());
  CHECK(s.size() == 0);
  CHECK(s.data() == nullptr);
}

TEST_CASE("string long short corner 14") {
  auto s = string{CORNER_CASE_SHORT_14, string::owning};
  CHECK(s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_SHORT_14));
  CHECK(s.view() == CORNER_CASE_SHORT_14);
}

TEST_CASE("string erase") {
  auto uut = std::vector<cista::generic_string<char const*>>{};
  auto ref = std::vector<std::string>{};
  for (auto const s :
       {CORNER_CASE_SHORT_14, CORNER_CASE_SHORT_15, CORNER_CASE_LONG_16}) {
    auto x = cista::generic_string{
        s, cista::generic_string<char const*>::non_owning};
    x.erase(3, 7);
    uut.emplace_back(std::move(x));

    auto y = std::string{s};
    y.erase(3, 7);
    ref.emplace_back(std::move(y));
  }

  for (auto i = 0U; i != ref.size(); ++i) {
    CHECK(ref[i] == uut[i].view());
  }
}

TEST_CASE("string long short corner 15") {
  auto s = string{CORNER_CASE_SHORT_15, string::owning};
  CHECK(s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_SHORT_15));
  CHECK(s.view() == CORNER_CASE_SHORT_15);
}

TEST_CASE("string long short corner 16") {
  auto s = string{CORNER_CASE_LONG_16, string::owning};
  CHECK(!s.is_short());
  CHECK(s.size() == std::strlen(CORNER_CASE_LONG_16));
  CHECK(s.view() == CORNER_CASE_LONG_16);
}

TEST_CASE("string long short") {
  auto s = string{SHORT_STR, string::owning};
  CHECK(s.view() == SHORT_STR);
  CHECK(s.is_short());

  s.set_owning(CORNER_CASE_LONG_16);
  CHECK(!s.is_short());
  CHECK(s.view() == CORNER_CASE_LONG_16);

  s.set_owning(LONG_STR);
  CHECK(!s.is_short());
  CHECK(s.view() == LONG_STR);
}

TEST_CASE("string dealloc long to short") {
  string s = "one two";
  CHECK(s.size() == std::strlen("one two"));
  CHECK(s.is_short());
  s.set_non_owning("");
}

TEST_CASE("string copy assign and copy construct") {
  auto s0 = string{LONG_STR, string::owning};
  auto s1 = string{s0};
  CHECK(s0 == s1);
  CHECK(s1.view() == LONG_STR);

  string s2;
  s2 = s0;
  CHECK(s0 == s2);
  CHECK(s2.view() == LONG_STR);
}

TEST_CASE("string hash") {
  auto str = string{""};
  auto h = cista::hash(str, cista::BASE_HASH);
  CHECK(cista::BASE_HASH == h);
}

TEST_CASE("string starts_with") {
  string s = "abacaba";

  CHECK(s.starts_with("abac") == true);
  CHECK(s.starts_with("abacaba") == true);
  CHECK(s.starts_with("abacaba_") == false);
  CHECK(s.starts_with("a") == true);
  CHECK(s.starts_with("") == true);
  CHECK(s.starts_with("abad") == false);

  CHECK(s.starts_with(string{"abac"}) == true);
  CHECK(s.starts_with(string{"abacaba"}) == true);
  CHECK(s.starts_with(string{"abacaba_"}) == false);
  CHECK(s.starts_with(string{"a"}) == true);
  CHECK(s.starts_with(string{""}) == true);
  CHECK(s.starts_with(string{"abad"}) == false);

  CHECK(s.starts_with(std::string{"abac"}) == true);
  CHECK(s.starts_with(std::string{"abacaba"}) == true);
  CHECK(s.starts_with(std::string{"abacaba_"}) == false);
  CHECK(s.starts_with(std::string{"a"}) == true);
  CHECK(s.starts_with(std::string{""}) == true);
  CHECK(s.starts_with(std::string{"\0", 1}) == false);
  CHECK(s.starts_with(std::string{"abad"}) == false);

  CHECK(s.starts_with(std::string_view{"abac"}) == true);
  CHECK(s.starts_with(std::string_view{"abacaba"}) == true);
  CHECK(s.starts_with(std::string_view{"abacaba_"}) == false);
  CHECK(s.starts_with(std::string_view{"a"}) == true);
  CHECK(s.starts_with(std::string_view{""}) == true);
  CHECK(s.starts_with(std::string_view{"\0", 1}) == false);
  CHECK(s.starts_with(std::string_view{"abad"}) == false);

  CHECK(s.starts_with('a') == true);
  CHECK(s.starts_with('b') == false);
  CHECK(s.starts_with('\0') == false);
}

TEST_CASE("string ends_with") {
  string s = "abacaba";

  CHECK(s.ends_with("caba") == true);
  CHECK(s.ends_with("abacaba") == true);
  CHECK(s.ends_with("abacaba_") == false);
  CHECK(s.ends_with("a") == true);
  CHECK(s.ends_with("") == true);
  CHECK(s.ends_with("daba") == false);

  CHECK(s.ends_with(string{"caba"}) == true);
  CHECK(s.ends_with(string{"abacaba"}) == true);
  CHECK(s.ends_with(string{"abacaba_"}) == false);
  CHECK(s.ends_with(string{"a"}) == true);
  CHECK(s.ends_with(string{""}) == true);
  CHECK(s.ends_with(string{"daba"}) == false);

  CHECK(s.ends_with(std::string{"caba"}) == true);
  CHECK(s.ends_with(std::string{"abacaba"}) == true);
  CHECK(s.ends_with(std::string{"abacaba_"}) == false);
  CHECK(s.ends_with(std::string{"a"}) == true);
  CHECK(s.ends_with(std::string{""}) == true);
  CHECK(s.ends_with(std::string{"\0", 1}) == false);
  CHECK(s.ends_with(std::string{"daba"}) == false);

  CHECK(s.ends_with(std::string_view{"caba"}) == true);
  CHECK(s.ends_with(std::string_view{"abacaba"}) == true);
  CHECK(s.ends_with(std::string_view{"abacaba_"}) == false);
  CHECK(s.ends_with(std::string_view{"a"}) == true);
  CHECK(s.ends_with(std::string_view{""}) == true);
  CHECK(s.ends_with(std::string_view{"\0", 1}) == false);
  CHECK(s.ends_with(std::string_view{"daba"}) == false);

  CHECK(s.ends_with('a') == true);
  CHECK(s.ends_with('b') == false);
  CHECK(s.ends_with('\0') == false);
}
