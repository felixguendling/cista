#pragma once

#include <cassert>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#include <ostream>
#include <type_traits>
#include <vector>

#include "cista/allocator.h"
#include "cista/containers/ptr.h"
#include "cista/exception.h"
#include "cista/is_iterable.h"
#include "cista/next_power_of_2.h"
#include "cista/strong.h"
#include "cista/unused_param.h"
#include "cista/verify.h"

namespace cista {

template <typename T, template <typename> typename Ptr,
          bool IndexPointers = false, typename TemplateSizeType = std::uint32_t,
          class Allocator = allocator<T, Ptr>>
struct basic_vector {
  using size_type = base_t<TemplateSizeType>;
  using difference_type = std::ptrdiff_t;
  using access_type = TemplateSizeType;
  using reference = T&;
  using const_reference = T const&;
  using pointer = Ptr<T>;
  using const_pointer = Ptr<T const>;
  using value_type = T;
  using iterator = T*;
  using const_iterator = T const*;
  using allocator_type = Allocator;

  explicit basic_vector(allocator_type const&) noexcept {}
  basic_vector() noexcept = default;

  explicit basic_vector(size_type const size, T init = T{},
                        Allocator const& alloc = Allocator{}) {
    CISTA_UNUSED_PARAM(alloc)
    resize(size, std::move(init));
  }

  basic_vector(std::initializer_list<T> init,
               Allocator const& alloc = Allocator{}) {
    CISTA_UNUSED_PARAM(alloc)
    set(init.begin(), init.end());
  }

  template <typename It>
  basic_vector(It begin_it, It end_it) {
    set(begin_it, end_it);
  }

  basic_vector(basic_vector&& o, Allocator const& alloc = Allocator{}) noexcept
      : el_(o.el_),
        used_size_(o.used_size_),
        allocated_size_(o.allocated_size_),
        self_allocated_(o.self_allocated_) {
    CISTA_UNUSED_PARAM(alloc)
    o.reset();
  }

  basic_vector(basic_vector const& o, Allocator const& alloc = Allocator{}) {
    CISTA_UNUSED_PARAM(alloc)
    set(o);
  }

  basic_vector& operator=(basic_vector&& arr) noexcept {
    deallocate();

    el_ = arr.el_;
    used_size_ = arr.used_size_;
    self_allocated_ = arr.self_allocated_;
    allocated_size_ = arr.allocated_size_;

    arr.reset();
    return *this;
  }

  basic_vector& operator=(basic_vector const& arr) {
    if (&arr != this) {
      set(arr);
    }
    return *this;
  }

  ~basic_vector() { deallocate(); }

  void deallocate() {
    if (!self_allocated_ || el_ == nullptr) {
      return;
    }

    for (auto& el : *this) {
      el.~T();
    }

    std::free(el_);  // NOLINT
    reset();
  }

  allocator_type get_allocator() const noexcept { return {}; }

  T const* data() const noexcept { return begin(); }
  T* data() noexcept { return begin(); }
  T const* begin() const noexcept { return el_; }
  T const* end() const noexcept { return el_ + used_size_; }  // NOLINT
  T const* cbegin() const noexcept { return el_; }
  T const* cend() const noexcept { return el_ + used_size_; }  // NOLINT
  T* begin() noexcept { return el_; }
  T* end() noexcept { return el_ + used_size_; }  // NOLINT

  std::reverse_iterator<T const*> rbegin() const {
    return std::reverse_iterator<T*>(el_ + size());  // NOLINT
  }
  std::reverse_iterator<T const*> rend() const {
    return std::reverse_iterator<T*>(el_);
  }
  std::reverse_iterator<T*> rbegin() {
    return std::reverse_iterator<T*>(el_ + size());  // NOLINT
  }
  std::reverse_iterator<T*> rend() { return std::reverse_iterator<T*>(el_); }

  friend T const* begin(basic_vector const& a) noexcept { return a.begin(); }
  friend T const* end(basic_vector const& a) noexcept { return a.end(); }

  friend T* begin(basic_vector& a) noexcept { return a.begin(); }
  friend T* end(basic_vector& a) noexcept { return a.end(); }

  T const& operator[](access_type const index) const noexcept {
    assert(el_ != nullptr && index < used_size_);
    return el_[to_idx(index)];
  }
  T& operator[](access_type const index) noexcept {
    assert(el_ != nullptr && index < used_size_);
    return el_[to_idx(index)];
  }

  T& at(access_type const index) {
    if (index >= used_size_) {
      throw_exception(std::out_of_range{"vector::at(): invalid index"});
    }
    return (*this)[index];
  }

  T const& at(access_type const index) const {
    return const_cast<basic_vector*>(this)->at(index);
  }

  T const& back() const noexcept { return ptr_cast(el_)[used_size_ - 1]; }
  T& back() noexcept { return ptr_cast(el_)[used_size_ - 1]; }

  T& front() noexcept { return ptr_cast(el_)[0]; }
  T const& front() const noexcept { return ptr_cast(el_)[0]; }

  size_type size() const noexcept { return used_size_; }
  bool empty() const noexcept { return size() == 0U; }

  template <typename It>
  void set(It begin_it, It end_it) {
    auto const range_size = std::distance(begin_it, end_it);
    verify(
        range_size >= 0 && range_size <= std::numeric_limits<size_type>::max(),
        "cista::vector::set: invalid range");

    reserve(static_cast<size_type>(range_size));

    auto copy_source = begin_it;
    auto copy_target = el_;
    for (; copy_source != end_it; ++copy_source, ++copy_target) {
      new (copy_target) T{std::forward<decltype(*copy_source)>(*copy_source)};
    }

    used_size_ = static_cast<size_type>(range_size);
  }

  void set(basic_vector const& arr) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      if (arr.used_size_ != 0U) {
        reserve(arr.used_size_);
        std::memcpy(data(), arr.data(), arr.used_size_ * sizeof(T));
      }
      used_size_ = arr.used_size_;
    } else {
      set(std::begin(arr), std::end(arr));
    }
  }

  friend std::ostream& operator<<(std::ostream& out, basic_vector const& v) {
    out << "[\n  ";
    auto first = true;
    for (auto const& e : v) {
      if (!first) {
        out << ",\n  ";
      }
      out << e;
      first = false;
    }
    return out << "\n]";
  }

  template <typename Arg>
  T* insert(T* it, Arg&& el) {
    auto const old_offset = std::distance(begin(), it);
    auto const old_size = used_size_;

    reserve(used_size_ + 1);
    new (el_ + used_size_) T{std::forward<Arg&&>(el)};
    ++used_size_;

    return std::rotate(begin() + old_offset, begin() + old_size, end());
  }

  template <class InputIt>
  T* insert(T* pos, InputIt first, InputIt last, std::input_iterator_tag) {
    auto const old_offset = std::distance(begin(), pos);
    auto const old_size = used_size_;

    for (; !(first == last); ++first) {
      reserve(used_size_ + 1);
      new (el_ + used_size_) T{std::forward<decltype(*first)>(*first)};
      ++used_size_;
    }

    return std::rotate(begin() + old_offset, begin() + old_size, end());
  }

  template <class FwdIt>
  T* insert(T* pos, FwdIt first, FwdIt last, std::forward_iterator_tag) {
    if (empty()) {
      set(first, last);
      return begin();
    }

    auto const pos_idx = pos - begin();
    auto const new_count = static_cast<size_type>(std::distance(first, last));
    reserve(used_size_ + new_count);
    pos = begin() + pos_idx;

    for (auto src_last = end() - 1, dest_last = end() + new_count - 1;
         !(src_last == pos - 1); --src_last, --dest_last) {
      if (dest_last >= end()) {
        new (dest_last) T(std::move(*src_last));
      } else {
        *dest_last = std::move(*src_last);
      }
    }

    for (auto insert_ptr = pos; !(first == last); ++first, ++insert_ptr) {
      if (insert_ptr >= end()) {
        new (insert_ptr) T(std::forward<decltype(*first)>(*first));
      } else {
        *insert_ptr = std::forward<decltype(*first)>(*first);
      }
    }

    used_size_ += new_count;

    return pos;
  }

  template <class It>
  T* insert(T* pos, It first, It last) {
    return insert(pos, first, last,
                  typename std::iterator_traits<It>::iterator_category());
  }

  void push_back(T const& el) {
    reserve(used_size_ + 1U);
    new (el_ + used_size_) T(el);
    ++used_size_;
  }

  template <typename... Args>
  T& emplace_back(Args&&... el) {
    reserve(used_size_ + 1U);
    new (el_ + used_size_) T{std::forward<Args>(el)...};
    T* ptr = el_ + used_size_;
    ++used_size_;
    return *ptr;
  }

  void resize(size_type const size, T init = T{}) {
    reserve(size);
    for (auto i = used_size_; i < size; ++i) {
      new (el_ + i) T{init};
    }
    used_size_ = size;
  }

  void pop_back() noexcept(noexcept(std::declval<T>().~T())) {
    --used_size_;
    el_[used_size_].~T();
  }

  void clear() {
    for (auto& el : *this) {
      el.~T();
    }
    used_size_ = 0;
  }

  void reserve(size_type new_size) {
    new_size = std::max(allocated_size_, new_size);

    if (allocated_size_ >= new_size) {
      return;
    }

    auto next_size = next_power_of_two(new_size);
    auto num_bytes = static_cast<std::size_t>(next_size) * sizeof(T);
    auto mem_buf = static_cast<T*>(std::malloc(num_bytes));  // NOLINT
    if (mem_buf == nullptr) {
      throw_exception(std::bad_alloc());
    }

    if (size() != 0) {
      try {
        auto move_target = mem_buf;
        for (auto& el : *this) {
          new (move_target++) T(std::move(el));
        }

        for (auto& el : *this) {
          el.~T();
        }
      } catch (...) {
        assert(0);
      }
    }

    auto free_me = el_;
    el_ = mem_buf;
    if (self_allocated_) {
      std::free(free_me);  // NOLINT
    }

    self_allocated_ = true;
    allocated_size_ = next_size;
  }

  T* erase(T* pos) {
    auto const r = pos;
    T* last = end() - 1;
    while (pos < last) {
      std::swap(*pos, *(pos + 1));
      pos = pos + 1;
    }
    pos->~T();
    --used_size_;
    return r;
  }

  T* erase(T* first, T* last) {
    if (first != last) {
      auto const new_end = std::move(last, end(), first);
      for (auto it = new_end; it != end(); ++it) {
        it->~T();
      }
      used_size_ -= static_cast<size_type>(std::distance(new_end, end()));
    }
    return end();
  }

  bool contains(T const* el) const noexcept {
    return el >= begin() && el < end();
  }

  std::size_t index_of(T const* el) const noexcept {
    assert(contains(el));
    return std::distance(begin(), el);
  }

  friend bool operator==(basic_vector const& a,
                         basic_vector const& b) noexcept {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
  }
  friend bool operator!=(basic_vector const& a,
                         basic_vector const& b) noexcept {
    return !(a == b);
  }
  friend bool operator<(basic_vector const& a, basic_vector const& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
  }
  friend bool operator>(basic_vector const& a, basic_vector const& b) noexcept {
    return b < a;
  }
  friend bool operator<=(basic_vector const& a,
                         basic_vector const& b) noexcept {
    return !(a > b);
  }
  friend bool operator>=(basic_vector const& a,
                         basic_vector const& b) noexcept {
    return !(a < b);
  }

  void reset() noexcept {
    el_ = nullptr;
    used_size_ = {};
    allocated_size_ = {};
    self_allocated_ = false;
  }

  Ptr<T> el_{nullptr};
  size_type used_size_{0U};
  size_type allocated_size_{0U};
  bool self_allocated_{false};
  std::uint8_t __fill_0__{0U};
  std::uint16_t __fill_1__{0U};
  std::uint32_t __fill_2__{0U};
};

namespace raw {

template <typename T>
using vector = basic_vector<T, ptr>;

template <typename T>
using indexed_vector = basic_vector<T, ptr, true>;

template <typename Key, typename Value>
using vector_map = basic_vector<Value, ptr, false, Key>;

template <typename It, typename UnaryOperation>
auto to_vec(It s, It e, UnaryOperation&& op)
    -> vector<decay_t<decltype(op(*s))>> {
  vector<decay_t<decltype(op(*s))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(std::distance(s, e)));
  std::transform(s, e, std::back_inserter(v), op);
  return v;
}

template <typename Container, typename UnaryOperation>
auto to_vec(Container const& c, UnaryOperation&& op)
    -> vector<decltype(op(*std::begin(c)))> {
  vector<decltype(op(*std::begin(c)))> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::transform(std::begin(c), std::end(c), std::back_inserter(v), op);
  return v;
}

template <typename Container>
auto to_vec(Container const& c) -> vector<decay_t<decltype(*std::begin(c))>> {
  vector<decay_t<decltype(*std::begin(c))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::copy(std::begin(c), std::end(c), std::back_inserter(v));
  return v;
}
template <typename It, typename UnaryOperation>
auto to_indexed_vec(It s, It e, UnaryOperation&& op)
    -> indexed_vector<decay_t<decltype(op(*s))>> {
  indexed_vector<decay_t<decltype(op(*s))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(std::distance(s, e)));
  std::transform(s, e, std::back_inserter(v), op);
  return v;
}

template <typename Container, typename UnaryOperation>
auto to_indexed_vec(Container const& c, UnaryOperation&& op)
    -> indexed_vector<decay_t<decltype(op(*std::begin(c)))>> {
  indexed_vector<decay_t<decltype(op(*std::begin(c)))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::transform(std::begin(c), std::end(c), std::back_inserter(v), op);
  return v;
}

template <typename Container>
auto to_indexed_vec(Container const& c)
    -> indexed_vector<decay_t<decltype(*std::begin(c))>> {
  indexed_vector<decay_t<decltype(*std::begin(c))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::copy(std::begin(c), std::end(c), std::back_inserter(v));
  return v;
}

}  // namespace raw

namespace offset {

template <typename T>
using vector = basic_vector<T, ptr>;

template <typename T>
using indexed_vector = basic_vector<T, ptr, true>;

template <typename Key, typename Value>
using vector_map = basic_vector<Value, ptr, false, Key>;

template <typename It, typename UnaryOperation>
auto to_vec(It s, It e, UnaryOperation&& op)
    -> vector<decay_t<decltype(op(*s))>> {
  vector<decay_t<decltype(op(*s))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(std::distance(s, e)));
  std::transform(s, e, std::back_inserter(v), op);
  return v;
}

template <typename Container, typename UnaryOperation>
auto to_vec(Container&& c, UnaryOperation&& op)
    -> vector<decltype(op(*std::begin(c)))> {
  vector<decltype(op(*std::begin(c)))> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(c.size()));
  std::transform(std::begin(c), std::end(c), std::back_inserter(v), op);
  return v;
}

template <typename Container>
auto to_vec(Container&& c) -> vector<decay_t<decltype(*std::begin(c))>> {
  vector<decay_t<decltype(*std::begin(c))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::copy(std::begin(c), std::end(c), std::back_inserter(v));
  return v;
}
template <typename It, typename UnaryOperation>
auto to_indexed_vec(It s, It e, UnaryOperation&& op)
    -> indexed_vector<decay_t<decltype(op(*s))>> {
  indexed_vector<decay_t<decltype(op(*s))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(std::distance(s, e)));
  std::transform(s, e, std::back_inserter(v), op);
  return v;
}

template <typename Container, typename UnaryOperation>
auto to_indexed_vec(Container const& c, UnaryOperation&& op)
    -> indexed_vector<decay_t<decltype(op(*std::begin(c)))>> {
  indexed_vector<decay_t<decltype(op(*std::begin(c)))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::transform(std::begin(c), std::end(c), std::back_inserter(v), op);
  return v;
}

template <typename Container>
auto to_indexed_vec(Container const& c)
    -> indexed_vector<decay_t<decltype(*std::begin(c))>> {
  indexed_vector<decay_t<decltype(*std::begin(c))>> v;
  v.reserve(static_cast<typename decltype(v)::size_type>(
      std::distance(std::begin(c), std::end(c))));
  std::copy(std::begin(c), std::end(c), std::back_inserter(v));
  return v;
}

}  // namespace offset

#undef CISTA_TO_VEC

}  // namespace cista
