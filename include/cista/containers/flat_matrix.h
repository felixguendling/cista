#pragma once

#include <cinttypes>

#include "cista/containers/vector.h"

namespace cista {

template <typename Vector>
struct base_flat_matrix {
  using value_type = typename Vector::value_type;
  using size_type = typename Vector::size_type;

  struct row {
    row(base_flat_matrix& matrix, size_type const i) : matrix_(matrix), i_(i) {}

    using iterator = typename Vector::iterator;
    using const_iterator = typename Vector::const_iterator;

    const_iterator begin() const {
      return std::next(matrix_.entries_, matrix_.n_columns_ * i_);
    }
    const_iterator end() const {
      return std::next(matrix_.entries_, matrix_.n_columns_ * (i_ + 1));
    }
    iterator begin() {
      return std::next(matrix_.entries_, matrix_.n_columns_ * i_);
    }
    iterator end() {
      return std::next(matrix_.entries_, matrix_.n_columns_ * (i_ + 1));
    }
    friend const_iterator begin(row const& r) { return r.begin(); }
    friend const_iterator end(row const& r) { return r.end(); }
    friend iterator begin(row& r) { return r.begin(); }
    friend iterator end(row& r) { return r.end(); }

    value_type& operator[](size_type const j) {
      assert(j < matrix_.n_columns_);
      auto const pos = matrix_.n_columns_ * i_ + j;
      return matrix_.entries_[pos];
    }

    base_flat_matrix& matrix_;
    size_type i_;
  };

  struct const_row {
    const_row(base_flat_matrix const& matrix, size_type const i)
        : matrix_(matrix), i_(i) {}

    using iterator = typename Vector::const_iterator;

    iterator begin() const {
      return std::next(matrix_.entries_, matrix_.n_columns_ * i_);
    }
    iterator end() const {
      return std::next(matrix_.entries_, matrix_.n_columns_ * (i_ + 1));
    }
    friend iterator begin(const_row const& r) { return r.begin(); }
    friend iterator end(const_row const& r) { return r.end(); }

    value_type const& operator[](size_type const j) const {
      assert(j < matrix_.n_columns_);
      auto const pos = matrix_.n_columns_ * i_ + j;
      return matrix_.entries_[pos];
    }

    base_flat_matrix const& matrix_;
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
using flat_matrix = base_flat_matrix<vector<T>>;

template <typename T>
inline flat_matrix<T> make_flat_matrix(std::uint32_t const n_rows,
                                       std::uint32_t const n_columns,
                                       T const& init = T{}) {
  auto v = vector<T>{};
  v.resize(n_rows * n_columns, init);
  return {n_rows, n_columns, std::move(v)};
}

}  // namespace offset

namespace raw {

template <typename T>
using flat_matrix = base_flat_matrix<vector<T>>;

template <typename T>
inline flat_matrix<T> make_flat_matrix(std::uint32_t const n_rows,
                                       std::uint32_t const n_columns,
                                       T const& init = T{}) {
  auto v = vector<T>{};
  v.resize(n_rows * n_columns, init);
  return {n_rows, n_columns, std::move(v)};
}

}  // namespace raw

}  // namespace cista
