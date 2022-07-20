#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include "cista/bit_counting.h"
#include "cista/containers/array.h"
#include "cista/containers/vector.h"
#include "cista/next_power_of_2.h"
#include "cista/verify.h"

namespace cista {

template <typename T, typename SizeType, template <typename> typename Vec,
          std::size_t Log2MaxEntriesPerBucket = 20>
struct dynamic_fws_multimap_base {
  using value_type = T;
  using size_type = SizeType;
  using DataVec = Vec<value_type>;
  static constexpr auto const MAX_ENTRIES_PER_BUCKET =
      static_cast<size_type>(1ULL << Log2MaxEntriesPerBucket);

  struct index_type {
    size_type begin_{};
    size_type size_{};
    size_type capacity_{};
  };
  using IndexVec = Vec<index_type>;

  template <bool Const>
  struct bucket {
    friend dynamic_fws_multimap_base;

    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::const_iterator;

    template <bool IsConst = Const, typename = std::enable_if_t<IsConst>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    bucket(bucket<false> const& b) : multimap_{b.multimap_}, index_{b.index_} {}

    size_type index() const { return index_; }
    size_type size() const { return get_index().size_; }
    size_type capacity() const { return get_index().capacity_; }
    [[nodiscard]] bool empty() const { return size() == 0; }

    iterator begin() {
      return mutable_mm().data_.begin() + to_idx(get_index().begin_);
    }

    const_iterator begin() const {
      return multimap_.data_.begin() + to_idx(get_index().begin_);
    }

    iterator end() {
      auto const& index = get_index();
      return std::next(mutable_mm().data_.begin(),
                       to_idx(index.begin_ + index.size_));
    }

    const_iterator end() const {
      auto const& index = get_index();
      return std::next(multimap_.data_.begin(),
                       to_idx(index.begin_ + index.size_));
    }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    friend iterator begin(bucket& b) { return b.begin(); }
    friend const_iterator begin(bucket const& b) { return b.begin(); }
    friend iterator end(bucket& b) { return b.end(); }
    friend const_iterator end(bucket const& b) { return b.end(); }

    value_type& operator[](size_type index) {
      return mutable_mm().data_[data_index(index)];
    }

    value_type const& operator[](size_type index) const {
      return multimap_.data_[data_index(index)];
    }

    value_type& at(size_type index) {
      return mutable_mm().data_[get_and_check_data_index(index)];
    }

    value_type const& at(size_type index) const {
      return multimap_.data_[get_and_check_data_index(index)];
    }

    value_type& front() { return (*this)[0]; }
    value_type const& front() const { return (*this)[0]; }

    value_type& back() {
      assert(!empty());
      return (*this)[size() - 1];
    }

    value_type const& back() const {
      assert(!empty());
      return (*this)[size() - 1];
    }

    size_type data_index(size_type index) const {
      assert(index < get_index().size_);
      return get_index().begin_ + index;
    }

    size_type bucket_index(const_iterator it) const {
      if (it < begin() || it >= end()) {
        throw std::out_of_range{
            "dynamic_fws_multimap::bucket::bucket_index() out of range"};
      }
      return std::distance(begin(), it);
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    size_type push_back(value_type const& val) {
      return mutable_mm().push_back_entry(index_, val);
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>,
              typename... Args>
    size_type emplace_back(Args&&... args) {
      return mutable_mm().emplace_back_entry(index_,
                                             std::forward<Args>(args)...);
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    void insert(iterator it, value_type const& val) {
      prepare_insert(it) = val;
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    void insert(iterator it, value_type&& val) {
      prepare_insert(it) = std::move(val);
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    void reserve(size_type new_size) {
      if (new_size > capacity()) {
        mutable_mm().grow_bucket(index_, get_index(), new_size);
      }
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    void resize(size_type new_size, value_type init = value_type{}) {
      auto const old_size = size();
      reserve(new_size);
      auto& index = get_index();
      auto& data = mutable_mm().data_;
      if (new_size < old_size) {
        for (auto i = new_size; i < old_size; ++i) {
          data[index.begin_ + i].~T();
        }
        mutable_mm().element_count_ -= old_size - new_size;
      } else if (new_size > old_size) {
        for (auto i = old_size; i < new_size; ++i) {
          data[index.begin_ + i] = value_type{init};
        }
        mutable_mm().element_count_ += new_size - old_size;
      }
      index.size_ = new_size;
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    void pop_back() {
      if (!empty()) {
        resize(size() - 1);
      }
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    void clear() {
      auto& index = get_index();
      auto& data = mutable_mm().data_;
      for (auto i = index.begin_; i < index.begin_ + index.size_; ++i) {
        data[i].~T();
      }
      mutable_mm().element_count_ -= index.size_;
      index.size_ = 0;
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    iterator erase(iterator pos) {
      auto last = std::prev(end());
      while (pos < last) {
        std::swap(*pos, *std::next(pos));
        pos = std::next(pos);
      }
      (*pos).~T();
      get_index().size_--;
      mutable_mm().element_count_--;
      return end();
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    iterator erase(iterator first, iterator last) {
      if (first != last) {
        auto const new_end = std::move(last, end(), first);
        for (auto it = new_end; it != end(); it = std::next(it)) {
          (*it).~T();
        }
        auto const count = std::distance(new_end, end());
        get_index().size_ -= count;
        mutable_mm().element_count_ -= count;
      }
      return end();
    }

  protected:
    bucket(dynamic_fws_multimap_base const& multimap, size_type index)
        : multimap_(multimap), index_(index) {}

    index_type& get_index() { return mutable_mm().index_[index_]; }
    index_type const& get_index() const { return multimap_.index_[index_]; }

    size_type get_and_check_data_index(size_type index) const {
      auto const& idx = get_index();
      if (index >= idx.size_) {
        throw std::out_of_range{
            "dynamic_fws_multimap::bucket::at() out of range"};
      }
      return idx.begin_ + index;
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    value_type& prepare_insert(bucket::iterator it) {
      auto const pos = std::distance(begin(), it);
      auto& index = get_index();
      reserve(index.size_ + 1);
      it = std::next(begin(), pos);
      std::move_backward(it, end(), std::next(end()));
      index.size_++;
      mutable_mm().element_count_++;
      return *it;
    }

    dynamic_fws_multimap_base& mutable_mm() {
      return const_cast<dynamic_fws_multimap_base&>(multimap_);  // NOLINT
    }

    dynamic_fws_multimap_base const& multimap_;
    size_type index_;
  };

  using mutable_bucket = bucket<false>;
  using const_bucket = bucket<true>;

  template <bool Const>
  struct bucket_iterator {
    friend dynamic_fws_multimap_base;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = bucket<Const>;
    using difference_type = int;
    using pointer = value_type;
    using reference = value_type;

    template <bool IsConst = Const, typename = std::enable_if_t<IsConst>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    bucket_iterator(bucket_iterator<false> const& it)
        : multimap_{it.multimap_}, index_{it.index_} {}

    value_type operator*() const { return multimap_.at(index_); }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    value_type operator*() {
      return const_cast<dynamic_fws_multimap_base&>(multimap_)  // NOLINT
          .at(index_);
    }

    value_type operator->() const { return multimap_.at(index_); }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    value_type operator->() {
      return const_cast<dynamic_fws_multimap_base&>(multimap_)  // NOLINT
          .at(index_);
    }

    bucket_iterator& operator+=(difference_type n) {
      index_ += n;
      return *this;
    }

    bucket_iterator& operator-=(difference_type n) {
      index_ -= n;
      return *this;
    }

    bucket_iterator& operator++() {
      ++index_;
      return *this;
    }

    bucket_iterator operator++(int) {
      auto old = *this;
      ++(*this);
      return old;
    }

    bucket_iterator& operator--() {
      ++index_;
      return *this;
    }

    bucket_iterator operator--(int) {
      auto old = *this;
      --(*this);
      return old;
    }

    bucket_iterator operator+(difference_type n) const {
      return {multimap_, index_ + n};
    }

    bucket_iterator operator-(difference_type n) const {
      return {multimap_, index_ - n};
    }

    difference_type operator-(bucket_iterator const& rhs) const {
      return static_cast<difference_type>(index_) -
             static_cast<difference_type>(rhs.index_);
    }

    value_type operator[](difference_type n) const {
      return multimap_.at(index_ + n);
    }

    template <bool IsConst = Const, typename = std::enable_if_t<!IsConst>>
    value_type operator[](difference_type n) {
      return const_cast<dynamic_fws_multimap_base&>(multimap_)  // NOLINT
          .at(index_ + n);
    }

    bool operator<(bucket_iterator const& rhs) const {
      return index_ < rhs.index_;
    }
    bool operator<=(bucket_iterator const& rhs) const {
      return index_ <= rhs.index_;
    }
    bool operator>(bucket_iterator const& rhs) const {
      return index_ > rhs.index_;
    }
    bool operator>=(bucket_iterator const& rhs) const {
      return index_ >= rhs.index_;
    }

    bool operator==(bucket_iterator const& rhs) const {
      return index_ == rhs.index_ && &multimap_ == &rhs.multimap_;
    }

    bool operator!=(bucket_iterator const& rhs) const {
      return index_ != rhs.index_ || &multimap_ != &rhs.multimap_;
    }

  protected:
    bucket_iterator(dynamic_fws_multimap_base const& multimap, size_type index)
        : multimap_{multimap}, index_{index} {}

    dynamic_fws_multimap_base const& multimap_;
    size_type index_;
  };

  using iterator = bucket_iterator<false>;
  using const_iterator = bucket_iterator<true>;

  mutable_bucket operator[](size_type index) {
    if (index >= index_.size()) {
      index_.resize(index + 1);
    }
    return {*this, index};
  }

  const_bucket operator[](size_type index) const {
    assert(index < index_.size());
    return {*this, index};
  }

  mutable_bucket at(size_type index) {
    if (index >= index_.size()) {
      throw std::out_of_range{"dynamic_fws_multimap::at() out of range"};
    } else {
      return {*this, index};
    }
  }

  const_bucket at(size_type index) const {
    if (index >= index_.size()) {
      throw std::out_of_range{"dynamic_fws_multimap::at() out of range"};
    } else {
      return {*this, index};
    }
  }

  mutable_bucket front() { return (*this)[0]; }
  const_bucket front() const { return (*this)[0]; }

  mutable_bucket back() { return (*this)[index_size() - 1]; }
  const_bucket back() const { return (*this)[index_size() - 1]; }

  mutable_bucket emplace_back() { return (*this)[index_size()]; }

  mutable_bucket get_or_create(size_type const index) {
    verify(index != std::numeric_limits<size_type>::max(),
           "mutable_fws_multimap::get_or_create: type bound");
    if (index + 1 >= index_.size()) {
      index_.resize(index + 1);
    }
    return {*this, index};
  }

  void erase(size_type const i) {
    if (i < index_.size()) {
      release_bucket(index_[i]);
    }
  }

  size_type index_size() const { return index_.size(); }
  size_type data_size() const { return data_.size(); }
  size_type element_count() const { return element_count_; }
  [[nodiscard]] bool empty() const { return index_size() == 0; }

  std::size_t allocated_size() const {
    auto size = index_.allocated_size_ * sizeof(index_type) +
                data_.allocated_size_ * sizeof(value_type);
    for (auto const& v : free_buckets_) {
      size += v.allocated_size_ * sizeof(index_type);
    }
    return size;
  }

  size_type max_entries_per_bucket() const { return MAX_ENTRIES_PER_BUCKET; }

  size_type max_entries_per_bucket_log2() const {
    return Log2MaxEntriesPerBucket;
  }

  iterator begin() { return {*this, size_type{0}}; }
  const_iterator begin() const { return {*this, size_type{0}}; }
  iterator end() {
    return iterator{*this, static_cast<size_type>(index_.size())};
  }
  const_iterator end() const {
    return const_iterator{*this, static_cast<size_type>(index_.size())};
  }

  friend iterator begin(dynamic_fws_multimap_base& m) { return m.begin(); }
  friend const_iterator begin(dynamic_fws_multimap_base const& m) {
    return m.begin();
  }

  friend iterator end(dynamic_fws_multimap_base& m) { return m.end(); }
  friend const_iterator end(dynamic_fws_multimap_base const& m) {
    return m.end();
  }

  DataVec& data() { return data_; }
  DataVec const& data() const { return data_; }

  void reserve(size_type index, size_type data) {
    index_.reserve(index);
    data_.reserve(data);
  }

protected:
  size_type insert_new_entry(size_type const map_index) {
    assert(map_index < index_.size());
    auto& idx = index_[map_index];
    if (idx.size_ == idx.capacity_) {
      grow_bucket(map_index, idx);
    }
    auto const data_index = idx.begin_ + idx.size_;
    idx.size_++;
    assert(idx.size_ <= idx.capacity_);
    return data_index;
  }

  void grow_bucket(size_type const map_index, index_type& idx) {
    grow_bucket(map_index, idx, idx.capacity_ + 1);
  }

  void grow_bucket(size_type const map_index, index_type& idx,
                   size_type const requested_capacity) {
    assert(requested_capacity > 0);
    auto const new_capacity =
        size_type{cista::next_power_of_two(to_idx(requested_capacity))};
    auto const new_order = get_order(new_capacity);

    verify(new_order <= Log2MaxEntriesPerBucket,
           "dynamic_fws_multimap: too many entries in a bucket");

    auto old_bucket = idx;

    auto free_bucket = get_free_bucket(new_order);
    if (free_bucket) {
      // reuse free bucket
      if (old_bucket.capacity_ != 0) {
        move_entries(map_index, old_bucket.begin_, free_bucket->begin_,
                     idx.size_);
        release_bucket(old_bucket);
      }
      idx.begin_ = free_bucket->begin_;
      idx.capacity_ = free_bucket->capacity_;
    } else {
      if (idx.begin_ + idx.capacity_ == data_.size()) {
        // last bucket -> resize data vector
        auto const additional_capacity = new_capacity - idx.capacity_;
        data_.resize(data_.size() + additional_capacity);
        idx.capacity_ = new_capacity;
      } else {
        // allocate new bucket at the end
        auto const new_begin = data_.size();
        data_.resize(data_.size() + new_capacity);
        move_entries(map_index, idx.begin_, new_begin, idx.size_);
        idx.begin_ = new_begin;
        idx.capacity_ = new_capacity;
        release_bucket(old_bucket);
      }
    }
  }

  std::optional<index_type> get_free_bucket(size_type const requested_order) {
    assert(requested_order <= Log2MaxEntriesPerBucket);

    auto const pop = [](IndexVec& vec) -> std::optional<index_type> {
      if (!vec.empty()) {
        auto it = std::prev(vec.end());
        auto const entry = *it;
        vec.erase(it);
        return entry;
      } else {
        return {};
      }
    };

    return pop(free_buckets_[to_idx(requested_order)]);  // NOLINT
  }

  void release_bucket(index_type& bucket) {
    if (bucket.capacity_ != 0) {
      auto const order = get_order(bucket.capacity_);
      assert(order <= Log2MaxEntriesPerBucket);
      bucket.size_ = size_type{0};
      free_buckets_[to_idx(order)].push_back(index_type{bucket});  // NOLINT
      bucket.capacity_ = size_type{0};
    }
  }

  void move_entries(size_type const /* map_index */,
                    size_type const old_data_index,
                    size_type const new_data_index, size_type const count) {
    if (count == 0) {
      return;
    }
    auto old_data = &data_[old_data_index];
    auto new_data = &data_[new_data_index];
    if constexpr (std::is_trivially_copyable_v<value_type>) {
      std::memcpy(new_data, old_data, to_idx(count) * sizeof(value_type));
    } else {
      for (auto i = static_cast<size_type>(0); i < count;
           ++i, ++old_data, ++new_data) {
        *new_data = value_type(std::move(*old_data));
        old_data->~T();
      }
    }
  }

  size_type push_back_entry(size_type const map_index, value_type const& val) {
    auto const data_index = insert_new_entry(map_index);
    data_[data_index] = val;
    ++element_count_;
    return data_index;
  }

  template <typename... Args>
  size_type emplace_back_entry(size_type const map_index, Args&&... args) {
    auto const data_index = insert_new_entry(map_index);
    data_[data_index] = value_type{std::forward<Args>(args)...};
    ++element_count_;
    return data_index;
  }

  static size_type get_order(size_type const size) {
    return size_type{cista::trailing_zeros(to_idx(size))};
  }

  IndexVec index_;
  DataVec data_;
  array<IndexVec, Log2MaxEntriesPerBucket + 1> free_buckets_;
  size_type element_count_{};
};

namespace offset {

template <typename K, typename V, std::size_t LogMaxBucketSize = 20>
struct mutable_multimap_helper {
  template <typename T>
  using vec = vector_map<K, T>;
  using type = dynamic_fws_multimap_base<V, K, vec, LogMaxBucketSize>;
};

template <typename K, typename V, std::size_t LogMaxBucketSize = 20>
using mutable_fws_multimap =
    typename mutable_multimap_helper<K, V, LogMaxBucketSize>::type;

}  // namespace offset

namespace raw {

template <typename K, typename V, std::size_t LogMaxBucketSize = 20>
struct mutable_multimap_helper {
  template <typename T>
  using vec = vector_map<K, T>;
  using type = dynamic_fws_multimap_base<V, K, vec, LogMaxBucketSize>;
};

template <typename K, typename V, std::size_t LogMaxBucketSize = 20>
using mutable_fws_multimap =
    typename mutable_multimap_helper<K, V, LogMaxBucketSize>::type;

}  // namespace raw

}  // namespace cista
