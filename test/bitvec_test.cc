#include "doctest.h"

#include <sstream>
#include <vector>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/bitvec.h"
#endif

void set(std::vector<bool>& v, std::string_view s) {
  v.resize(s.size());
  for (auto i = 0U; i != s.size(); ++i) {
    auto const j = s.size() - i - 1;
    v[j] = s[i] != '0';
  }
}

void set(cista::raw::bitvec& v, std::string_view s) {
  v.resize(s.size());
  for (auto i = 0U; i != s.size(); ++i) {
    auto const j = s.size() - i - 1;
    v.set(j, s[i] != '0');
  }
}

std::ostream& operator<<(std::ostream& out, std::vector<bool> const& v) {
  for (auto i = 0U; i != v.size(); ++i) {
    auto const j = v.size() - i - 1U;
    out << (v[j] ? '1' : '0');
  }
  return out;
}

std::string to_string(std::vector<bool> const& v) {
  auto ss = std::stringstream{};
  ss << v;
  return ss.str();
}

template <typename T>
bool lt(T const& x, T const& y) {
  assert(x.size() == y.size());
  for (size_t i = x.size() - 1; i != 0; --i) {
    if (x[i] ^ y[i]) return y[i];
  }
  if (x[0] ^ y[0]) return y[0];
  return false;
}

TEST_CASE("bitvec less than") {
  auto const str_1 =
      R"(000000000101001001010010010100100101001001010010010100100101001001010010010100100101001001010010111111111111111111111111111111111111111111111111)";
  auto const str_2 =
      R"(000010010000100100001001000010010000100100001001000010010101001001010010010100100101001001010010010100100101001001010010010100100101001011111111)";

  auto uut1 = cista::raw::bitvec{};
  auto uut2 = cista::raw::bitvec{};
  auto ref1 = std::vector<bool>{};
  auto ref2 = std::vector<bool>{};

  set(uut1, str_1);
  set(uut2, str_2);
  set(ref1, str_1);
  set(ref2, str_2);

  CHECK(uut1.str() == std::string_view{str_1});
  CHECK(uut2.str() == std::string_view{str_2});
  CHECK(to_string(ref1) == std::string_view{str_1});
  CHECK(to_string(ref2) == std::string_view{str_2});

  auto const ref_lt = lt(ref1, ref2);
  auto const uut_lt = lt(uut1, uut2);
  CHECK(ref_lt == uut_lt);
}
