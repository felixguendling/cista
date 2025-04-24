#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>
#include <vector>

#include "cista/char_traits.h"
#include "cista/const_iterator.h"
#include "cista/containers/vector.h"
#include "cista/cuda_check.h"
#include "cista/verify.h"

namespace cista {

template <typename Key, typename DataVec, typename IndexVec>
struct basic_vecvec {
  using key = Key;
  using data_value_type = typename DataVec::value_type;
  using index_value_type = typename IndexVec::value_type;
  using char_traits = std::conditional_t<std::is_same_v<data_value_type, char>,
                                         ::std::char_traits<data_value_type>,
                                         ::cista::char_traits<data_value_type>>;

  struct bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::iterator;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;
    using reference = bucket;

    CISTA_CUDA_COMPAT bucket(basic_vecvec* map, index_value_type const i)
        : map_{map}, i_{to_idx(i)} {}

    friend CISTA_CUDA_COMPAT data_value_type* data(bucket b) { return &b[0]; }
    friend CISTA_CUDA_COMPAT index_value_type size(bucket b) {
      return b.size();
    }

    CISTA_CUDA_COMPAT data_value_type const* data() const {
      return empty() ? nullptr : &front();
    }

    template <typename T = std::decay_t<data_value_type>,
              typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
    std::basic_string_view<T, char_traits> view() const {
      return {begin(), size()};
    }

    CISTA_CUDA_COMPAT value_type& front() {
      assert(!empty());
      return operator[](0);
    }

    CISTA_CUDA_COMPAT value_type& back() {
      assert(!empty());
      return operator[](size() - 1U);
    }

    CISTA_CUDA_COMPAT value_type const& front() const {
      assert(!empty());
      return operator[](0);
    }

    CISTA_CUDA_COMPAT value_type const& back() const {
      assert(!empty());
      return operator[](size() - 1U);
    }

    CISTA_CUDA_COMPAT bool empty() const { return begin() == end(); }

    template <typename Args>
    void push_back(Args&& args) {
      map_->data_.insert(std::next(std::begin(map_->data_), bucket_end_idx()),
                         std::forward<Args>(args));
      for (auto i = i_ + 1; i != map_->bucket_starts_.size(); ++i) {
        ++map_->bucket_starts_[i];
      }
    }

    void grow(std::size_t const n, value_type const& value = value_type{}) {
      verify(n >= size(), "bucket::grow: new size < old size");
      auto const growth = n - size();

      map_->data_.insert(
          std::next(std::begin(map_->data_), bucket_end_idx()),
          static_cast<typename decltype(map_->data_)::size_type>(growth),
          value);
      for (auto i = i_ + 1; i != map_->bucket_starts_.size(); ++i) {
        map_->bucket_starts_[i] += growth;
      }
    }

    CISTA_CUDA_COMPAT value_type& operator[](std::size_t const i) {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
    }

    CISTA_CUDA_COMPAT value_type const& operator[](std::size_t const i) const {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
    }

    CISTA_CUDA_COMPAT value_type const& at(std::size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    CISTA_CUDA_COMPAT value_type& at(std::size_t const i) {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    CISTA_CUDA_COMPAT std::size_t size() const {
      return bucket_end_idx() - bucket_begin_idx();
    }
    CISTA_CUDA_COMPAT iterator begin() {
      return map_->data_.begin() + bucket_begin_idx();
    }
    CISTA_CUDA_COMPAT iterator end() {
      return map_->data_.begin() + bucket_end_idx();
    }
    CISTA_CUDA_COMPAT const_iterator begin() const {
      return map_->data_.begin() + bucket_begin_idx();
    }
    CISTA_CUDA_COMPAT const_iterator end() const {
      return map_->data_.begin() + bucket_end_idx();
    }
    CISTA_CUDA_COMPAT friend iterator begin(bucket const& b) {
      return b.begin();
    }
    CISTA_CUDA_COMPAT friend iterator end(bucket const& b) { return b.end(); }
    CISTA_CUDA_COMPAT friend iterator begin(bucket& b) { return b.begin(); }
    CISTA_CUDA_COMPAT friend iterator end(bucket& b) { return b.end(); }

    friend bool operator==(bucket const& a, bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ == b.i_;
    }
    friend bool operator!=(bucket const& a, bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ != b.i_;
    }
    CISTA_CUDA_COMPAT bucket& operator++() {
      ++i_;
      return *this;
    }
    CISTA_CUDA_COMPAT bucket& operator--() {
      --i_;
      return *this;
    }
    CISTA_CUDA_COMPAT bucket operator*() const { return *this; }
    CISTA_CUDA_COMPAT bucket& operator+=(difference_type const n) {
      i_ += n;
      return *this;
    }
    CISTA_CUDA_COMPAT bucket& operator-=(difference_type const n) {
      i_ -= n;
      return *this;
    }
    CISTA_CUDA_COMPAT bucket operator+(difference_type const n) const {
      auto tmp = *this;
      tmp += n;
      return tmp;
    }
    CISTA_CUDA_COMPAT bucket operator-(difference_type const n) const {
      auto tmp = *this;
      tmp -= n;
      return tmp;
    }
    CISTA_CUDA_COMPAT friend difference_type operator-(bucket const& a,
                                                       bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ - b.i_;
    }

  private:
    CISTA_CUDA_COMPAT index_value_type bucket_begin_idx() const {
      return map_->empty() ? index_value_type{}
                           : to_idx(map_->bucket_starts_[i_]);
    }
    CISTA_CUDA_COMPAT index_value_type bucket_end_idx() const {
      return map_->empty() ? index_value_type{}
                           : to_idx(map_->bucket_starts_[i_ + 1U]);
    }
    CISTA_CUDA_COMPAT bool is_inside_bucket(std::size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    basic_vecvec* map_;
    index_value_type i_;
  };

  struct const_bucket final {
    using value_type = data_value_type;
    using iterator = const_iterator_t<DataVec>;
    using const_iterator = iterator;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference<value_type>;

    CISTA_CUDA_COMPAT const_bucket(basic_vecvec const* map,
                                   index_value_type const i)
        : map_{map}, i_{to_idx(i)} {}

    friend CISTA_CUDA_COMPAT data_value_type const* data(const_bucket b) {
      return b.data();
    }
    friend CISTA_CUDA_COMPAT index_value_type size(const_bucket b) {
      return b.size();
    }

    CISTA_CUDA_COMPAT data_value_type const* data() const {
      return empty() ? nullptr : &front();
    }

    template <typename T = std::decay_t<data_value_type>,
              typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
    std::basic_string_view<T, char_traits> view() const {
      return {begin(), size()};
    }

    CISTA_CUDA_COMPAT value_type const& front() const {
      assert(!empty());
      return operator[](0);
    }

    CISTA_CUDA_COMPAT value_type const& back() const {
      assert(!empty());
      return operator[](size() - 1U);
    }

    CISTA_CUDA_COMPAT bool empty() const { return begin() == end(); }

    CISTA_CUDA_COMPAT value_type const& at(std::size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    CISTA_CUDA_COMPAT value_type const& operator[](std::size_t const i) const {
      assert(is_inside_bucket(i));
      return map_->data_[map_->bucket_starts_[i_] + i];
    }

    CISTA_CUDA_COMPAT index_value_type size() const {
      return bucket_end_idx() - bucket_begin_idx();
    }
    CISTA_CUDA_COMPAT const_iterator begin() const {
      return map_->data_.begin() + bucket_begin_idx();
    }
    CISTA_CUDA_COMPAT const_iterator end() const {
      return map_->data_.begin() + bucket_end_idx();
    }
    friend CISTA_CUDA_COMPAT const_iterator begin(const_bucket const& b) {
      return b.begin();
    }
    friend CISTA_CUDA_COMPAT const_iterator end(const_bucket const& b) {
      return b.end();
    }

    std::reverse_iterator<const_iterator> rbegin() const {
      return std::reverse_iterator{begin() + size()};
    }
    std::reverse_iterator<const_iterator> rend() const {
      return std::reverse_iterator{begin()};
    }

    friend bool operator==(const_bucket const& a, const_bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ == b.i_;
    }
    friend bool operator!=(const_bucket const& a, const_bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ != b.i_;
    }
    CISTA_CUDA_COMPAT const_bucket& operator++() {
      ++i_;
      return *this;
    }
    CISTA_CUDA_COMPAT const_bucket& operator--() {
      --i_;
      return *this;
    }
    CISTA_CUDA_COMPAT const_bucket operator*() const { return *this; }
    CISTA_CUDA_COMPAT const_bucket& operator+=(difference_type const n) {
      i_ += n;
      return *this;
    }
    CISTA_CUDA_COMPAT const_bucket& operator-=(difference_type const n) {
      i_ -= n;
      return *this;
    }
    CISTA_CUDA_COMPAT const_bucket operator+(difference_type const n) const {
      auto tmp = *this;
      tmp += n;
      return tmp;
    }
    CISTA_CUDA_COMPAT const_bucket operator-(difference_type const n) const {
      auto tmp = *this;
      tmp -= n;
      return tmp;
    }
    friend CISTA_CUDA_COMPAT difference_type operator-(const_bucket const& a,
                                                       const_bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ - b.i_;
    }

  private:
    CISTA_CUDA_COMPAT std::size_t bucket_begin_idx() const {
      return to_idx(map_->bucket_starts_[i_]);
    }
    CISTA_CUDA_COMPAT std::size_t bucket_end_idx() const {
      return to_idx(map_->bucket_starts_[i_ + 1]);
    }
    CISTA_CUDA_COMPAT bool is_inside_bucket(std::size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    std::size_t i_;
    basic_vecvec const* map_;
  };

  using value_type = bucket;
  using iterator = bucket;
  using const_iterator = const_bucket;

  CISTA_CUDA_COMPAT bucket operator[](Key const i) {
    return bucket{this, to_idx(i)};
  }
  CISTA_CUDA_COMPAT const_bucket operator[](Key const i) const {
    return const_bucket{this, to_idx(i)};
  }

  CISTA_CUDA_COMPAT const_bucket at(Key const i) const {
    verify(to_idx(i) < bucket_starts_.size(),
           "basic_vecvec::at: index out of range");
    return {this, to_idx(i)};
  }

  CISTA_CUDA_COMPAT bucket at(Key const i) {
    verify(to_idx(i) < bucket_starts_.size(),
           "basic_vecvec::at: index out of range");
    return {this, to_idx(i)};
  }

  CISTA_CUDA_COMPAT bucket front() { return at(Key{0}); }
  CISTA_CUDA_COMPAT bucket back() { return at(Key{size() - 1}); }

  CISTA_CUDA_COMPAT const_bucket front() const { return at(Key{0}); }
  CISTA_CUDA_COMPAT const_bucket back() const { return at(Key{size() - 1}); }

  CISTA_CUDA_COMPAT index_value_type size() const {
    return empty() ? 0U : bucket_starts_.size() - 1;
  }
  CISTA_CUDA_COMPAT bool empty() const { return bucket_starts_.empty(); }

  void clear() {
    bucket_starts_.clear();
    data_.clear();
  }

  template <typename Container,
            typename = std::enable_if_t<std::is_convertible_v<
                decltype(*std::declval<Container>().begin()), data_value_type>>>
  void emplace_back(Container&& bucket) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(index_value_type{0U});
    }
    data_.insert(std::end(data_),  //
                 std::make_move_iterator(std::begin(bucket)),
                 std::make_move_iterator(std::end(bucket)));
    bucket_starts_.emplace_back(static_cast<index_value_type>(data_.size()));
  }

  bucket add_back_sized(std::size_t const bucket_size) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(index_value_type{0U});
    }
    data_.resize(data_.size() + bucket_size);
    bucket_starts_.emplace_back(static_cast<index_value_type>(data_.size()));
    return at(Key{size() - 1U});
  }

  template <typename X>
  std::enable_if_t<std::is_convertible_v<std::decay_t<X>, data_value_type>>
  emplace_back(std::initializer_list<X>&& x) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(index_value_type{0U});
    }
    bucket_starts_.emplace_back(
        static_cast<index_value_type>(data_.size() + x.size()));
    data_.insert(std::end(data_),  //
                 std::make_move_iterator(std::begin(x)),
                 std::make_move_iterator(std::end(x)));
  }

  template <typename T = data_value_type,
            typename = std::enable_if_t<std::is_convertible_v<T, char const>>>
  void emplace_back(char const* s) {
    return emplace_back(std::string_view{s});
  }

  void resize(std::size_t const new_size) {
    auto const old_size = bucket_starts_.size();
    bucket_starts_.resize(
        static_cast<typename IndexVec::size_type>(new_size + 1U));
    for (auto i = old_size; i < new_size + 1U; ++i) {
      bucket_starts_[i] = data_.size();
    }
  }

  CISTA_CUDA_COMPAT
  bucket begin() { return bucket{this, 0U}; }
  CISTA_CUDA_COMPAT
  bucket end() { return bucket{this, size()}; }
  CISTA_CUDA_COMPAT
  const_bucket begin() const { return const_bucket{this, 0U}; }
  CISTA_CUDA_COMPAT
  const_bucket end() const { return const_bucket{this, size()}; }

  CISTA_CUDA_COMPAT
  friend bucket begin(basic_vecvec& m) { return m.begin(); }
  CISTA_CUDA_COMPAT
  friend bucket end(basic_vecvec& m) { return m.end(); }
  CISTA_CUDA_COMPAT
  friend const_bucket begin(basic_vecvec const& m) { return m.begin(); }
  CISTA_CUDA_COMPAT
  friend const_bucket end(basic_vecvec const& m) { return m.end(); }

  DataVec data_;
  IndexVec bucket_starts_;
};

namespace offset {

template <typename K, typename V, typename SizeType = base_t<K>>
using vecvec = basic_vecvec<K, vector<V>, vector<SizeType>>;

}  // namespace offset

namespace raw {

template <typename K, typename V, typename SizeType = base_t<K>>
using vecvec = basic_vecvec<K, vector<V>, vector<SizeType>>;

}  // namespace raw

}  // namespace cista
