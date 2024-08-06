#pragma once

#include <cassert>
#include <cinttypes>
#include <iosfwd>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "cista/bit_counting.h"
#include "cista/containers/array.h"

namespace cista {

template <std::size_t Size>
struct bitset {
  using block_t = std::uint64_t;
  static constexpr auto const bits_per_block = sizeof(block_t) * 8U;
  static constexpr auto const num_blocks =
      Size / bits_per_block + (Size % bits_per_block == 0U ? 0U : 1U);

  constexpr bitset() noexcept = default;
  constexpr bitset(std::string_view s) noexcept { set(s); }
  static constexpr bitset max() {
    bitset ret;
    for (auto& b : ret.blocks_) {
      b = std::numeric_limits<block_t>::max();
    }
    return ret;
  }

  auto cista_members() noexcept { return std::tie(blocks_); }

  constexpr void set(std::string_view s) noexcept {
    for (std::size_t i = 0U; i != std::min(Size, s.size()); ++i) {
      set(i, s[s.size() - i - 1U] != '0');
    }
  }

  constexpr void set(std::size_t const i, bool const val = true) noexcept {
    assert((i / bits_per_block) < num_blocks);
    auto& block = blocks_[i / bits_per_block];
    auto const bit = i % bits_per_block;
    auto const mask = block_t{1U} << bit;
    if (val) {
      block |= mask;
    } else {
      block &= (~block_t{0U} ^ mask);
    }
  }

  void reset() noexcept { blocks_ = {}; }

  bool operator[](std::size_t const i) const noexcept { return test(i); }

  std::size_t count() const noexcept {
    std::size_t sum = 0U;
    for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
      sum += popcount(blocks_[i]);
    }
    return sum + popcount(sanitized_last_block());
  }

  constexpr bool test(std::size_t const i) const noexcept {
    if (i >= Size) {
      return false;
    }
    auto const block = blocks_[i / bits_per_block];
    auto const bit = (i % bits_per_block);
    return (block & (block_t{1U} << bit)) != 0U;
  }

  std::size_t size() const noexcept { return Size; }

  bool any() const noexcept {
    for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
      if (blocks_[i] != 0U) {
        return true;
      }
    }
    return sanitized_last_block() != 0U;
  }

  bool none() const noexcept { return !any(); }

  block_t sanitized_last_block() const noexcept {
    if constexpr ((Size % bits_per_block) != 0U) {
      return blocks_[num_blocks - 1U] &
             ~((~block_t{0U}) << (Size % bits_per_block));
    } else {
      return blocks_[num_blocks - 1U];
    }
  }

  template <typename Fn>
  void for_each_set_bit(Fn&& f) const {
    auto const check_block = [&](std::size_t const i, block_t const block) {
      if (block != 0U) {
        for (auto bit = std::size_t{0U}; bit != bits_per_block; ++bit) {
          if ((block & (block_t{1U} << bit)) != 0U) {
            f(std::size_t{i * bits_per_block + bit});
          }
        }
      }
    };
    for (auto i = std::size_t{0U}; i != blocks_.size() - 1; ++i) {
      check_block(i, blocks_[i]);
    }
    check_block(blocks_.size() - 1, sanitized_last_block());
  }

  std::string to_string() const {
    std::string s{};
    s.resize(Size);
    for (std::size_t i = 0U; i != Size; ++i) {
      s[i] = test(Size - i - 1U) ? '1' : '0';
    }
    return s;
  }

  friend bool operator==(bitset const& a, bitset const& b) noexcept {
    for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
      if (a.blocks_[i] != b.blocks_[i]) {
        return false;
      }
    }
    return a.sanitized_last_block() == b.sanitized_last_block();
  }

  friend bool operator<(bitset const& a, bitset const& b) noexcept {
    auto const a_last = a.sanitized_last_block();
    auto const b_last = b.sanitized_last_block();
    if (a_last < b_last) {
      return true;
    }
    if (b_last < a_last) {
      return false;
    }

    for (int i = num_blocks - 2; i != -1; --i) {
      auto const x = a.blocks_[i];
      auto const y = b.blocks_[i];
      if (x < y) {
        return true;
      }
      if (y < x) {
        return false;
      }
    }

    return false;
  }
  friend bool operator!=(bitset const& a, bitset const& b) noexcept {
    return !(a == b);
  }

  friend bool operator>(bitset const& a, bitset const& b) noexcept {
    return b < a;
  }

  friend bool operator<=(bitset const& a, bitset const& b) noexcept {
    return !(a > b);
  }

  friend bool operator>=(bitset const& a, bitset const& b) noexcept {
    return !(a < b);
  }

  bitset& operator&=(bitset const& o) noexcept {
    for (auto i = 0U; i < num_blocks; ++i) {
      blocks_[i] &= o.blocks_[i];
    }
    return *this;
  }

  bitset& operator|=(bitset const& o) noexcept {
    for (auto i = 0U; i < num_blocks; ++i) {
      blocks_[i] |= o.blocks_[i];
    }
    return *this;
  }

  bitset& operator^=(bitset const& o) noexcept {
    for (auto i = 0U; i < num_blocks; ++i) {
      blocks_[i] ^= o.blocks_[i];
    }
    return *this;
  }

  bitset operator~() const noexcept {
    auto copy = *this;
    for (auto& b : copy.blocks_) {
      b = ~b;
    }
    return copy;
  }

  friend bitset operator&(bitset const& lhs, bitset const& rhs) noexcept {
    auto copy = lhs;
    copy &= rhs;
    return copy;
  }

  friend bitset operator|(bitset const& lhs, bitset const& rhs) noexcept {
    auto copy = lhs;
    copy |= rhs;
    return copy;
  }

  friend bitset operator^(bitset const& lhs, bitset const& rhs) noexcept {
    auto copy = lhs;
    copy ^= rhs;
    return copy;
  }

  bitset& operator>>=(std::size_t const shift) noexcept {
    if (shift >= Size) {
      reset();
      return *this;
    }

    if constexpr ((Size % bits_per_block) != 0U) {
      blocks_[num_blocks - 1U] = sanitized_last_block();
    }

    if constexpr (num_blocks == 1U) {
      blocks_[0U] >>= shift;
      return *this;
    } else {
      if (shift == 0U) {
        return *this;
      }

      auto const shift_blocks = shift / bits_per_block;
      auto const shift_bits = shift % bits_per_block;
      auto const border = num_blocks - shift_blocks - 1U;

      if (shift_bits == 0U) {
        for (std::size_t i = 0U; i <= border; ++i) {
          blocks_[i] = blocks_[i + shift_blocks];
        }
      } else {
        for (std::size_t i = 0U; i < border; ++i) {
          blocks_[i] =
              (blocks_[i + shift_blocks] >> shift_bits) |
              (blocks_[i + shift_blocks + 1] << (bits_per_block - shift_bits));
        }
        blocks_[border] = (blocks_[num_blocks - 1] >> shift_bits);
      }

      for (auto i = border + 1U; i != num_blocks; ++i) {
        blocks_[i] = 0U;
      }

      return *this;
    }
  }

  bitset& operator<<=(std::size_t const shift) noexcept {
    if (shift >= Size) {
      reset();
      return *this;
    }

    if constexpr (num_blocks == 1U) {
      blocks_[0U] <<= shift;
      return *this;
    } else {
      if (shift == 0U) {
        return *this;
      }

      auto const shift_blocks = shift / bits_per_block;
      auto const shift_bits = shift % bits_per_block;

      if (shift_bits == 0U) {
        for (auto i = std::size_t{num_blocks - 1}; i >= shift_blocks; --i) {
          blocks_[i] = blocks_[i - shift_blocks];
        }
      } else {
        for (auto i = std::size_t{num_blocks - 1}; i != shift_blocks; --i) {
          blocks_[i] =
              (blocks_[i - shift_blocks] << shift_bits) |
              (blocks_[i - shift_blocks - 1U] >> (bits_per_block - shift_bits));
        }
        blocks_[shift_blocks] = blocks_[0U] << shift_bits;
      }

      for (auto i = 0U; i != shift_blocks; ++i) {
        blocks_[i] = 0U;
      }

      return *this;
    }
  }

  bitset operator>>(std::size_t const i) const noexcept {
    auto copy = *this;
    copy >>= i;
    return copy;
  }

  bitset operator<<(std::size_t const i) const noexcept {
    auto copy = *this;
    copy <<= i;
    return copy;
  }

  friend std::ostream& operator<<(std::ostream& out, bitset const& b) {
    return out << b.to_string();
  }

  cista::array<block_t, num_blocks> blocks_{};
};

}  // namespace cista

#if __has_include("fmt/ostream.h")

#include "fmt/ostream.h"

template <std::size_t Size>
struct fmt::formatter<cista::bitset<Size>> : ostream_formatter {};

#endif
