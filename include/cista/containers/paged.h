#pragma once

#include <cinttypes>
#include <cstring>
#include <limits>

#include "cista/bit_counting.h"
#include "cista/containers/array.h"
#include "cista/next_power_of_2.h"
#include "cista/verify.h"

namespace cista {

template <typename SizeType, typename PageSizeType>
struct page {
  bool valid() const { return capacity_ != 0U; }
  PageSizeType size() const noexcept { return size_; }

  PageSizeType size_{0U};
  PageSizeType capacity_{0U};
  SizeType start_{0U};
};

template <typename DataVec, typename SizeType = typename DataVec::size_type,
          typename PageSizeType = std::uint16_t,
          PageSizeType MinPageSize =
              std::max(std::size_t{2U},
                       next_power_of_two(sizeof(page<SizeType, PageSizeType>) /
                                         sizeof(typename DataVec::value_type))),
          PageSizeType MaxPageSize = 4096>
struct paged {
  using value_type = typename DataVec::value_type;
  using iterator = typename DataVec::iterator;
  using const_iterator = typename DataVec::const_iterator;
  using reference = typename DataVec::reference;
  using const_reference = typename DataVec::const_reference;
  using size_type = SizeType;
  using page_size_type = PageSizeType;
  using page_t = page<SizeType, PageSizeType>;

  static_assert(sizeof(value_type) * MinPageSize >= sizeof(page_t));
  static_assert(std::is_trivially_copyable_v<value_type>);

  static constexpr std::size_t free_list_index(size_type const capacity) {
    return static_cast<size_type>(constexpr_trailing_zeros(capacity) -
                                  constexpr_trailing_zeros(MinPageSize));
  }

  static constexpr auto const free_list_size =
      free_list_index(MaxPageSize) + 1U;

  page_t resize_page(page_t const& p, PageSizeType const size) {
    if (size <= p.capacity_) {
      return {size, p.capacity_, p.start_};
    } else {
      auto const new_page = create_page(size);
      copy(new_page, p);
      free_page(p);
      return new_page;
    }
  }

  page_t create_page(PageSizeType const size) {
    auto const capacity = next_power_of_two(std::max(MinPageSize, size));
    auto const i = free_list_index(capacity);
    verify(i < free_list_.size(), "paged::create_page: size > max capacity");
    if (!free_list_[i].empty()) {
      auto const start = free_list_[i].pop(*this);
      return {size, capacity, start};
    } else {
      auto const start = data_.size();
      data_.resize(data_.size() + capacity);
      return {size, capacity, start};
    }
  }

  void free_page(page_t const& p) {
    if (!p.valid()) {
      return;
    }
    auto const i = free_list_index(p.capacity_);
    verify(i < free_list_.size(), "paged::free_page: size > max capacity");
    free_list_[i].push(*this, p.start_);
  }

  template <typename T>
  T read(size_type const offset) {
    static_assert(std::is_trivially_copyable_v<T>);
    auto x = T{};
    std::memcpy(&x, &data_[offset], sizeof(x));
    return x;
  }

  template <typename T>
  void write(size_type const offset, T const& x) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memcpy(&data_[offset], &x, sizeof(T));
  }

  value_type* data(page_t const& p) { return &data_[p.start_]; }
  value_type const* data(page_t const& p) const { return &data_[p.start_]; }

  value_type* begin(page_t const& p) { return data(p); }
  value_type const* begin(page_t const& p) const { return data(p); }

  value_type* end(page_t& p) const { return begin(p) + p.size(); }
  value_type const* end(page_t const& p) const { return begin() + p.size; }

  void copy(page_t const& to, page_t const& from) {
    std::memcpy(data(to), data(from), from.size() * sizeof(value_type));
  }

  template <typename ItA, typename ItB>
  void copy(page_t const& to, ItA begin, ItB end) {
    auto const n = static_cast<std::size_t>(std::distance(begin, end));
    if (n != 0U) {
      std::memcpy(data(to), &*begin, n * sizeof(value_type));
    }
  }

  void clear() {
    data_.clear();
    for (auto& n : free_list_) {
      n.next_ = std::numeric_limits<size_type>::max();
    }
  }

  struct node {
    bool empty() const {
      return next_ == std::numeric_limits<size_type>::max();
    }

    void push(paged& m, size_type const start) {
      m.write(start, next_);
      next_ = start;
    }

    size_type pop(paged& m) {
      verify(!empty(), "paged: invalid read access to empty free list entry");
      auto const next_start = m.read<size_type>(next_);
      auto start = next_;
      next_ = next_start;
      return start;
    }

    size_type next_{std::numeric_limits<size_type>::max()};
  };

  DataVec data_;
  array<node, free_list_size> free_list_{};
};

}  // namespace cista