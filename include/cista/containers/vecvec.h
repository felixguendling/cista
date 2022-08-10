#pragma once

#include <cassert>

#include "cista/containers/vector.h"

namespace cista {

template <typename DataVec, typename IndexVec>
struct vecvec {
  using data_value_type = typename DataVec::value_type;
  using index_value_type = typename IndexVec::value_type;

  struct bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::iterator;

    bucket(vecvec* map, index_value_type const i) : map_{map}, i_{to_idx(i)} {}

    value_type& operator[](size_t const i) {
      assert(is_inside_bucket(i));
      return map_->data_[map_->bucket_starts_[i_] + i];
    }

    value_type const& at(size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type& at(size_t const i) {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }
    iterator begin() { return map_->data_.begin() + bucket_begin_idx(); }
    iterator end() { return map_->data_.begin() + bucket_end_idx(); }
    friend iterator begin(bucket const& b) { return b.begin(); }
    friend iterator end(bucket const& b) { return b.end(); }

  private:
    size_t bucket_begin_idx() const { return to_idx(map_->bucket_starts_[i_]); }
    size_t bucket_end_idx() const {
      return to_idx(map_->bucket_starts_[i_ + 1]);
    }
    bool is_inside_bucket(size_t const i) {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    size_t const i_;
    vecvec* map_;
  };

  struct const_bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::const_iterator;

    const_bucket(vecvec const* map, index_value_type const i)
        : map_{map}, i_{to_idx(i)} {}

    value_type const& at(size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type& operator[](size_t const i) {
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
    bool is_inside_bucket(size_t const i) {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    size_t const i_;
    vecvec const* map_;
  };

  using value_type = bucket;
  using const_value_type = const_bucket;

  value_type operator[](index_value_type const i) { return {this, i}; }

  const_value_type at(index_value_type const i) const {
    utl::verify(to_idx(i) < bucket_starts_.size(),
                "vecvec::at: index out of range");
    return {this, i};
  }

  value_type at(index_value_type const i) {
    utl::verify(to_idx(i) < bucket_starts_.size(),
                "vecvec::at: index out of range");
    return {this, i};
  }

  size_t size() const { return empty() ? 0U : bucket_starts_.size() - 1; }
  bool empty() const { return data_.empty(); }

  void emplace_back(DataVec&& bucket) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(index_value_type{0});
    }
    bucket_starts_.emplace_back(data_.size() + bucket.size());
    data_.insert(end(data_),  //
                 std::make_move_iterator(begin(bucket)),
                 std::make_move_iterator(end(bucket)));
  }

  DataVec data_;
  IndexVec bucket_starts_;
};

namespace offset {

template <typename K, typename V>
using vecvec = vecvec<vector<V>, vector<K>>;

}  // namespace offset

namespace raw {

template <typename K, typename V>
using vecvec = vecvec<vector<V>, vector<K>>;

}  // namespace raw

}  // namespace cista
