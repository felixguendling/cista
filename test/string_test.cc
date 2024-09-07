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

TEST_CASE("self assignment") {
  // case short string
  cista::raw::string test_short{"test_short"};
  std::string output_short_pre{test_short};
  test_short = test_short;
  std::string output_short_post{test_short};
  // test_short is empty now
  CHECK(output_short_pre == output_short_post);

  // case long string
  cista::raw::string test_long{"test_long_12345678901234567890123456789012"};
  std::string output_long_pre{test_long};
  test_long = test_long;
  std::string output_long_post{test_long};
  // test_long is filled with 0x01 now
  CHECK(output_long_pre == output_long_post);
}

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

using cista::raw::u16string;
using cista::raw::u16string_view;

const auto constexpr U16STR_SHORT = u"0123";
const auto constexpr U16STR_SHORT_CORNER_CASE = u"0123456";
const auto constexpr U16STR_LONG_CORNER_CASE = u"01234567";
const auto constexpr U16STR_LONG = u"0123456789ABCDEF\xD83D\xDCBB";

TEST_CASE("u16string long short") {
  u16string s[] = {u"", U16STR_SHORT, U16STR_SHORT_CORNER_CASE,
                   U16STR_LONG_CORNER_CASE, U16STR_LONG};

  CHECK(!s[0].is_short());
  CHECK(s[0].size() == 0);
  CHECK(s[0].data() == nullptr);

  CHECK(s[1].is_short());
  CHECK(s[1].size() == 4);
  CHECK(s[1].view() == U16STR_SHORT);

  CHECK(s[2].is_short());
  CHECK(s[2].size() == 7);
  CHECK(s[2].view() == U16STR_SHORT_CORNER_CASE);

  CHECK(!s[3].is_short());
  CHECK(s[3].size() == 8);
  CHECK(s[3].view() == U16STR_LONG_CORNER_CASE);

  CHECK(!s[4].is_short());
  CHECK(s[4].size() == 18);
  CHECK(s[4].view() == U16STR_LONG);
}

TEST_CASE("u16string copy") {
  u16string s, s_s{U16STR_SHORT}, s_l{U16STR_LONG};
  u16string_view sv_s{U16STR_SHORT}, sv_l{U16STR_LONG};

  s = U16STR_SHORT;
  CHECK(s.view() == U16STR_SHORT);

  s = U16STR_LONG;
  CHECK(s.view() == U16STR_LONG);

  s = s_s;
  CHECK(s == s_s);
  CHECK(s.view() == U16STR_SHORT);

  s = s_l;
  CHECK(s == s_l);
  CHECK(s.view() == U16STR_LONG);

  s = sv_s;
  CHECK(s == sv_s);
  CHECK(s.view() == U16STR_SHORT);

  s = sv_l;
  CHECK(s == sv_l);
  CHECK(s.view() == U16STR_LONG);
}

TEST_CASE("u16string erase") {
  u16string s[] = {U16STR_SHORT, U16STR_SHORT_CORNER_CASE,
                   U16STR_LONG_CORNER_CASE, U16STR_LONG};
  std::u16string ref[] = {U16STR_SHORT, U16STR_SHORT_CORNER_CASE,
                          U16STR_LONG_CORNER_CASE, U16STR_LONG};

  for (auto i = 0; i < 4; ++i) {
    s[i].erase(1, i + 2);
    ref[i].erase(1, i + 2);
  }

  for (auto i = 0; i < 4; ++i) {
    CHECK(s[i] == ref[i]);
  }

  for (auto i = 0; i < 4; ++i) {
    s[i].erase(0, s[i].size());
    CHECK(s[i].empty());
  }
}

TEST_CASE("u16string starts_with") {
  u16string s[] = {U16STR_SHORT, U16STR_SHORT_CORNER_CASE,
                   U16STR_LONG_CORNER_CASE, U16STR_LONG};

  for (auto i = 1; i < 4; ++i) {
    CHECK(s[i].starts_with(s[i - 1]));
  }
  for (auto i = 1; i < 4; ++i) {
    CHECK(!s[i - 1].starts_with(s[i]));
  }
  for (auto i = 0; i < 4; ++i) {
    CHECK(s[i].starts_with(u'0'));
    CHECK(!s[i].starts_with(u'A'));
  }
}

TEST_CASE("u16string ends_with") {
  u16string s[] = {U16STR_SHORT, U16STR_SHORT_CORNER_CASE,
                   U16STR_LONG_CORNER_CASE, U16STR_LONG};

  CHECK(s[0].ends_with(U16STR_SHORT));
  CHECK(s[0].ends_with(u"123"));
  CHECK(s[0].ends_with(u'3'));
  CHECK(!s[0].ends_with(u"01234"));
  CHECK(!s[0].ends_with(u'A'));

  CHECK(s[1].ends_with(U16STR_SHORT_CORNER_CASE));
  CHECK(s[1].ends_with(u"456"));
  CHECK(s[1].ends_with(u'6'));
  CHECK(!s[1].ends_with(u"01234567"));
  CHECK(!s[1].ends_with(u'A'));

  CHECK(s[2].ends_with(U16STR_LONG_CORNER_CASE));
  CHECK(s[2].ends_with(u"567"));
  CHECK(s[2].ends_with(u'7'));
  CHECK(!s[2].ends_with(u"012345677"));
  CHECK(!s[2].ends_with(u'A'));

  CHECK(s[3].ends_with(U16STR_LONG));
  CHECK(s[3].ends_with(u"F\xD83D\xDCBB"));
  CHECK(s[3].ends_with(u'\xDCBB'));
  CHECK(!s[3].ends_with(u'A'));
}

using cista::raw::u32string;
using cista::raw::u32string_view;

const auto constexpr U32STR_SHORT = U"01";
const auto constexpr U32STR_SHORT_CORNER_CASE = U"012";
const auto constexpr U32STR_LONG_CORNER_CASE = U"0123";
const auto constexpr U32STR_LONG = U"0123456789ABCDEF\U0001F4BB";

TEST_CASE("u32string long short") {
  u32string s[] = {U"", U32STR_SHORT, U32STR_SHORT_CORNER_CASE,
                   U32STR_LONG_CORNER_CASE, U32STR_LONG};

  CHECK(!s[0].is_short());
  CHECK(s[0].size() == 0);
  CHECK(s[0].data() == nullptr);

  CHECK(s[1].is_short());
  CHECK(s[1].size() == 2);
  CHECK(s[1].view() == U32STR_SHORT);

  CHECK(s[2].is_short());
  CHECK(s[2].size() == 3);
  CHECK(s[2].view() == U32STR_SHORT_CORNER_CASE);

  CHECK(!s[3].is_short());
  CHECK(s[3].size() == 4);
  CHECK(s[3].view() == U32STR_LONG_CORNER_CASE);

  CHECK(!s[4].is_short());
  CHECK(s[4].size() == 17);
  CHECK(s[4].view() == U32STR_LONG);
}

TEST_CASE("u32string copy") {
  u32string s, s_s{U32STR_SHORT}, s_l{U32STR_LONG};
  u32string_view sv_s{U32STR_SHORT}, sv_l{U32STR_LONG};

  s = U32STR_SHORT;
  CHECK(s.view() == U32STR_SHORT);

  s = U32STR_LONG;
  CHECK(s.view() == U32STR_LONG);

  s = s_s;
  CHECK(s == s_s);
  CHECK(s.view() == U32STR_SHORT);

  s = s_l;
  CHECK(s == s_l);
  CHECK(s.view() == U32STR_LONG);

  s = sv_s;
  CHECK(s == sv_s);
  CHECK(s.view() == U32STR_SHORT);

  s = sv_l;
  CHECK(s == sv_l);
  CHECK(s.view() == U32STR_LONG);
}

TEST_CASE("u32string erase") {
  u32string s[] = {U32STR_SHORT, U32STR_SHORT_CORNER_CASE,
                   U32STR_LONG_CORNER_CASE, U32STR_LONG};
  std::u32string ref[] = {U32STR_SHORT, U32STR_SHORT_CORNER_CASE,
                          U32STR_LONG_CORNER_CASE, U32STR_LONG};

  for (auto i = 0; i < 4; ++i) {
    s[i].erase(1, i + 1);
    ref[i].erase(1, i + 1);
  }

  for (auto i = 0; i < 4; ++i) {
    CHECK(s[i] == ref[i]);
  }

  for (auto i = 0; i < 4; ++i) {
    s[i].erase(0, s[i].size());
    CHECK(s[i].empty());
  }
}

TEST_CASE("u32string starts_with") {
  u32string s[] = {U32STR_SHORT, U32STR_SHORT_CORNER_CASE,
                   U32STR_LONG_CORNER_CASE, U32STR_LONG};

  for (auto i = 1; i < 4; ++i) {
    CHECK(s[i].starts_with(s[i - 1]));
  }
  for (auto i = 1; i < 4; ++i) {
    CHECK(!s[i - 1].starts_with(s[i]));
  }
  for (auto i = 0; i < 4; ++i) {
    CHECK(s[i].starts_with(U'0'));
    CHECK(!s[i].starts_with(U'A'));
  }
}

TEST_CASE("u32string ends_with") {
  u32string s[] = {U32STR_SHORT, U32STR_SHORT_CORNER_CASE,
                   U32STR_LONG_CORNER_CASE, U32STR_LONG};

  CHECK(s[0].ends_with(U32STR_SHORT));
  CHECK(s[0].ends_with(U'1'));
  CHECK(!s[0].ends_with(U"012"));
  CHECK(!s[0].ends_with(U'A'));

  CHECK(s[1].ends_with(U32STR_SHORT_CORNER_CASE));
  CHECK(s[1].ends_with(U"12"));
  CHECK(s[1].ends_with(U'2'));
  CHECK(!s[1].ends_with(U"0123"));
  CHECK(!s[1].ends_with(U'A'));

  CHECK(s[2].ends_with(U32STR_LONG_CORNER_CASE));
  CHECK(s[2].ends_with(U"123"));
  CHECK(s[2].ends_with(U'3'));
  CHECK(!s[2].ends_with(U"01234"));
  CHECK(!s[2].ends_with(U'A'));

  CHECK(s[3].ends_with(U32STR_LONG));
  CHECK(s[3].ends_with(U"EF\U0001F4BB"));
  CHECK(s[3].ends_with(U'\U0001F4BB'));
  CHECK(!s[3].ends_with(U'A'));
}
