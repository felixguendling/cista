#pragma once

#include "cista/strong.h"

namespace cista {

template <typename Index, typename Paged, typename Key>
struct paged_vecvec {
  using index_t = Index;
  using data_t = Paged;

  using page_t = typename Paged::page_t;
  using size_type = typename Paged::size_type;
  using data_value_type = typename Paged::value_type;

  struct const_bucket final {
    using size_type = typename Paged::size_type;
    using data_value_type = typename Paged::value_type;

    using value_type = data_value_type;
    using iterator = typename Paged::const_iterator;
    using const_iterator = typename Paged::const_iterator;
    using reference = typename Paged::const_reference;
    using const_reference = typename Paged::const_reference;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;

    const_bucket(paged_vecvec const* pv, Key const i) : pv_{pv}, i_{i} {}

    template <typename T = std::decay_t<data_value_type>,
              typename = std::enable_if_t<std::is_same_v<T, char>>>
    std::string_view view() const {
      return std::string_view{begin(), size()};
    }

    const_iterator begin() const { return pv_->data(i_); }
    const_iterator end() const { return pv_->data(i_) + size(); }
    friend const_iterator begin(const_bucket const& b) { return b.begin(); }
    friend const_iterator end(const_bucket const& b) { return b.end(); }

    value_type const& operator[](std::size_t const i) const {
      assert(i < size());
      return *(begin() + i);
    }

    value_type const& at(std::size_t const i) const {
      verify(i < size(), "paged_vecvec: const_bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type& at(std::size_t const i) {
      verify(i < size(), "paged_vecvec: const_bucket::at: index out of range");
      return *(begin() + i);
    }

    value_type const& front() const {
      assert(!empty());
      return (*this)[0];
    }

    value_type const& back() const {
      assert(!empty());
      return (*this)[size() - 1U];
    }

    reference operator*() const { return *this; }

    size_type size() const { return pv_->page(i_).size_; }
    bool empty() const { return size() == 0U; }

    friend bool operator==(const_bucket const& a, const_bucket const& b) {
      assert(a.pv_ == b.pv_);
      return a.i_ == b.i_;
    }

    friend bool operator!=(const_bucket const& a, const_bucket const& b) {
      assert(a.pv_ == b.pv_);
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
      assert(a.pv_ == b.pv_);
      return a.i_ - b.i_;
    }

  private:
    paged_vecvec const* pv_;
    Key i_;
  };

  struct bucket final {
    using size_type = typename Paged::size_type;
    using index_value_type = typename Paged::page_t;
    using data_value_type = typename Paged::value_type;

    using value_type = data_value_type;
    using iterator = typename Paged::iterator;
    using const_iterator = typename Paged::iterator;
    using reference = typename Paged::reference;
    using const_reference = typename Paged::const_reference;

    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<value_type>;

    bucket(paged_vecvec* pv, Key const i) : pv_{pv}, i_{i} {}

    value_type& front() {
      assert(!empty());
      return (*this)[0];
    }

    value_type& back() {
      assert(!empty());
      return (*this)[size() - 1U];
    }

    value_type const& front() const {
      assert(!empty());
      return (*this)[0];
    }

    value_type const& back() const {
      assert(!empty());
      return (*this)[size() - 1U];
    }

    void push_back(data_value_type const& x) {
      auto& p = pv_->page(i_);
      p = pv_->paged_.resize_page(p, p.size_ + 1U);
      (*this)[size() - 1U] = x;
    }

    template <typename T = std::decay_t<data_value_type>,
              typename = std::enable_if_t<std::is_same_v<T, char>>>
    std::string_view view() const {
      return std::string_view{begin(), static_cast<std::size_t>(size())};
    }

    iterator begin() { return pv_->data(i_); }
    iterator end() { return pv_->data(i_) + size(); }
    const_iterator begin() const { return pv_->data(i_); }
    const_iterator end() const { return pv_->data(i_) + size(); }
    friend iterator begin(bucket const& b) { return b.begin(); }
    friend iterator end(bucket const& b) { return b.end(); }
    friend iterator begin(bucket& b) { return b.begin(); }
    friend iterator end(bucket& b) { return b.end(); }

    value_type& operator[](std::size_t const i) {
      assert(i < size());
      return *(begin() + i);
    }

    value_type const& operator[](std::size_t const i) const {
      assert(i < size());
      return *(begin() + i);
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

    operator const_bucket() const { return {pv_, i_}; }

    size_type size() const { return pv_->page(i_).size_; }
    bool empty() const { return size() == 0U; }

    friend bool operator==(bucket const& a, bucket const& b) {
      assert(a.pv_ == b.pv_);
      return a.i_ == b.i_;
    }

    friend bool operator!=(bucket const& a, bucket const& b) {
      assert(a.pv_ == b.pv_);
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
      assert(a.pv_ == b.pv_);
      return a.i_ - b.i_;
    }

  private:
    page_t page() { return pv_->page(i_); }
    pointer data() { pv_->data(i_); }

    paged_vecvec* pv_;
    Key i_;
  };

  using value_type = bucket;
  using iterator = bucket;
  using const_iterator = bucket;

  bucket operator[](Key const i) { return {this, i}; }
  const_bucket operator[](Key const i) const { return {this, i}; }

  page_t& page(Key const i) { return idx_[to_idx(i)]; }
  page_t const& page(Key const i) const { return idx_[to_idx(i)]; }

  data_value_type const* data(Key const i) const {
    return data(idx_[to_idx(i)]);
  }
  data_value_type* data(Key const i) { return data(idx_[to_idx(i)]); }
  data_value_type const* data(page_t const& p) const { return paged_.data(p); }
  data_value_type* data(page_t const& p) { return paged_.data(p); }

  const_bucket at(Key const i) const {
    verify(to_idx(i) < idx_.size(), "paged_vecvec::at: index out of range");
    return operator[](i);
  }

  bucket at(Key const i) {
    verify(to_idx(i) < idx_.size(), "paged_vecvec::at: index out of range");
    return operator[](i);
  }

  bucket front() { return at(Key{0}); }
  bucket back() { return at(Key{size() - 1}); }

  const_bucket front() const { return at(Key{0}); }
  const_bucket back() const { return at(Key{size() - 1}); }

  size_type size() const { return idx_.size(); }
  bool empty() const { return idx_.empty(); }

  bucket begin() { return front(); }
  bucket end() { return operator[](Key{size()}); }
  const_bucket begin() const { return front(); }
  const_bucket end() const { return operator[](Key{size()}); }

  friend bucket begin(paged_vecvec& m) { return m.begin(); }
  friend bucket end(paged_vecvec& m) { return m.end(); }
  friend const_bucket begin(paged_vecvec const& m) { return m.begin(); }
  friend const_bucket end(paged_vecvec const& m) { return m.end(); }

  template <typename Container,
            typename = std::enable_if_t<std::is_convertible_v<
                decltype(*std::declval<Container>().begin()), data_value_type>>>
  void emplace_back(Container&& bucket) {
    auto p = paged_.create_page(
        static_cast<typename Paged::page_size_type>(bucket.size()));
    paged_.copy(p, std::begin(bucket), std::end(bucket));
    idx_.emplace_back(p);
  }

  template <typename X>
  std::enable_if_t<std::is_convertible_v<std::decay_t<X>, data_value_type>>
  emplace_back(std::initializer_list<X>&& x) {
    emplace_back(x);
  }

  void emplace_back_empty() { idx_.emplace_back(paged_.create_page(0U)); }

  template <typename T = data_value_type,
            typename = std::enable_if_t<std::is_convertible_v<T, char const>>>
  void emplace_back(char const* s) {
    return emplace_back(std::string_view{s});
  }

  void resize(size_type const size) {
    for (auto i = size; i < idx_.size(); ++i) {
      paged_.free_page(idx_[i]);
    }
    idx_.resize(size);
  }

  void clear() {
    paged_.clear();
    idx_.clear();
  }

  Paged paged_;
  Index idx_;
};

}  // namespace cista