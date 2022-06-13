#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>

#include "cista/containers/vector.h"
#include "cista/strong.h"

namespace cista {

template <typename DataVec, typename IndexVec>
struct fws_multimap_entry {
  using iterator = typename DataVec::const_iterator;
  using index_t = typename IndexVec::value_type;
  using value_t = typename DataVec::value_type;

  static_assert(!std::is_same_v<std::remove_cv<value_t>, bool>,
                "bool not supported");

  fws_multimap_entry(DataVec const& data, IndexVec const& index,
                     index_t const key)
      : data_{data}, index_start{index[key]}, index_end{index[key + 1]} {}

  fws_multimap_entry(DataVec const& data, index_t const start_index,
                     index_t const end_index)
      : data_{data}, index_start{start_index}, index_end{end_index} {}

  iterator begin() const { return data_.begin() + index_start; }
  iterator end() const { return data_.begin() + index_end; }

  iterator cbegin() const { return begin(); }
  iterator cend() const { return end(); }

  friend iterator begin(fws_multimap_entry const& e) { return e.begin(); }
  friend iterator end(fws_multimap_entry const& e) { return e.end(); }

  value_t const& operator[](index_t const index) const {
    return data_[data_index(index)];
  }

  index_t data_index(index_t index) const {
    assert(index_start + index < data_.size());
    return index_start + index;
  }

  std::size_t size() const { return index_end - index_start; }
  bool empty() const { return size() == 0; }

  DataVec const& data_;
  index_t const index_start;
  index_t const index_end;
};

template <typename MapType, typename EntryType>
struct fws_multimap_iterator {
  using iterator_category = std::random_access_iterator_tag;
  using value_type = EntryType;
  using difference_type = int;
  using pointer = value_type*;
  using reference = value_type&;
  using index_t = typename MapType::index_t;

  fws_multimap_iterator(MapType const& map, index_t const index)
      : map_{map}, index_{index} {}

  value_type operator*() const { return {map_[index_]}; }

  fws_multimap_iterator& operator+=(int n) {
    index_ += n;
    return *this;
  }

  fws_multimap_iterator& operator-=(int n) {
    index_ -= n;
    return *this;
  }

  fws_multimap_iterator& operator++() {
    ++index_;
    return *this;
  }

  fws_multimap_iterator& operator--() {
    --index_;
    return *this;
  }

  fws_multimap_iterator operator+(int n) const {
    return {map_, index_ + static_cast<index_t>(n)};
  }

  fws_multimap_iterator operator-(int n) const {
    return {map_, index_ + static_cast<index_t>(n)};
  }

  int operator-(fws_multimap_iterator const& o) const {
    return index_ - o.index_;
  }

  value_type& operator[](int n) const { return {map_, index_ + n}; }

  bool operator<(fws_multimap_iterator const& o) const {
    return index_ < o.index_;
  }

  bool operator>(fws_multimap_iterator const& o) const {
    return index_ > o.index_;
  }

  bool operator<=(fws_multimap_iterator const& o) const {
    return index_ <= o.index_;
  }

  bool operator>=(fws_multimap_iterator const& o) const {
    return index_ >= o.index_;
  }

  bool operator==(fws_multimap_iterator const& o) const {
    return &map_ == &o.map_ && index_ == o.index_;
  }

  bool operator!=(fws_multimap_iterator const& o) const {
    return !(*this == o);
  }

protected:
  MapType const& map_;
  index_t index_;
};

template <typename DataVec, typename IndexVec>
struct fws_multimap {
  using value_t = typename DataVec::value_type;
  using index_t = typename IndexVec::value_type;
  using entry_t = fws_multimap_entry<DataVec, IndexVec>;
  using iterator = fws_multimap_iterator<fws_multimap, entry_t>;

  template <typename T>
  struct is_unsigned_strong {
    static constexpr auto const value =
        std::is_unsigned_v<typename index_t::value_t>;
  };

  static_assert(
      std::disjunction_v<
          std::is_unsigned<index_t>,
          std::conjunction<is_strong<index_t>, is_unsigned_strong<index_t>>>,
      "index has to be unsigned");

  void push_back(value_t const& val) {
    assert(!complete_);
    data_.push_back(val);
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    data_.emplace_back(std::forward<Args>(args)...);
  }

  index_t current_key() const { return static_cast<index_t>(index_.size()); }

  void finish_key() {
    assert(!complete_);
    index_.push_back(current_start_);
    current_start_ = static_cast<index_t>(data_.size());
  }

  void finish_map() {
    assert(!complete_);
    index_.push_back(static_cast<index_t>(data_.size()));
    complete_ = true;
  }

  void reserve_index(index_t size) {
    index_.reserve(static_cast<std::size_t>(size) + 1);
  }

  entry_t operator[](index_t const index) const {
    assert(index < index_.size() - 1);
    return {data_, index_, index};
  }

  iterator begin() const { return {*this, 0}; }
  iterator end() const { return {*this, index_.size() - 1}; }

  iterator cbegin() const { return begin(); }
  iterator cend() const { return end(); }

  friend iterator begin(fws_multimap const& e) { return e.begin(); }
  friend iterator end(fws_multimap const& e) { return e.end(); }

  std::size_t index_size() const { return index_.size(); }
  std::size_t data_size() const { return data_.size(); }
  bool finished() const { return complete_; }

  DataVec data_;
  IndexVec index_;
  index_t current_start_{0};
  bool complete_{false};
};

namespace offset {

template <typename K, typename V>
using fws_multimap = fws_multimap<vector<V>, vector<K>>;

}  // namespace offset

namespace raw {

template <typename K, typename V>
using fws_multimap = fws_multimap<vector<V>, vector<K>>;

}  // namespace raw

}  // namespace cista
