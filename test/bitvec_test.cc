#include "doctest.h"

#include <sstream>
#include <thread>
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
  v.resize(static_cast<unsigned>(s.size()));
  for (auto i = 0U; i != s.size(); ++i) {
    auto const j = static_cast<unsigned>(s.size() - i - 1);
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
bool bitvec_lt(T const& x, T const& y) {
  assert(x.size() == y.size());
  for (auto i = x.size() - 1; i != 0; --i) {
    if (x[i] ^ y[i]) return y[i];
  }
  if (!x.empty() && x[0] ^ y[0]) return y[0];
  return false;
}

TEST_CASE("bitvec bignum") {
  auto uut =
      cista::basic_bitvec<cista::basic_vector<std::uint64_t, cista::raw::ptr,
                                              false, std::uint64_t>>{};
  uut.resize(8355212022ULL);
  uut.set(8355212021ULL, true);
  uut.for_each_set_bit([](auto const i) { CHECK_EQ(i, 8355212021ULL); });
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

  auto count = 0U;
  uut2.for_each_set_bit([&](auto const /* */) { ++count; });
  CHECK_EQ(count, uut2.count());

  CHECK(uut1.str() == std::string_view{str_1});
  CHECK(uut2.str() == std::string_view{str_2});
  CHECK(to_string(ref1) == std::string_view{str_1});
  CHECK(to_string(ref2) == std::string_view{str_2});

  auto const ref_lt = bitvec_lt(ref1, ref2);
  auto const uut_lt = bitvec_lt(uut1, uut2);
  CHECK(ref_lt == uut_lt);
}

unsigned long get_random_number() {  // period 2^96-1
  static std::uint64_t x = 123456789, y = 362436069, z = 521288629;

  unsigned long t;
  x ^= x << 16;
  x ^= x >> 5;
  x ^= x << 1;

  t = x;
  x = y;
  y = z;
  z = t ^ x ^ y;

  return z;
}

TEST_CASE("bitvec atomic set") {
  constexpr auto const kBits = 100'000U;
  constexpr auto const kWorkers = 100U;

  auto start = std::atomic_bool{};
  auto b = cista::raw::bitvec{};
  b.resize(kBits);
  auto workers = std::vector<std::thread>(kWorkers);
  for (auto i = 0U; i != kWorkers; ++i) {
    workers[i] = std::thread{[&, i]() {
      while (!start) {
        // wait for synchronized start
      }

      for (auto j = i; j < kBits; j += kBits / kWorkers) {
        b.atomic_set(j);
      }
    }};
  }

  start.store(true);

  for (auto& w : workers) {
    w.join();
  }

  CHECK(b.count() == kBits);
}

TEST_CASE("bitvec parallel") {
  constexpr auto const kBits = 1'000'000U;
  constexpr auto const kWorkers = 100U;

  auto b = cista::raw::bitvec{};
  b.resize(kBits);

  auto bits = std::vector<std::size_t>{};
  bits.resize(b.size() * 0.2);
  std::generate(begin(bits), end(bits), [&]() {
    auto x = static_cast<std::uint32_t>(get_random_number() % b.size());
    b.set(x, true);
    return x;
  });
  std::sort(begin(bits), end(bits));
  bits.erase(std::unique(begin(bits), end(bits)), end(bits));

  auto next = std::atomic_size_t{0U};
  auto workers = std::vector<std::thread>(kWorkers);
  auto collected_bits = std::vector<std::vector<std::size_t>>(kWorkers);
  for (auto i = 0U; i != kWorkers; ++i) {
    workers[i] = std::thread{[&, i]() {
      auto next_bit = std::optional<std::size_t>{};
      do {
        next_bit = b.get_next(next);
        if (next_bit.has_value()) {
          collected_bits[i].push_back(*next_bit);
        }
      } while (next_bit.has_value());
    }};
  }

  for (auto& w : workers) {
    w.join();
  }

  auto check = std::vector<std::size_t>{};
  for (auto& x : collected_bits) {
    check.insert(end(check), begin(x), end(x));
  }
  std::sort(begin(check), end(check));
  check.erase(std::unique(begin(check), end(check)), end(check));

  CHECK_EQ(bits, check);
}