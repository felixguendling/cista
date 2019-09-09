#pragma once

#include <cinttypes>
#include <iterator>
#include <stdexcept>

namespace cista {

template <typename T, std::size_t Size>
struct array {
  array() = default;

  constexpr size_t size() const { return Size; }

  constexpr T const& operator[](size_t index) const { return el_[index]; }
  constexpr T& operator[](size_t index) { return el_[index]; }
  constexpr T& at(size_t index) {
    if (index >= Size) {
      throw std::out_of_range{"array index out of range"};
    }
    return el_[index];
  }
  constexpr T const& at(size_t index) const {
    return const_cast<array*>(this)->at(index);
  }

  constexpr T* begin() { return el_; }
  constexpr T const* begin() const { return el_; }
  constexpr T* end() { return el_ + Size; }
  constexpr T const* end() const { return el_ + Size; }

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

  constexpr friend T const* begin(array const& a) { return a.begin(); }
  constexpr friend T const* end(array const& a) { return a.end(); }

  constexpr friend T* begin(array& a) { return a.begin(); }
  constexpr friend T* end(array& a) { return a.end(); }

  constexpr T const& back() const { return el_[Size - 1]; }
  constexpr T& back() { return el_[Size - 1]; }

  constexpr T& front() { return el_[0]; }
  constexpr T const& front() const { return el_[0]; }

  constexpr T* data() { return el_; }
  constexpr T const* data() const { return el_; }

  T el_[Size];
};

namespace raw {
using cista::array;
}  // namespace raw

namespace offset {
using cista::array;
}  // namespace offset

}  // namespace cista