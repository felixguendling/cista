#pragma once

#include <cassert>
#include <type_traits>

#include "cista/containers/vector.h"
#include "cista/verify.h"

namespace cista {

template <typename Key, typename DataVec, typename IndexVec>
struct vecvec {
  static_assert(std::is_same_v<typename IndexVec::value_type, base_t<Key>>);

  using data_value_type = typename DataVec::value_type;
  using index_value_type = typename IndexVec::value_type;

  struct bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::iterator;

    bucket(vecvec* map, Key const i) : map_{map}, i_{to_idx(i)} {}

    std::string_view str() const {
      static_assert(std::is_same_v<std::decay_t<value_type>, char>);
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

    bool empty() const { return begin() == end(); }

    value_type& operator[](size_t const i) {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
    }

    value_type const& at(size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type& at(size_t const i) {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    index_value_type size() const {
      return bucket_end_idx() - bucket_begin_idx();
    }
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

  private:
    index_value_type bucket_begin_idx() const {
      return to_idx(map_->bucket_starts_[i_]);
    }
    index_value_type bucket_end_idx() const {
      return to_idx(map_->bucket_starts_[i_ + 1]);
    }
    bool is_inside_bucket(index_value_type const i) {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    vecvec* map_;
    index_value_type const i_;
  };

  struct const_bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::const_iterator;

    const_bucket(vecvec const* map, Key const i) : map_{map}, i_{to_idx(i)} {}

    template <typename T = std::decay_t<data_value_type>,
              std::enable_if_t<std::is_same_v<T, char>>>
    std::string_view str() const {
      return std::string_view{begin(), end()};
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

    value_type const& at(size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type const& operator[](size_t const i) const {
      assert(is_inside_bucket(i));
      return map_->data_[map_->bucket_starts_[i_] + i];
    }

    size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }
    const_iterator begin() const {
      return map_->data_.begin() + bucket_begin_idx();
    }
    const_iterator end() const {
      return map_->data_.begin() + bucket_end_idx();
    }
    friend const_iterator begin(const_bucket const& b) { return b.begin(); }
    friend const_iterator end(const_bucket const& b) { return b.end(); }

  private:
    size_t bucket_begin_idx() const { return to_idx(map_->bucket_starts_[i_]); }
    size_t bucket_end_idx() const {
      return to_idx(map_->bucket_starts_[i_ + 1]);
    }
    bool is_inside_bucket(size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    size_t const i_;
    vecvec const* map_;
  };

  using value_type = bucket;
  using const_value_type = const_bucket;

  value_type operator[](Key const i) { return {this, i}; }
  const_value_type operator[](Key const i) const { return {this, i}; }

  const_value_type at(Key const i) const {
    verify(to_idx(i) < bucket_starts_.size(), "vecvec::at: index out of range");
    return {this, i};
  }

  value_type at(Key const i) {
    verify(to_idx(i) < bucket_starts_.size(), "vecvec::at: index out of range");
    return {this, i};
  }

  index_value_type size() const {
    return empty() ? 0U : bucket_starts_.size() - 1;
  }
  bool empty() const { return data_.empty(); }

  template <typename Container,
            typename = std::enable_if_t<std::is_convertible_v<
                typename Container::value_type, data_value_type>>>
  void emplace_back(Container&& bucket) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(index_value_type{0U});
    }
    bucket_starts_.emplace_back(data_.size() + bucket.size());
    data_.insert(end(data_),  //
                 std::make_move_iterator(begin(bucket)),
                 std::make_move_iterator(end(bucket)));
  }

  template <typename String,
            typename = std::enable_if_t<
                std::is_convertible_v<data_value_type, char const> &&
                std::is_convertible_v<decltype(String{}.data()), char const*> &&
                std::is_convertible_v<decltype(String{}.size()), size_t>>>
  void emplace_back(String const& s) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(index_value_type{0U});
    }
    bucket_starts_.emplace_back(data_.size() + s.size());
    data_.insert(end(data_), s.data(), s.data() + s.size());
  }

  template <typename T = data_value_type,
            typename = std::enable_if_t<std::is_convertible_v<T, char const>>>
  void emplace_back(char const* s) {
    return emplace_back(std::string_view{s});
  }

  DataVec data_;
  IndexVec bucket_starts_;
};

namespace offset {

template <typename K, typename V>
using vecvec = vecvec<K, vector<V>, vector<base_t<K>>>;

}  // namespace offset

namespace raw {

template <typename K, typename V>
using vecvec = vecvec<K, vector<V>, vector<base_t<K>>>;

}  // namespace raw

}  // namespace cista
