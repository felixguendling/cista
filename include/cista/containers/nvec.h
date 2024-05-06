#pragma once

#include <cinttypes>
#include <string_view>

#include "cista/containers/array.h"
#include "cista/containers/vector.h"
#include "cista/strong.h"
#include "cista/verify.h"

namespace cista {
template <typename DataVec, typename IndexVec, typename SizeType>
struct const_bucket final {
  using size_type = SizeType;
  using index_value_type = typename IndexVec::value_type;
  using data_value_type = typename DataVec::value_type;

  using value_type = data_value_type;
  using iterator = typename DataVec::const_iterator;
  using const_iterator = typename DataVec::const_iterator;
  using reference = typename DataVec::reference;
  using const_reference = typename DataVec::const_reference;

  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using pointer = std::add_pointer_t<value_type>;

  const_bucket(DataVec const* data, IndexVec const* index,
               index_value_type const i)
      : data_{data}, index_{index}, i_{to_idx(i)} {}

  friend data_value_type* data(const_bucket b) { return &b[0]; }
  friend index_value_type size(const_bucket b) { return b.size(); }

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
    return data()[index()[i_] + i];
  }

  const_bucket operator*() const { return *this; }

  std::size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }

  const_iterator begin() const { return data().begin() + bucket_begin_idx(); }
  const_iterator end() const { return data().begin() + bucket_end_idx(); }

  friend const_iterator begin(const_bucket const& b) { return b.begin(); }
  friend const_iterator end(const_bucket const& b) { return b.end(); }

  friend bool operator==(const_bucket const& a, const_bucket const& b) {
    assert(a.data_ == b.data_);
    assert(a.index_ == b.index_);
    return a.i_ == b.i_;
  }
  friend bool operator!=(const_bucket const& a, const_bucket const& b) {
    assert(a.data_ == b.data_);
    assert(a.index_ == b.index_);
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
    assert(a.data_ == b.data_);
    assert(a.index_ == b.index_);
    return a.i_ - b.i_;
  }

private:
  DataVec const& data() const { return *data_; }
  IndexVec const& index() const { return *index_; }

  std::size_t bucket_begin_idx() const { return to_idx(index()[i_]); }
  std::size_t bucket_end_idx() const { return to_idx(index()[i_ + 1U]); }
  bool is_inside_bucket(std::size_t const i) const {
    return bucket_begin_idx() + i < bucket_end_idx();
  }

  DataVec const* data_;
  IndexVec const* index_;
  size_type i_;
};

template <typename DataVec, typename IndexVec, typename SizeType>
struct bucket final {
  using size_type = SizeType;
  using index_value_type = typename IndexVec::value_type;
  using data_value_type = typename DataVec::value_type;

  using value_type = data_value_type;
  using iterator = typename DataVec::iterator;
  using const_iterator = typename DataVec::iterator;
  using reference = typename DataVec::reference;
  using const_reference = typename DataVec::const_reference;

  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using pointer = std::add_pointer_t<value_type>;

  bucket(DataVec* data, IndexVec* index, index_value_type const i)
      : data_{data}, index_{index}, i_{to_idx(i)} {}

  friend data_value_type* data(bucket b) { return &b[0]; }
  friend index_value_type size(bucket b) { return b.size(); }

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
    return data()[to_idx(index()[i_] + i)];
  }

  value_type const& operator[](std::size_t const i) const {
    assert(is_inside_bucket(i));
    return data()[to_idx(index()[i_] + i)];
  }

  value_type const& at(std::size_t const i) const {
    verify(i < size(), "bucket::at: index out of range");
    return *(begin() + i);
  }

  value_type& at(std::size_t const i) {
    verify(i < size(), "bucket::at: index out of range");
    return *(begin() + i);
  }

  reference operator*() const { return *this; }

  operator const_bucket<DataVec, IndexVec, SizeType>() const {
    return {data_, index_, i_};
  }

  index_value_type size() const {
    return bucket_end_idx() - bucket_begin_idx();
  }
  iterator begin() { return data().begin() + bucket_begin_idx(); }
  iterator end() { return data().begin() + bucket_end_idx(); }
  const_iterator begin() const { return data().begin() + bucket_begin_idx(); }
  const_iterator end() const { return data().begin() + bucket_end_idx(); }
  friend iterator begin(bucket const& b) { return b.begin(); }
  friend iterator end(bucket const& b) { return b.end(); }
  friend iterator begin(bucket& b) { return b.begin(); }
  friend iterator end(bucket& b) { return b.end(); }

  friend bool operator==(bucket const& a, bucket const& b) {
    assert(a.data_ == b.data_);
    assert(a.index_ == b.index_);
    return a.i_ == b.i_;
  }
  friend bool operator!=(bucket const& a, bucket const& b) {
    assert(a.data_ == b.data_);
    assert(a.index_ == b.index_);
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
    assert(a.data_ == b.data_);
    assert(a.index_ == b.index_);
    return a.i_ - b.i_;
  }

private:
  DataVec& data() const { return *data_; }
  IndexVec& index() const { return *index_; }

  index_value_type bucket_begin_idx() const { return to_idx(index()[i_]); }
  index_value_type bucket_end_idx() const { return to_idx(index()[i_ + 1U]); }
  bool is_inside_bucket(std::size_t const i) const {
    return bucket_begin_idx() + i < bucket_end_idx();
  }

  size_type i_;
  DataVec* data_;
  IndexVec* index_;
};

template <std::size_t Depth, typename DataVec, typename IndexVec,
          typename SizeType>
struct const_meta_bucket {
  using index_value_type = typename IndexVec::value_type;

  using const_iterator = std::conditional_t<
      Depth == 1U, const_bucket<DataVec, IndexVec, SizeType>,
      const_meta_bucket<Depth - 1U, DataVec, IndexVec, SizeType>>;
  using iterator = const_iterator;
  using difference_type = std::ptrdiff_t;
  using value_type = const_iterator;
  using pointer = void;
  using reference = const_meta_bucket;
  using const_reference = const_meta_bucket;
  using iterator_category = std::random_access_iterator_tag;
  using size_type = SizeType;

  const_meta_bucket(DataVec const* data, IndexVec const* index,
                    index_value_type const i)
      : data_{data}, index_{index}, i_{i} {}

  index_value_type size() const { return index()[i_ + 1U] - index()[i_]; }

  iterator begin() const { return {data_, index_ - 1U, index()[i_]}; }
  const_iterator end() const { return {data_, index_ - 1U, index()[i_ + 1U]}; }

  friend iterator begin(const_meta_bucket const& b) { return b.begin(); }
  friend iterator end(const_meta_bucket const& b) { return b.end(); }

  reference operator*() const { return *this; }

  iterator operator[](size_type const i) { return begin() + i; }

  const_meta_bucket& operator++() {
    ++i_;
    return *this;
  }
  const_meta_bucket& operator--() {
    --i_;
    return *this;
  }
  const_meta_bucket& operator+=(difference_type const n) {
    i_ += n;
    return *this;
  }
  const_meta_bucket& operator-=(difference_type const n) {
    i_ -= n;
    return *this;
  }
  const_meta_bucket operator+(difference_type const n) const {
    auto tmp = *this;
    tmp += n;
    return tmp;
  }
  const_meta_bucket operator-(difference_type const n) const {
    auto tmp = *this;
    tmp -= n;
    return tmp;
  }

  friend bool operator==(const_meta_bucket const& a,
                         const_meta_bucket const& b) {
    return a.i_ == b.i_;
  }

  friend bool operator!=(const_meta_bucket const& a,
                         const_meta_bucket const& b) {
    return !(a == b);
  }

private:
  IndexVec const& index() const { return *index_; }
  DataVec const& data() const { return *data_; }

  DataVec const* data_;
  IndexVec const* index_;
  index_value_type i_;
};

template <std::size_t Depth, typename DataVec, typename IndexVec,
          typename SizeType>
struct meta_bucket {
  using index_value_type = typename IndexVec::value_type;

  using iterator =
      std::conditional_t<Depth == 1U, bucket<DataVec, IndexVec, SizeType>,
                         meta_bucket<Depth - 1U, DataVec, IndexVec, SizeType>>;
  using const_iterator = std::conditional_t<
      Depth == 1U, const_bucket<DataVec, IndexVec, SizeType>,
      const_meta_bucket<Depth - 1U, DataVec, IndexVec, SizeType>>;

  using iterator_category = std::random_access_iterator_tag;
  using reference = meta_bucket;
  using const_reference = const_meta_bucket<Depth, DataVec, IndexVec, SizeType>;
  using difference_type = std::ptrdiff_t;
  using size_type = SizeType;

  meta_bucket(DataVec* data, IndexVec* index, index_value_type const i)
      : data_{data}, index_{index}, i_{i} {}

  index_value_type size() const { return index()[i_ + 1U] - index()[i_]; }

  iterator begin() { return {data_, index_ - 1U, index()[i_]}; }
  iterator end() { return {data_, index_ - 1U, index()[i_ + 1U]}; }

  const_iterator begin() const { return {data_, index_ - 1U, index()[i_]}; }
  const_iterator end() const { return {data_, index_ - 1U, index()[i_ + 1U]}; }

  friend iterator begin(meta_bucket& b) { return b.begin(); }
  friend iterator end(meta_bucket& b) { return b.end(); }

  friend const_iterator begin(meta_bucket const& b) { return b.begin(); }
  friend const_iterator end(meta_bucket const& b) { return b.end(); }

  const_reference operator*() const { return {data_, index_, i_}; }
  reference operator*() { return *this; }

  iterator operator[](size_type const i) { return begin() + i; }
  const_iterator operator[](size_type const i) const { return begin() + i; }

  operator const_meta_bucket<Depth, DataVec, IndexVec, SizeType>() const {
    return {data_, index_, i_};
  }

  meta_bucket& operator++() {
    ++i_;
    return *this;
  }

  meta_bucket& operator--() {
    --i_;
    return *this;
  }
  meta_bucket& operator+=(difference_type const n) {
    i_ += n;
    return *this;
  }
  meta_bucket& operator-=(difference_type const n) {
    i_ -= n;
    return *this;
  }
  meta_bucket operator+(difference_type const n) const {
    auto tmp = *this;
    tmp += n;
    return tmp;
  }
  meta_bucket operator-(difference_type const n) const {
    auto tmp = *this;
    tmp -= n;
    return tmp;
  }

  friend bool operator==(meta_bucket const& a, meta_bucket const& b) {
    return a.i_ == b.i_;
  }

  friend bool operator!=(meta_bucket const& a, meta_bucket const& b) {
    return !(a == b);
  }

private:
  IndexVec& index() const { return *index_; }
  DataVec& data() const { return *data_; }

  DataVec* data_;
  IndexVec* index_;
  index_value_type i_;
};

template <typename Key, typename DataVec, typename IndexVec, std::size_t N,
          typename SizeType = std::uint32_t>
struct basic_nvec {
  using data_vec_t = DataVec;
  using index_vec_t = IndexVec;
  using size_type = SizeType;
  using data_value_type = typename DataVec::value_type;
  using index_value_type = typename IndexVec::value_type;

  using bucket_t = bucket<DataVec, IndexVec, SizeType>;
  using const_bucket_t = const_bucket<DataVec, IndexVec, SizeType>;

  using iterator_category = std::random_access_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using iterator = meta_bucket<N - 1U, DataVec, IndexVec, SizeType>;
  using const_iterator = const_meta_bucket<N - 1U, DataVec, IndexVec, SizeType>;
  using reference = iterator;
  using const_reference = const_iterator;
  using value_type = iterator;

  iterator begin() { return {&data_, &index_.back(), 0U}; }
  iterator end() { return {&data_, &index_.back(), size()}; }

  const_iterator begin() const { return {&data_, &index_.back(), 0U}; }
  const_iterator end() const { return {&data_, &index_.back(), size()}; }

  iterator front() { return begin(); }
  iterator back() { return begin() + size() - 1; }

  const_iterator front() const { return begin(); }
  const_iterator back() const { return begin() + size() - 1; }

  template <typename Container>
  void emplace_back(Container&& bucket) {
    if (index_[0].size() == 0U) {
      for (auto& i : index_) {
        i.push_back(0U);
      }
    }
    add<N - 1>(bucket);
  }

  iterator operator[](Key const k) { return begin() + to_idx(k); }
  const_iterator operator[](Key const k) const { return begin() + to_idx(k); }

  size_type size() const {
    return index_[N - 1].size() == 0U ? 0U : (index_[N - 1].size() - 1U);
  }

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
  bucket_t at(Key const first, Indices... rest) {
    constexpr auto const I = sizeof...(Indices);
    static_assert(I == N - 1);
    verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
    return get_bucket(index_[I][to_idx(first)], rest...);
  }

  template <typename... Indices>
  const_bucket_t at(Key const first, Indices... rest) const {
    constexpr auto const I = sizeof...(Indices);
    static_assert(I == N - 1);
    verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
    return get_bucket(index_[I][to_idx(first)], rest...);
  }

  auto cista_members() noexcept { return std::tie(index_, data_); }

  template <typename... Rest>
  bucket_t get_bucket(index_value_type const bucket_start,
                      index_value_type const i, Rest... rest) {
    return get_bucket<Rest...>(index_[sizeof...(Rest)][bucket_start + i],
                               rest...);
  }

  bucket_t get_bucket(index_value_type const bucket_start,
                      index_value_type const i) {
    return {&data_, &index_[0], bucket_start + i};
  }

  template <typename... Rest>
  const_bucket_t get_bucket(index_value_type const bucket_start,
                            index_value_type const i, Rest... rest) const {
    return get_bucket<Rest...>(index_[sizeof...(Rest)][bucket_start + i],
                               rest...);
  }

  const_bucket_t get_bucket(index_value_type const bucket_start,
                            index_value_type const i) const {
    return {&data_, &index_[0], bucket_start + i};
  }

  template <std::size_t L, typename Container>
  void add(Container&& c) {
    if constexpr (L == 0) {
      index_[0].push_back(static_cast<size_type>(data_.size() + c.size()));
      data_.insert(std::end(data_), std::make_move_iterator(std::begin(c)),
                   std::make_move_iterator(std::end(c)));
    } else {
      index_[L].push_back(
          static_cast<size_type>(index_[L - 1].size() + c.size() - 1U));
      for (auto&& x : c) {
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

template <typename K, typename V, std::size_t N,
          typename SizeType = std::uint32_t>
using nvec = basic_nvec<K, vector<V>, vector<base_t<K>>, N, SizeType>;

}  // namespace offset

namespace raw {

template <typename K, typename V, std::size_t N,
          typename SizeType = std::uint32_t>
using nvec = basic_nvec<K, vector<V>, vector<base_t<K>>, N, SizeType>;

}  // namespace raw

}  // namespace cista