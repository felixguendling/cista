#pragma once

#include "cista/containers/vector.h"

namespace cista {

template <typename VectorType>
struct base_flat_matrix {
  using value_type = typename VectorType::value_type;
  using size_type = typename VectorType::size_type;

  struct row {
    constexpr row(base_flat_matrix& matrix, int const row_index) noexcept
        : matrix_(matrix), row_index_(row_index) {}

    value_type& operator[](int column_index) {
      auto pos = matrix_.column_count_ * row_index_ + column_index;
      return matrix_.entries_[pos];
    }

    base_flat_matrix& matrix_;
    size_type row_index_;
  };

  struct const_row {
    constexpr const_row(base_flat_matrix const& matrix,
                        int const row_index) noexcept
        : matrix_(matrix), row_index_(row_index) {}

    value_type const& operator[](int const column_index) const {
      auto pos = matrix_.column_count_ * row_index_ + column_index;
      return matrix_.entries_[pos];
    }

    base_flat_matrix const& matrix_;
    int row_index_;
  };

  row operator[](int const row_index) noexcept { return {*this, row_index}; }
  const_row operator[](int const row_index) const noexcept {
    return {*this, row_index};
  }

  value_type& operator()(int const row_index, int const column_index) {
    return entries_[column_count_ * row_index + column_index];
  }

  size_type column_count_{0U};
  VectorType entries_{};
};

namespace detail {

template <typename V>
auto make_flat_matrix(std::uint32_t const column_count,
                      typename V::value_type const& init) {
  V v{};
  v.resize(column_count * column_count, init);
  return base_flat_matrix<V>{column_count, std::move(v)};
}

}  // namespace detail

namespace offset {

template <typename T>
using flat_matrix = base_flat_matrix<vector<T>>;

template <typename T>
flat_matrix<T> make_flat_matrix(std::uint32_t const column_count,
                                T const& init = T{}) {
  return detail::make_flat_matrix<vector<T>>(column_count, init);
}

}  // namespace offset

namespace raw {

template <typename T>
using flat_matrix = base_flat_matrix<vector<T>>;

template <typename T>
flat_matrix<T> make_flat_matrix(std::uint32_t const column_count,
                                T const& init = T{}) {
  return detail::make_flat_matrix<vector<T>>(column_count, init);
}

}  // namespace raw

}  // namespace cista
