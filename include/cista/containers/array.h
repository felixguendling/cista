#pragma once

#include <cinttypes>
#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace cista {

template <typename T, std::size_t Size>
struct array {
  constexpr size_t size() const noexcept { return Size; }

  constexpr T const& operator[](size_t index) const noexcept {
    return el_[index];
  }
  constexpr T& operator[](size_t index) noexcept { return el_[index]; }
  constexpr T& at(size_t index) {
    if (index >= Size) {
      throw std::out_of_range{"array index out of range"};
    }
    return el_[index];
  }
  constexpr T const& at(size_t index) const {
    return const_cast<array*>(this)->at(index);
  }

  constexpr T* begin() noexcept { return el_; }
  constexpr T const* begin() const noexcept { return el_; }
  constexpr T* end() noexcept { return el_ + Size; }
  constexpr T const* end() const noexcept { return el_ + Size; }

  constexpr std::reverse_iterator<T const*> rbegin() const {
    return std::reverse_iterator<T*>(el_ + size());  // NOLINT
  }
  constexpr std::reverse_iterator<T const*> rend() const {
    return std::reverse_iterator<T*>(el_);
  }
  constexpr std::reverse_iterator<T*> rbegin() {
    return std::reverse_iterator<T*>(el_ + size());  // NOLINT
  }
  constexpr std::reverse_iterator<T*> rend() {
    return std::reverse_iterator<T*>(el_);
  }

  constexpr friend T const* begin(array const& a) noexcept { return a.begin(); }
  constexpr friend T const* end(array const& a) noexcept { return a.end(); }

  constexpr friend T* begin(array& a) { return a.begin(); }
  constexpr friend T* end(array& a) { return a.end(); }

  constexpr T const& back() const noexcept { return el_[Size - 1]; }
  constexpr T& back() noexcept { return el_[Size - 1]; }

  constexpr T& front() noexcept { return el_[0]; }
  constexpr T const& front() const noexcept { return el_[0]; }

  constexpr T* data() noexcept { return el_; }
  constexpr T const* data() const noexcept { return el_; }

  friend bool operator==(array const& a, array const& b) noexcept {
    for (auto i = 0U; i != Size; ++i) {
      if (a[i] != b[i]) {
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(array const& a, array const& b) noexcept {
    for (auto i = 0U; i != Size; ++i) {
      if (a[i] != b[i]) {
        return true;
      }
    }
    return false;
  }

  friend bool operator<(array const& a, array const& b) noexcept {
    using std::begin;
    using std::end;
    return std::lexicographical_compare(begin(a), end(a), begin(b), end(b));
  }
  friend bool operator>(array const& a, array const& b) noexcept {
    return b < a;
  }
  friend bool operator<=(array const& a, array const& b) noexcept {
    return !(a > b);
  }
  friend bool operator>=(array const& a, array const& b) noexcept {
    return !(a < b);
  }

  T el_[Size];
};

namespace raw {
using cista::array;
}  // namespace raw

namespace offset {
using cista::array;
}  // namespace offset

}  // namespace cista
