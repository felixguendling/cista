#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>

#include "cista/containers/vector.h"
#include "cista/verify.h"

namespace cista {

template <typename Key, typename DataVec, typename IndexVec>
struct basic_vecvec {
  using data_value_type = typename DataVec::value_type;
  using index_value_type = typename IndexVec::value_type;

  struct bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::iterator;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;
    using reference = bucket;

    bucket(basic_vecvec* map, index_value_type const i)
        : map_{map}, i_{to_idx(i)} {}

    friend data_value_type* data(bucket b) { return &b[0]; }
    friend index_value_type size(bucket b) { return b.size(); }

    data_value_type const* data() const { return empty() ? nullptr : &front(); }

    template <typename T = std::decay_t<data_value_type>,
              typename = std::enable_if_t<std::is_same_v<T, char>>>
    std::string_view view() const {
      return std::string_view{begin(), size()};
    }

    value_type& front() {
      assert(!empty());
      return operator[](0);
    }

    value_type& back() {
      assert(!empty());
      return operator[](size() - 1U);
    }

    value_type const& front() const {
      assert(!empty());
      return operator[](0);
    }

    value_type const& back() const {
      assert(!empty());
      return operator[](size() - 1U);
    }

    bool empty() const { return begin() == end(); }

    template <typename Args>
    void push_back(Args&& args) {
      map_->data_.insert(std::next(std::begin(map_->data_), bucket_end_idx()),
                         std::forward<Args>(args));
      for (auto i = i_ + 1; i != map_->bucket_starts_.size(); ++i) {
        ++map_->bucket_starts_[i];
      }
    }

    value_type& operator[](std::size_t const i) {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
    }

    value_type const& operator[](std::size_t const i) const {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
    }

    value_type const& at(std::size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type& at(std::size_t const i) {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    std::size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }
    iterator begin() { return map_->data_.begin() + bucket_begin_idx(); }
    iterator end() { return map_->data_.begin() + bucket_end_idx(); }
    const_iterator begin() const {
      return map_->data_.begin() + bucket_begin_idx();
    }
    const_iterator end() const {
      return map_->data_.begin() + bucket_end_idx();
    }
    friend iterator begin(bucket const& b) { return b.begin(); }
    friend iterator end(bucket const& b) { return b.end(); }
    friend iterator begin(bucket& b) { return b.begin(); }
    friend iterator end(bucket& b) { return b.end(); }

    friend bool operator==(bucket const& a, bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ == b.i_;
    }
    friend bool operator!=(bucket const& a, bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ != b.i_;
    }
    bucket& operator++() {
      ++i_;
      return *this;
    }
    bucket& operator--() {
      --i_;
      return *this;
    }
    bucket operator*() const { return *this; }
    bucket& operator+=(difference_type const n) {
      i_ += n;
      return *this;
    }
    bucket& operator-=(difference_type const n) {
      i_ -= n;
      return *this;
    }
    bucket operator+(difference_type const n) const {
      auto tmp = *this;
      tmp += n;
      return tmp;
    }
    bucket operator-(difference_type const n) const {
      auto tmp = *this;
      tmp -= n;
      return tmp;
    }
    friend difference_type operator-(bucket const& a, bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ - b.i_;
    }

  private:
    index_value_type bucket_begin_idx() const {
      return to_idx(map_->bucket_starts_[i_]);
    }
    index_value_type bucket_end_idx() const {
      return to_idx(map_->bucket_starts_[i_ + 1U]);
    }
    bool is_inside_bucket(std::size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    basic_vecvec* map_;
    index_value_type i_;
  };

  struct const_bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::const_iterator;
    using const_iterator = typename DataVec::const_iterator;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference<value_type>;

    const_bucket(basic_vecvec const* map, index_value_type const i)
        : map_{map}, i_{to_idx(i)} {}

    friend data_value_type const* data(const_bucket b) { return b.data(); }
    friend index_value_type size(const_bucket b) { return b.size(); }

    data_value_type const* data() const { return empty() ? nullptr : &front(); }

    template <typename T = std::decay_t<data_value_type>,
              typename = std::enable_if_t<std::is_same_v<T, char>>>
    std::string_view view() const {
      return std::string_view{begin(), size()};
    }

    value_type const& front() const {
      assert(!empty());
      return operator[](0);
    }

    value_type const& back() const {
      assert(!empty());
      return operator[](size() - 1U);
    }

    bool empty() const { return begin() == end(); }

    value_type const& at(std::size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type const& operator[](std::size_t const i) const {
      assert(is_inside_bucket(i));
      return map_->data_[map_->bucket_starts_[i_] + i];
    }

    index_value_type size() const {
      return bucket_end_idx() - bucket_begin_idx();
    }
    const_iterator begin() const {
      return map_->data_.begin() + bucket_begin_idx();
    }
    const_iterator end() const {
      return map_->data_.begin() + bucket_end_idx();
    }
    friend const_iterator begin(const_bucket const& b) { return b.begin(); }
    friend const_iterator end(const_bucket const& b) { return b.end(); }

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
    const_bucket& operator++() {
      ++i_;
      return *this;
    }
    const_bucket& operator--() {
      --i_;
      return *this;
    }
    const_bucket operator*() const { return *this; }
    const_bucket& operator+=(difference_type const n) {
      i_ += n;
      return *this;
    }
    const_bucket& operator-=(difference_type const n) {
      i_ -= n;
      return *this;
    }
    const_bucket operator+(difference_type const n) const {
      auto tmp = *this;
      tmp += n;
      return tmp;
    }
    const_bucket operator-(difference_type const n) const {
      auto tmp = *this;
      tmp -= n;
      return tmp;
    }
    friend difference_type operator-(const_bucket const& a,
                                     const_bucket const& b) {
      assert(a.map_ == b.map_);
      return a.i_ - b.i_;
    }

  private:
    std::size_t bucket_begin_idx() const {
      return to_idx(map_->bucket_starts_[i_]);
    }
    std::size_t bucket_end_idx() const {
      return to_idx(map_->bucket_starts_[i_ + 1]);
    }
    bool is_inside_bucket(std::size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    std::size_t i_;
    basic_vecvec const* map_;
  };

  using value_type = bucket;
  using iterator = bucket;
  using const_iterator = const_bucket;

  bucket operator[](Key const i) { return {this, to_idx(i)}; }
  const_bucket operator[](Key const i) const { return {this, to_idx(i)}; }

  const_bucket at(Key const i) const {
    verify(to_idx(i) < bucket_starts_.size(),
           "basic_vecvec::at: index out of range");
    return {this, to_idx(i)};
  }

  bucket at(Key const i) {
    verify(to_idx(i) < bucket_starts_.size(),
           "basic_vecvec::at: index out of range");
    return {this, to_idx(i)};
  }

  bucket front() { return at(Key{0}); }
  bucket back() { return at(Key{size() - 1}); }

  const_bucket front() const { return at(Key{0}); }
  const_bucket back() const { return at(Key{size() - 1}); }

  index_value_type size() const {
    return empty() ? 0U : bucket_starts_.size() - 1;
  }
  bool empty() const { return bucket_starts_.empty(); }

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
    bucket_starts_.emplace_back(
        static_cast<index_value_type>(data_.size() + bucket.size()));
    data_.insert(std::end(data_),  //
                 std::make_move_iterator(std::begin(bucket)),
                 std::make_move_iterator(std::end(bucket)));
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

  bucket begin() { return bucket{this, 0U}; }
  bucket end() { return bucket{this, size()}; }
  const_bucket begin() const { return const_bucket{this, 0U}; }
  const_bucket end() const { return const_bucket{this, size()}; }

  friend bucket begin(basic_vecvec& m) { return m.begin(); }
  friend bucket end(basic_vecvec& m) { return m.end(); }
  friend const_bucket begin(basic_vecvec const& m) { return m.begin(); }
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
