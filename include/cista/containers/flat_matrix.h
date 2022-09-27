#pragma once

#include "cista/containers/vector.h"

namespace cista {

template <typename VectorType>
struct base_flat_matrix {
  using value_type = typename VectorType::value_type;
  using size_type = typename VectorType::size_type;

  struct row {
    row(base_flat_matrix& matrix, int row_index)
        : matrix_(matrix), row_index_(row_index) {}

    value_type& operator[](int column_index) {
      auto pos = matrix_.column_count_ * row_index_ + column_index;
      return matrix_.entries_[pos];
    }

    base_flat_matrix& matrix_;
    size_type row_index_;
  };

  struct const_row {
    const_row(base_flat_matrix const& matrix, int row_index)
        : matrix_(matrix), row_index_(row_index) {}

    value_type const& operator[](int column_index) const {
      auto pos = matrix_.column_count_ * row_index_ + column_index;
      return matrix_.entries_[pos];
    }

    base_flat_matrix const& matrix_;
    int row_index_;
  };

  row operator[](int row_index) { return {*this, row_index}; }
  const_row operator[](int row_index) const { return {*this, row_index}; }

  value_type& operator()(int const row_index, int const column_index) {
    return entries_[column_count_ * row_index + column_index];
  }

  size_type column_count_{0U};
  VectorType entries_;
};

namespace offset {

template <typename T>
using flat_matrix = base_flat_matrix<vector<T>>;

template <typename T>
inline flat_matrix<T> make_flat_matrix(std::uint32_t const column_count,
                                       T const& init = T{}) {
  auto v = vector<T>{};
  v.resize(column_count * column_count, init);
  return flat_matrix<T>{column_count, std::move(v)};
}

}  // namespace offset

namespace raw {

template <typename T>
using flat_matrix = base_flat_matrix<vector<T>>;

template <typename T>
inline flat_matrix<T> make_flat_matrix(std::uint32_t const column_count,
                                       T const& init = T{}) {
  auto v = vector<T>{};
  v.resize(column_count * column_count, init);
  return flat_matrix<T>{column_count, std::move(v)};
}

}  // namespace raw

}  // namespace cista
