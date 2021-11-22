#pragma once

#include <cassert>
#include <cinttypes>
#include <limits>
#include <numeric>
#include <string_view>
#include <tuple>

#include "cista/bit_counting.h"
#include "cista/containers/array.h"
#include "cista/reflection/comparable.h"

namespace cista {

template <std::size_t Size>
struct bitset {
  CISTA_COMPARABLE()

  using block_t = std::uint64_t;
  static constexpr auto const bits_per_block = sizeof(block_t) * 8;
  static constexpr auto const num_blocks = Size / sizeof(block_t);

  constexpr bitset() noexcept = default;
  constexpr bitset(std::string_view s) noexcept { set(s); }

  auto cista_members() noexcept { return std::tie(blocks_); }

  constexpr void set(std::string_view s) noexcept {
    for (auto i = std::size_t{0U}; i != std::min(Size, s.size()); ++i) {
      set(Size - i - 1, s[i] != '0');
    }
  }

  constexpr void set(std::size_t const i, bool const val = true) noexcept {
    assert((i / bits_per_block) < num_blocks);
    auto& block = blocks_[i / bits_per_block];
    auto const bit = i % bits_per_block;
    if (val) {
      block |= (block_t{1U} << bit);
    } else {
      block &= (~block_t{0U} ^ (block_t{1U} << bit));
    }
  }

  void reset() noexcept { blocks_ = {}; }

  bool operator[](std::size_t i) const noexcept { return test(i); }

  std::size_t count() const noexcept {
    return std::accumulate(begin(blocks_), end(blocks_),
                           [](auto const& b) { return popcount(b); });
  }

  constexpr bool test(std::size_t const i) const noexcept {
    assert((i / bits_per_block) < num_blocks);
    auto const block = blocks_[i / bits_per_block];
    auto const bit = (i % bits_per_block);
    return (block & (block_t{1U} << bit)) != 0U;
  }

  std::size_t size() const noexcept { return Size; }

  std::string to_string() const {
    auto s = std::string{};
    s.resize(Size);
    for (auto i = 0U; i != Size; ++i) {
      s[i] = test(Size - i - 1) ? '1' : '0';
    }
    return s;
  }

  cista::array<block_t, num_blocks> blocks_{};
};

}  // namespace cista
