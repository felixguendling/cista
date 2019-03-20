#pragma once

#include <cassert>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <ostream>
#include <string>
#include <type_traits>

namespace cista {

template <typename T, typename Ptr = T*, typename TemplateSizeType = uint32_t>
struct basic_vector {
  using size_type = TemplateSizeType;
  using value_type = T;
  using iterator = T*;
  using const_iterator = T const*;

  static inline TemplateSizeType next_power_of_two(TemplateSizeType n) {
    n--;
    n |= n >> 1u;
    n |= n >> 2u;
    n |= n >> 4u;
    n |= n >> 8u;
    n |= n >> 16u;
    if constexpr (sizeof(TemplateSizeType) > 32) {
      n |= n >> 32u;
    }
    n++;
    return n;
  }

  basic_vector() = default;

  explicit basic_vector(TemplateSizeType size) { resize(size); }

  explicit basic_vector(const char* str) {
    auto length = static_cast<size_type>(std::strlen(str) + 1);
    reserve(length);
    std::memcpy(el_, str, length);
    used_size_ = length;
  }

  template <typename It>
  basic_vector(It begin_it, It end_it) {
    set(begin_it, end_it);
  }

  basic_vector(basic_vector&& arr) noexcept
      : el_(arr.el_),
        used_size_(arr.used_size_),
        allocated_size_(arr.allocated_size_),
        self_allocated_(arr.self_allocated_) {
    arr.el_ = nullptr;
    arr.used_size_ = 0;
    arr.self_allocated_ = false;
    arr.allocated_size_ = 0;
  }

  basic_vector(basic_vector const& arr) { set(std::begin(arr), std::end(arr)); }

  basic_vector& operator=(basic_vector&& arr) noexcept {
    deallocate();

    el_ = arr.el_;
    used_size_ = arr.used_size_;
    self_allocated_ = arr.self_allocated_;
    allocated_size_ = arr.allocated_size_;

    arr.el_ = nullptr;
    arr.used_size_ = 0;
    arr.self_allocated_ = false;
    arr.allocated_size_ = 0;

    return *this;
  }

  basic_vector& operator=(basic_vector const& arr) {
    set(std::begin(arr), std::end(arr));
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
    el_ = nullptr;
    used_size_ = 0;
    allocated_size_ = 0;
    self_allocated_ = 0;
  }

  T const* begin() const { return el_; }
  T const* end() const { return el_ + used_size_; }  // NOLINT
  T* begin() { return el_; }
  T* end() { return el_ + used_size_; }  // NOLINT

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

  friend T const* begin(basic_vector const& a) { return a.begin(); }
  friend T const* end(basic_vector const& a) { return a.end(); }

  friend T* begin(basic_vector& a) { return a.begin(); }
  friend T* end(basic_vector& a) { return a.end(); }

  inline T const& operator[](size_t index) const { return el_[index]; }
  inline T& operator[](size_t index) { return el_[index]; }

  T const& back() const { return el_[used_size_ - 1]; }
  T& back() { return el_[used_size_ - 1]; }

  T& front() { return el_[0]; }
  T const& front() const { return el_[0]; }

  inline TemplateSizeType size() const { return used_size_; }
  inline bool empty() const { return size() == 0; }

  basic_vector& operator=(std::string const& str) {
    *this = basic_vector(str.c_str());
    return *this;
  }

  template <typename It>
  void set(It begin_it, It end_it) {
    auto range_size = std::distance(begin_it, end_it);
    assert(range_size <= std::numeric_limits<TemplateSizeType>::max() &&
           "size tpye overflow");
    reserve(static_cast<TemplateSizeType>(range_size));

    auto copy_source = begin_it;
    auto copy_target = el_;
    for (; copy_source != end_it; ++copy_source, ++copy_target) {
      new (copy_target) T(*copy_source);
    }

    used_size_ = static_cast<TemplateSizeType>(range_size);
  }

  void push_back(T const& el) {
    reserve(used_size_ + 1);
    new (el_ + used_size_) T(el);
    ++used_size_;
  }

  template <typename... Args>
  T& emplace_back(Args&&... el) {
    reserve(used_size_ + 1);
    new (el_ + used_size_) T(std::forward<Args>(el)...);
    T* ptr = el_ + used_size_;
    ++used_size_;
    return *ptr;
  }

  void resize(size_type size) {
    reserve(size);
    for (auto i = used_size_; i < size; ++i) {
      new (el_ + i) T();
    }
    used_size_ = size;
  }

  void clear() {
    used_size_ = 0;
    for (auto& el : *this) {
      el.~T();
    }
  }

  void reserve(TemplateSizeType new_size) {
    new_size = std::max(allocated_size_, new_size);

    if (allocated_size_ >= new_size) {
      return;
    }

    auto next_size = next_power_of_two(new_size);
    auto num_bytes = sizeof(T) * next_size;
    auto mem_buf = static_cast<T*>(std::malloc(num_bytes));  // NOLINT
    if (mem_buf == nullptr) {
      throw std::bad_alloc();
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
    T* last = end() - 1;
    while (pos < last) {
      std::swap(*pos, *(pos + 1));
      pos = pos + 1;
    }
    pos->~T();
    --used_size_;
    return end();
  }

  bool contains(T const* el) const { return el >= begin() && el < end(); }

  std::string to_string() const { return std::string(el_); }

  explicit operator std::string() const { return to_string(); }

  Ptr el_{nullptr};
  TemplateSizeType used_size_{0};
  TemplateSizeType allocated_size_{0};
  bool self_allocated_{false};
  uint8_t __fill_0__{0};
  uint16_t __fill_1__{0};
  uint32_t __fill_2__{0};
};

template <typename T, typename Ptr, typename TemplateSizeType>
inline bool operator==(basic_vector<T, Ptr, TemplateSizeType> const& a,
                       basic_vector<T, Ptr, TemplateSizeType> const& b) {
  return a.size() == b.size() &&
         std::equal(std::begin(a), std::end(a), std::begin(b));
}

template <typename T, typename Ptr, typename TemplateSizeType>
inline bool operator<(basic_vector<T, Ptr, TemplateSizeType> const& a,
                      basic_vector<T, Ptr, TemplateSizeType> const& b) {
  return std::lexicographical_compare(std::begin(a), std::end(a), std::begin(b),
                                      std::end(b));
}

template <typename T, typename Ptr, typename TemplateSizeType>
inline bool operator<=(basic_vector<T, Ptr, TemplateSizeType> const& a,
                       basic_vector<T, Ptr, TemplateSizeType> const& b) {
  return !(a > b);
}

template <typename T, typename Ptr, typename TemplateSizeType>
inline bool operator>(basic_vector<T, Ptr, TemplateSizeType> const& a,
                      basic_vector<T, Ptr, TemplateSizeType> const& b) {
  return b < a;
}

template <typename T, typename Ptr, typename TemplateSizeType>
inline bool operator>=(basic_vector<T, Ptr, TemplateSizeType> const& a,
                       basic_vector<T, Ptr, TemplateSizeType> const& b) {
  return !(a < b);
}

}  // namespace cista
