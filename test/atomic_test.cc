#include "doctest.h"

#include <thread>
#include <vector>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/atomic.h"
#endif

unsigned long get_random_number();

TEST_CASE("atomic fetch_or / fetch_and return the old value") {
  auto block = std::uint64_t{0b0101U};

  CHECK(cista::fetch_or(block, 0b0010U) == 0b0101U);
  CHECK(block == 0b0111U);

  CHECK(cista::fetch_and(block, 0b1100U) == 0b0111U);
  CHECK(block == 0b0100U);
}

TEST_CASE("atomic min test") {
  constexpr auto const kNValues = 1'000'000;
  constexpr auto const kWorkers = 100U;

  auto values = std::vector<std::int16_t>{};
  values.resize(kNValues);
  std::generate(begin(values), end(values), [&]() {
    return static_cast<std::int16_t>(get_random_number());
  });

  auto min = std::vector<std::int16_t>{};
  min.resize(1000, std::numeric_limits<std::int16_t>::max());

  auto max = std::vector<std::int16_t>{};
  max.resize(1000, std::numeric_limits<std::int16_t>::min());

  auto workers = std::vector<std::thread>(kWorkers);
  auto next = std::atomic_size_t{0U};
  auto start = std::atomic_bool{false};
  for (auto i = 0U; i != kWorkers; ++i) {
    workers[i] = std::thread{[&]() {
      while (!start) {
        // wait for synchronized start
      }

      while (true) {
        auto idx = next++;
        if (idx >= values.size()) {
          return;
        }
        for (auto j = 0U; j != min.size(); ++j) {
          cista::fetch_min(min[j], values[idx]);
          cista::fetch_max(max[j], values[idx]);
        }
      }
    }};
  }

  start.store(true);

  for (auto& w : workers) {
    w.join();
  }

  auto const smallest = *std::min_element(begin(values), end(values));
  for (auto const& x : min) {
    CHECK(x == smallest);
  }

  auto const largest = *std::max_element(begin(values), end(values));
  for (auto const& x : max) {
    CHECK(x == largest);
  }
}