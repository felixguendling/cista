#pragma once

#include "cista/containers/vector.h"

namespace cista {

template <typename Vector>
struct base_matrix {
  using value_type = typename Vector::value_type;
  using size_type = typename Vector::size_type;

  struct row {
    row(base_matrix& matrix, size_type const i) : matrix_(matrix), i_(i) {}

    value_type& operator[](size_type const j) {
      assert(j < matrix_.n_columns_);
      auto const pos = matrix_.n_columns_ * i_ + j;
      return matrix_.entries_[pos];
    }

    base_matrix& matrix_;
    size_type i_;
  };

  struct const_row {
    const_row(base_matrix const& matrix, size_type const i)
        : matrix_(matrix), i_(i) {}

    value_type const& operator[](size_type const j) const {
      assert(j < matrix_.n_columns_);
      auto const pos = matrix_.n_columns_ * i_ + j;
      return matrix_.entries_[pos];
    }

    base_matrix const& matrix_;
    size_type i_;
  };

  row operator[](size_type i) {
    assert(i < n_rows_);
    return {*this, i};
  }
  const_row operator[](size_type i) const {
    assert(i < n_rows_);
    return {*this, i};
  }

  value_type& operator()(size_type const i, size_type const j) {
    assert(i < n_rows_ && j < n_columns_);
    return entries_[n_columns_ * i + j];
  }

  row at(size_type const i) {
    verify(i < n_rows_, "matrix::at: index out of range");
    return {*this, i};
  }

  const_row at(size_type const i) const {
    verify(i < n_rows_, "matrix::at: index out of range");
    return {*this, i};
  }

  void resize(size_type const n_rows, size_t const n_columns) {
    n_rows_ = n_rows;
    n_columns_ = n_columns;
    entries_.resize(n_rows * n_columns);
  }

  void reset(value_type const& t) {
    std::fill(begin(entries_), end(entries_), t);
  }

  size_type n_rows_{0U}, n_columns_{0U};
  Vector entries_;
};

namespace offset {

template <typename T>
using matrix = base_matrix<vector<T>>;

template <typename T>
inline matrix<T> make_matrix(std::uint32_t const n_rows,
                             std::uint32_t const n_columns,
                             T const& init = T{}) {
  auto v = vector<T>{};
  v.resize(n_rows * n_columns, init);
  return {n_rows, n_columns, std::move(v)};
}

}  // namespace offset

namespace raw {

template <typename T>
using matrix = base_matrix<vector<T>>;

template <typename T>
inline matrix<T> make_matrix(std::uint32_t const n_rows,
                             std::uint32_t const n_columns,
                             T const& init = T{}) {
  auto v = vector<T>{};
  v.resize(n_rows * n_columns, init);
  return {n_rows, n_columns, std::move(v)};
}

}  // namespace raw

}  // namespace cista