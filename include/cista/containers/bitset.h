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
#include "cista/reflection/comparable.h"

namespace cista {

template <std::size_t Size>
struct bitset {
  using block_t = std::uint64_t;
  static constexpr auto const bits_per_block = sizeof(block_t) * 8;
  static constexpr auto const num_blocks =
      Size / bits_per_block + (Size % bits_per_block == 0 ? 0 : 1);

  constexpr bitset() noexcept = default;
  constexpr bitset(std::string_view s) noexcept { set(s); }

  auto cista_members() noexcept { return std::tie(blocks_); }

  constexpr void set(std::string_view s) noexcept {
    for (auto i = std::size_t{0U}; i != std::min(Size, s.size()); ++i) {
      set(i, s[s.size() - i - 1] != '0');
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
    auto sum = std::size_t{0U};
    for (auto i = std::size_t{0U}; i != num_blocks - 1; ++i) {
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
    for (auto i = std::size_t{0U}; i != num_blocks - 1; ++i) {
      if (blocks_[i] != 0U) {
        return true;
      }
    }
    return sanitized_last_block() != 0U;
  }

  bool none() const noexcept { return !any(); }

  block_t sanitized_last_block() const noexcept {
    if constexpr ((Size % bits_per_block) != 0) {
      return blocks_[num_blocks - 1] &
             ~((~block_t{0}) << (Size % bits_per_block));
    } else {
      return blocks_[num_blocks - 1];
    }
  }

  std::string to_string() const {
    auto s = std::string{};
    s.resize(Size);
    for (auto i = 0U; i != Size; ++i) {
      s[i] = test(Size - i - 1) ? '1' : '0';
    }
    return s;
  }

  friend bool operator==(bitset const& a, bitset const& b) noexcept {
    for (auto i = std::size_t{0U}; i != num_blocks - 1; ++i) {
      if (a.blocks_[i] != b.blocks_[i]) {
        return false;
      }
    }
    return a.sanitized_last_block() == b.sanitized_last_block();
  }

  friend bool operator<(bitset const& a, bitset const& b) {
    auto const a_last = a.sanitized_last_block();
    auto const b_last = b.sanitized_last_block();
    if (a_last < b_last) {
      return true;
    } else if (b_last < a_last) {
      return false;
    }

    for (int i = num_blocks - 2; i != -1; --i) {
      if (a.blocks_[i] < b.blocks_[i]) {
        return true;
      } else if (b.blocks_[i] < a.blocks_[i]) {
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

    if constexpr ((Size % bits_per_block) != 0) {
      blocks_[num_blocks - 1] = sanitized_last_block();
    }

    if constexpr (num_blocks == 1U) {
      blocks_[0] >>= shift;
      return *this;
    } else {
      if (shift == 0U) {
        return *this;
      }

      auto const shift_blocks = shift / bits_per_block;
      auto const shift_bits = shift % bits_per_block;
      auto const border = num_blocks - shift_blocks - 1U;

      if (shift_bits == 0U) {
        for (auto i = std::size_t{0U}; i <= border; ++i) {
          blocks_[i] = blocks_[i + shift_blocks];
        }
      } else {
        for (auto i = std::size_t{0U}; i < border; ++i) {
          blocks_[i] =
              (blocks_[i + shift_blocks] >> shift_bits) |
              (blocks_[i + shift_blocks + 1] << (bits_per_block - shift_bits));
        }
        blocks_[border] = (blocks_[num_blocks - 1] >> shift_bits);
      }

      for (auto i = border + 1; i != num_blocks; ++i) {
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
      blocks_[0] <<= shift;
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
              (blocks_[i - shift_blocks - 1] >> (bits_per_block - shift_bits));
        }
        blocks_[shift_blocks] = blocks_[0] << shift_bits;
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
