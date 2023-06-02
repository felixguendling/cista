#pragma once

#include <cinttypes>
#include <string_view>

#include "cista/containers/array.h"
#include "cista/containers/vector.h"
#include "cista/strong.h"
#include "cista/verify.h"

namespace cista {

template <typename Key, typename DataVec, typename IndexVec, std::size_t N,
          typename SizeType = std::uint32_t>
struct basic_nvec {
  static_assert(std::is_same_v<typename IndexVec::value_type, base_t<Key>>);

  using size_type = SizeType;
  using data_value_type = typename DataVec::value_type;
  using index_value_type = typename IndexVec::value_type;

  struct bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::iterator;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference<value_type>;

    bucket(basic_nvec* map, index_value_type const i)
        : map_{map}, i_{to_idx(i)} {}

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

    bool empty() const { return begin() == end(); }

    value_type& operator[](std::size_t const i) {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->index_[0][i_] + i)];
    }

    value_type const& operator[](std::size_t const i) const {
      assert(is_inside_bucket(i));
      return map_->data_[to_idx(map_->index_[0][i_] + i)];
    }

    value_type const& at(std::size_t const i) const {
      verify(i < size(), "bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type& at(std::size_t const i) {
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
    bucket operator*() { return *this; }
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
      return to_idx(map_->index_[0][i_]);
    }
    index_value_type bucket_end_idx() const {
      return to_idx(map_->index_[0][i_ + 1U]);
    }
    bool is_inside_bucket(std::size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    basic_nvec* map_;
    index_value_type i_;
  };

  struct const_bucket final {
    using value_type = data_value_type;
    using iterator = typename DataVec::iterator;
    using const_iterator = typename DataVec::const_iterator;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference<value_type>;

    const_bucket(basic_nvec const* map, index_value_type const i)
        : i_{to_idx(i)}, map_{map} {}

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
      return map_->data_[map_->index_[0][i_] + i];
    }

    std::size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }
    const_iterator begin() const {
      return map_->data_.begin() + bucket_begin_idx();
    }
    const_iterator end() const {
      return map_->data_.begin() + bucket_end_idx();
    }
    friend const_iterator begin(const_bucket const& b) { return b.begin(); }
    friend const_iterator end(const_bucket const& b) { return b.end(); }

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
    const_bucket operator*() { return *this; }
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
    std::size_t bucket_begin_idx() const { return to_idx(map_->index_[0][i_]); }
    std::size_t bucket_end_idx() const {
      return to_idx(map_->index_[0][i_ + 1U]);
    }
    bool is_inside_bucket(std::size_t const i) const {
      return bucket_begin_idx() + i < bucket_end_idx();
    }

    size_type i_;
    basic_nvec const* map_;
  };

  basic_nvec() {
    for (auto& i : index_) {
      i.push_back(0U);
    }
  }

  template <typename Container>
  void emplace_back(Container&& bucket) {
    if (index_[0].size() == 0U) {
      for (auto& i : index_) {
        i.push_back(0U);
      }
    }
    add<N - 1>(bucket);
  }

  size_type size() const { return index_[N - 1].size() - 1U; }

  template <typename... Indices>
  size_type size(Key const first, Indices... rest) const {
    constexpr auto const I = sizeof...(Indices);
    verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
    if (sizeof...(Indices) == 0U) {
      return get_size<N - sizeof...(Indices) - 1>(first);
    } else {
      return get_size<N - sizeof...(Indices) - 1>(index_[I][first], rest...);
    }
  }

  template <typename... Indices>
  bucket at(Key const first, Indices... rest) {
    constexpr auto const I = sizeof...(Indices);
    static_assert(I == N - 1);
    verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
    return get_bucket(index_[I][first], rest...);
  }

  template <typename... Indices>
  const_bucket at(Key const first, Indices... rest) const {
    constexpr auto const I = sizeof...(Indices);
    static_assert(I == N - 1);
    verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
    return get_bucket(index_[I][first], rest...);
  }

  auto cista_members() noexcept { return std::tie(index_, data_); }

  template <typename... Rest>
  bucket get_bucket(index_value_type const bucket_start,
                    index_value_type const i, Rest... rest) {
    return get_bucket<Rest...>(index_[sizeof...(Rest)][bucket_start + i],
                               rest...);
  }

  bucket get_bucket(index_value_type const bucket_start,
                    index_value_type const i) {
    return bucket{this, bucket_start + i};
  }

  template <typename... Rest>
  const_bucket get_bucket(index_value_type const bucket_start,
                          index_value_type const i, Rest... rest) const {
    return get_bucket<Rest...>(index_[sizeof...(Rest)][bucket_start + i],
                               rest...);
  }

  const_bucket get_bucket(index_value_type const bucket_start,
                          index_value_type const i) const {
    return const_bucket{this, bucket_start + i};
  }

  template <std::size_t L, typename Container>
  void add(Container&& c) {
    if constexpr (L == 0) {
      index_[0].push_back(static_cast<size_type>(data_.size() + c.size()));
      data_.insert(end(data_), std::make_move_iterator(begin(c)),
                   std::make_move_iterator(end(c)));
    } else {
      index_[L].push_back(
          static_cast<size_type>(index_[L - 1].size() + c.size() - 1U));
      for (auto& x : c) {
        add<L - 1>(x);
      }
    }
  }

  template <std::size_t L, typename... Rest>
  size_type get_size(index_value_type const i, index_value_type const j,
                     Rest... rest) const {
    if constexpr (sizeof...(Rest) == 0U) {
      return index_[L][i + j + 1] - index_[L][i + j];
    } else {
      return get_size<L>(index_[L][i + j], rest...);
    }
  }

  template <std::size_t L>
  size_type get_size(index_value_type const i) const {
    return index_[L][i + 1] - index_[L][i];
  }

  array<IndexVec, N> index_;
  DataVec data_;
};

namespace offset {

template <typename K, typename V, std::size_t N>
using nvec = basic_nvec<K, vector<V>, vector<base_t<K>>, N>;

}  // namespace offset

namespace raw {

template <typename K, typename V, std::size_t N>
using nvec = basic_nvec<K, vector<V>, vector<base_t<K>>, N>;

}  // namespace raw

}  // namespace cista