#pragma once

#include <cassert>

#include "cista/mmap.h"
#include "cista/strong.h"

namespace cista {

template <typename T, typename Key = std::uint32_t>
struct basic_mmap_vec {
  using size_type = base_t<Key>;
  using difference_type = std::ptrdiff_t;
  using access_type = Key;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T*;
  using value_type = T;
  using iterator = T*;
  using const_iterator = T const*;

  static_assert(std::is_trivially_copyable_v<T>);

  explicit basic_mmap_vec(cista::mmap mmap)
      : mmap_{std::move(mmap)},
        used_size_{static_cast<size_type>(mmap_.size() / sizeof(T))} {}

  void push_back(T const& t) {
    ++used_size_;
    mmap_.resize(sizeof(T) * used_size_);
    (*this)[Key{used_size_ - 1U}] = t;
  }

  template <typename... Args>
  T& emplace_back(Args&&... el) {
    reserve(used_size_ + 1U);
    new (data() + used_size_) T{std::forward<Args>(el)...};
    T* ptr = data() + used_size_;
    ++used_size_;
    return *ptr;
  }

  size_type size() const { return used_size_; }

  T const* data() const noexcept { return begin(); }
  T* data() noexcept { return begin(); }
  T const* begin() const noexcept {
    return reinterpret_cast<T const*>(mmap_.data());
  }
  T const* end() const noexcept { return begin() + used_size_; }  // NOLINT
  T const* cbegin() const noexcept { return begin(); }
  T const* cend() const noexcept { return begin() + used_size_; }  // NOLINT
  T* begin() noexcept { return reinterpret_cast<T*>(mmap_.data()); }
  T* end() noexcept { return begin() + used_size_; }  // NOLINT

  friend T const* begin(basic_mmap_vec const& a) noexcept { return a.begin(); }
  friend T const* end(basic_mmap_vec const& a) noexcept { return a.end(); }

  friend T* begin(basic_mmap_vec& a) noexcept { return a.begin(); }
  friend T* end(basic_mmap_vec& a) noexcept { return a.end(); }

  bool empty() const noexcept { return size() == 0U; }

  T const& operator[](access_type const index) const noexcept {
    assert(index < used_size_);
    return begin()[to_idx(index)];
  }

  T& operator[](access_type const index) noexcept {
    assert(used_size_);
    return begin()[to_idx(index)];
  }

  void reserve(size_type const size) { mmap_.resize(size * sizeof(T)); }

  void resize(size_type const size) {
    mmap_.resize(size * sizeof(T));
    for (auto i = used_size_; i < size; ++i) {
      new (data() + i) T{};
    }
    used_size_ = size;
  }

  template <typename It>
  void set(It begin_it, It end_it) {
    using diff_t =
        std::common_type_t<typename std::iterator_traits<It>::difference_type,
                           size_type>;
    auto const range_size = std::distance(begin_it, end_it);
    verify(range_size >= 0 &&
               static_cast<diff_t>(range_size) <=
                   static_cast<diff_t>(std::numeric_limits<size_type>::max()),
           "cista::vector::set: invalid range");

    reserve(static_cast<size_type>(range_size));

    auto copy_source = begin_it;
    auto copy_target = data();
    for (; copy_source != end_it; ++copy_source, ++copy_target) {
      new (copy_target) T{std::forward<decltype(*copy_source)>(*copy_source)};
    }

    used_size_ = static_cast<size_type>(range_size);
  }

  template <typename Arg>
  T* insert(T* it, Arg&& el) {
    auto const old_offset = std::distance(begin(), it);
    auto const old_size = used_size_;

    reserve(used_size_ + 1);
    new (data() + used_size_) T{std::forward<Arg&&>(el)};
    ++used_size_;

    return std::rotate(begin() + old_offset, begin() + old_size, end());
  }

  template <class InputIt>
  T* insert(T* pos, InputIt first, InputIt last, std::input_iterator_tag) {
    auto const old_offset = std::distance(begin(), pos);
    auto const old_size = used_size_;

    for (; !(first == last); ++first) {
      reserve(used_size_ + 1);
      new (data() + used_size_) T{std::forward<decltype(*first)>(*first)};
      ++used_size_;
    }

    return std::rotate(begin() + old_offset, begin() + old_size, end());
  }

  template <class FwdIt>
  T* insert(T* pos, FwdIt first, FwdIt last, std::forward_iterator_tag) {
    if (empty()) {
      set(first, last);
      return begin();
    }

    auto const pos_idx = pos - begin();
    auto const new_count = static_cast<size_type>(std::distance(first, last));
    reserve(used_size_ + new_count);
    pos = begin() + pos_idx;

    for (auto src_last = end() - 1, dest_last = end() + new_count - 1;
         !(src_last == pos - 1); --src_last, --dest_last) {
      if (dest_last >= end()) {
        new (dest_last) T(std::move(*src_last));
      } else {
        *dest_last = std::move(*src_last);
      }
    }

    for (auto insert_ptr = pos; !(first == last); ++first, ++insert_ptr) {
      if (insert_ptr >= end()) {
        new (insert_ptr) T(std::forward<decltype(*first)>(*first));
      } else {
        *insert_ptr = std::forward<decltype(*first)>(*first);
      }
    }

    used_size_ += new_count;

    return pos;
  }

  template <class It>
  T* insert(T* pos, It first, It last) {
    return insert(pos, first, last,
                  typename std::iterator_traits<It>::iterator_category());
  }

  cista::mmap mmap_;
  size_type used_size_{0U};
};

template <typename T>
using mmap_vec = basic_mmap_vec<T>;

template <typename Key, typename T>
using mmap_vec_map = basic_mmap_vec<Key, T>;

}  // namespace cista