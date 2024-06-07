#pragma once

#include <cassert>
#include <cinttypes>
#include <atomic>
#include <iosfwd>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "cista/bit_counting.h"
#include "cista/containers/vector.h"
#include "cista/strong.h"

namespace cista {

template <typename Vec, typename Key = typename Vec::size_type>
struct basic_bitvec {
  using block_t = typename Vec::value_type;
  using size_type = typename Vec::size_type;
  static constexpr auto const bits_per_block =
      static_cast<size_type>(sizeof(block_t) * 8);

  constexpr basic_bitvec() noexcept {}
  constexpr basic_bitvec(std::string_view s) noexcept { set(s); }
  constexpr basic_bitvec(Vec&& v) noexcept
      : size_{v.size() * bits_per_block},  // inaccurate for loading mmap vector
        blocks_{std::move(v)} {}
  static constexpr basic_bitvec max(std::size_t const size) {
    basic_bitvec ret;
    ret.resize(size);
    for (auto& b : ret.blocks_) {
      b = std::numeric_limits<block_t>::max();
    }
    return ret;
  }

  auto cista_members() noexcept { return std::tie(blocks_); }

  static constexpr size_type num_blocks(size_type num_bits) {
    return static_cast<size_type>(num_bits / bits_per_block +
                                  (num_bits % bits_per_block == 0 ? 0 : 1));
  }

  void resize(size_type const new_size) {
    if (new_size == size_) {
      return;
    }

    if (!empty() && (size_ % bits_per_block) != 0U) {
      blocks_[blocks_.size() - 1] &=
          ~((~block_t{0}) << (size_ % bits_per_block));
    }
    blocks_.resize(num_blocks(new_size));
    size_ = new_size;
  }

  constexpr void set(std::string_view s) noexcept {
    assert(std::all_of(begin(s), end(s),
                       [](char const c) { return c == '0' || c == '1'; }));
    resize(s.size());
    for (auto i = std::size_t{0U}; i != std::min(size_, s.size()); ++i) {
      set(i, s[s.size() - i - 1] != '0');
    }
  }

  constexpr void set(Key const i, bool const val = true) noexcept {
    assert(i < size_);
    assert((to_idx(i) / bits_per_block) < blocks_.size());
    auto& block = blocks_[static_cast<size_type>(to_idx(i)) / bits_per_block];
    auto const bit = to_idx(i) % bits_per_block;
    if (val) {
      block |= (block_t{1U} << bit);
    } else {
      block &= (~block_t{0U} ^ (block_t{1U} << bit));
    }
  }

  void reset() noexcept { blocks_ = {}; }

  bool operator[](Key const i) const noexcept { return test(i); }

  std::size_t count() const noexcept {
    if (empty()) {
      return 0;
    }
    auto sum = std::size_t{0U};
    for (auto i = size_type{0U}; i != blocks_.size() - 1; ++i) {
      sum += popcount(blocks_[i]);
    }
    return sum + popcount(sanitized_last_block());
  }

  constexpr bool test(Key const i) const noexcept {
    if (i >= size_) {
      return false;
    }
    assert((i / bits_per_block) < blocks_.size());
    auto const block =
        blocks_[static_cast<size_type>(to_idx(i)) / bits_per_block];
    auto const bit = (to_idx(i) % bits_per_block);
    return (block & (block_t{1U} << bit)) != 0U;
  }

  template <typename Fn>
  void for_each_set_bit(Fn&& f) const {
    if (empty()) {
      return;
    }
    auto const check_block = [&](size_type const i, block_t const block) {
      if (block != 0U) {
        for (auto bit = size_type{0U}; bit != bits_per_block; ++bit) {
          if ((block & (block_t{1U} << bit)) != 0U) {
            f(Key{i * bits_per_block + bit});
          }
        }
      }
    };
    for (auto i = size_type{0U}; i != blocks_.size() - 1; ++i) {
      check_block(i, blocks_[i]);
    }
    check_block(blocks_.size() - 1, sanitized_last_block());
  }

  std::optional<Key> next_set_bit(size_type const i) const {
    if (i >= size()) {
      return std::nullopt;
    }

    auto const first_block_idx = i / bits_per_block;
    auto const first_block = blocks_[first_block_idx];
    if (first_block != 0U) {
      auto const first_bit = i % bits_per_block;
      auto const n = std::min(size(), bits_per_block);
      for (auto bit = first_bit; bit != n; ++bit) {
        if ((first_block & (block_t{1U} << bit)) != 0U) {
          return Key{first_block_idx * bits_per_block + bit};
        }
      }
    }

    if (first_block_idx + 1U == blocks_.size()) {
      return std::nullopt;
    }

    auto const check_block = [&](size_type const block_idx,
                                 block_t const block) -> std::optional<Key> {
      if (block != 0U) {
        for (auto bit = size_type{0U}; bit != bits_per_block; ++bit) {
          if ((block & (block_t{1U} << bit)) != 0U) {
            return Key{block_idx * bits_per_block + bit};
          }
        }
      }
      return std::nullopt;
    };

    for (auto block_idx = first_block_idx + 1U; block_idx != blocks_.size() - 1;
         ++block_idx) {
      if (auto const set_bit_idx = check_block(block_idx, blocks_[block_idx]);
          set_bit_idx.has_value()) {
        return set_bit_idx;
      }
    }

    if (auto const set_bit_idx =
            check_block(blocks_.size() - 1, sanitized_last_block());
        set_bit_idx.has_value()) {
      return set_bit_idx;
    }

    return std::nullopt;
  }

  std::optional<Key> get_next(std::atomic_size_t& next) const {
    while (true) {
      auto expected = next.load();
      auto idx = next_set_bit(Key{static_cast<base_t<Key>>(expected)});
      if (!idx.has_value()) {
        return std::nullopt;
      }
      if (next.compare_exchange_weak(expected, *idx + 1U)) {
        return idx;
      }
    }
  }

  size_type size() const noexcept { return size_; }
  bool empty() const noexcept { return size() == 0U; }

  bool any() const noexcept {
    if (empty()) {
      return false;
    }
    for (auto i = size_type{0U}; i != blocks_.size() - 1; ++i) {
      if (blocks_[i] != 0U) {
        return true;
      }
    }
    return sanitized_last_block() != 0U;
  }

  bool none() const noexcept { return !any(); }

  block_t sanitized_last_block() const noexcept {
    if ((size_ % bits_per_block) != 0) {
      return blocks_[blocks_.size() - 1] &
             ~((~block_t{0}) << (size_ % bits_per_block));
    } else {
      return blocks_[blocks_.size() - 1];
    }
  }

  std::string str() const {
    auto s = std::string{};
    s.resize(size_);
    for (auto i = 0U; i != size_; ++i) {
      s[i] = test(size_ - i - 1) ? '1' : '0';
    }
    return s;
  }

  friend bool operator==(basic_bitvec const& a,
                         basic_bitvec const& b) noexcept {
    assert(a.size() == b.size());

    if (a.empty() && b.empty()) {
      return true;
    }

    for (auto i = size_type{0U}; i != a.blocks_.size() - 1; ++i) {
      if (a.blocks_[i] != b.blocks_[i]) {
        return false;
      }
    }
    return a.sanitized_last_block() == b.sanitized_last_block();
  }

  friend bool operator<(basic_bitvec const& a, basic_bitvec const& b) noexcept {
    assert(a.size() == b.size());
    if (a.empty() && b.empty()) {
      return false;
    }

    auto const a_last = a.sanitized_last_block();
    auto const b_last = b.sanitized_last_block();
    if (a_last < b_last) {
      return true;
    } else if (b_last < a_last) {
      return false;
    }

    for (int i = a.blocks_.size() - 2; i != -1; --i) {
      if (a.blocks_[i] < b.blocks_[i]) {
        return true;
      } else if (b.blocks_[i] < a.blocks_[i]) {
        return false;
      }
    }

    return false;
  }
  friend bool operator!=(basic_bitvec const& a,
                         basic_bitvec const& b) noexcept {
    return !(a == b);
  }

  friend bool operator>(basic_bitvec const& a, basic_bitvec const& b) noexcept {
    return !(a.empty() && b.empty()) && b < a;
  }

  friend bool operator<=(basic_bitvec const& a,
                         basic_bitvec const& b) noexcept {
    return (a.empty() && b.empty()) || !(a > b);
  }

  friend bool operator>=(basic_bitvec const& a,
                         basic_bitvec const& b) noexcept {
    return (a.empty() && b.empty()) || !(a < b);
  }

  basic_bitvec& operator&=(basic_bitvec const& o) noexcept {
    assert(size() == o.size());

    for (auto i = 0U; i < blocks_.size(); ++i) {
      blocks_[i] &= o.blocks_[i];
    }
    return *this;
  }

  basic_bitvec& operator|=(basic_bitvec const& o) noexcept {
    assert(size() == o.size());

    for (auto i = 0U; i < blocks_.size(); ++i) {
      blocks_[i] |= o.blocks_[i];
    }
    return *this;
  }

  basic_bitvec& operator^=(basic_bitvec const& o) noexcept {
    assert(size() == o.size());

    for (auto i = 0U; i < blocks_.size(); ++i) {
      blocks_[i] ^= o.blocks_[i];
    }
    return *this;
  }

  basic_bitvec operator~() const noexcept {
    auto copy = *this;
    for (auto& b : copy.blocks_) {
      b = ~b;
    }
    return copy;
  }

  friend basic_bitvec operator&(basic_bitvec const& lhs,
                                basic_bitvec const& rhs) noexcept {
    auto copy = lhs;
    copy &= rhs;
    return copy;
  }

  friend basic_bitvec operator|(basic_bitvec const& lhs,
                                basic_bitvec const& rhs) noexcept {
    auto copy = lhs;
    copy |= rhs;
    return copy;
  }

  friend basic_bitvec operator^(basic_bitvec const& lhs,
                                basic_bitvec const& rhs) noexcept {
    auto copy = lhs;
    copy ^= rhs;
    return copy;
  }

  basic_bitvec& operator>>=(std::size_t const shift) noexcept {
    if (shift >= size_) {
      reset();
      return *this;
    }

    if ((size_ % bits_per_block) != 0) {
      blocks_[blocks_.size() - 1] = sanitized_last_block();
    }

    if (blocks_.size() == 1U) {
      blocks_[0] >>= shift;
      return *this;
    } else {
      if (shift == 0U) {
        return *this;
      }

      auto const shift_blocks = shift / bits_per_block;
      auto const shift_bits = shift % bits_per_block;
      auto const border = blocks_.size() - shift_blocks - 1U;

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
        blocks_[border] = (blocks_[blocks_.size() - 1] >> shift_bits);
      }

      for (auto i = border + 1; i != blocks_.size(); ++i) {
        blocks_[i] = 0U;
      }

      return *this;
    }
  }

  basic_bitvec& operator<<=(std::size_t const shift) noexcept {
    if (shift >= size_) {
      reset();
      return *this;
    }

    if (blocks_.size() == 1U) {
      blocks_[0] <<= shift;
      return *this;
    } else {
      if (shift == 0U) {
        return *this;
      }

      auto const shift_blocks = shift / bits_per_block;
      auto const shift_bits = shift % bits_per_block;

      if (shift_bits == 0U) {
        for (auto i = size_type{blocks_.size() - 1}; i >= shift_blocks; --i) {
          blocks_[i] = blocks_[i - shift_blocks];
        }
      } else {
        for (auto i = size_type{blocks_.size() - 1}; i != shift_blocks; --i) {
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

  basic_bitvec operator>>(std::size_t const i) const noexcept {
    auto copy = *this;
    copy >>= i;
    return copy;
  }

  basic_bitvec operator<<(std::size_t const i) const noexcept {
    auto copy = *this;
    copy <<= i;
    return copy;
  }

  friend std::ostream& operator<<(std::ostream& out, basic_bitvec const& b) {
    return out << b.str();
  }

  size_type size_{0U};
  Vec blocks_;
};

namespace offset {
using bitvec = basic_bitvec<vector<std::uint64_t>>;
}

namespace raw {
using bitvec = basic_bitvec<vector<std::uint64_t>>;
}

}  // namespace cista
