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

    bucket(vecvec* map, size_t const i) : map_{map}, i_{i} {}

    data_value_type& operator[](size_t const i) {
      assert(is_inside_bucket(i));
      return map_->data_[map_->bucket_starts_[i_] + i];
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

  using value_type = bucket;

  value_type operator[](size_t const i) { return {this, i}; }

  size_t size() const { return empty() ? 0U : bucket_starts_.size() - 1; }
  bool empty() const { return data_.empty(); }

  void emplace_back(DataVec&& bucket) {
    if (bucket_starts_.empty()) {
      bucket_starts_.emplace_back(0);
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
