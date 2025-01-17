#pragma once

#include <limits>
#include <ostream>
#include <type_traits>
#include <utility>

#include "cista/cuda_check.h"

namespace cista {

template <typename T, typename Tag>
struct strong {
  using value_t = T;

  constexpr strong() = default;

  explicit constexpr strong(T const& v) noexcept(
      std::is_nothrow_copy_constructible_v<T>)
      : v_{v} {}
  explicit constexpr strong(T&& v) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : v_{std::move(v)} {}

  template <typename X>
#if _MSVC_LANG >= 202002L || __cplusplus >= 202002L
    requires std::is_integral_v<std::decay_t<X>> &&
             std::is_integral_v<std::decay_t<T>>
#endif
  explicit constexpr strong(X&& x) : v_{static_cast<T>(x)} {
  }

  constexpr strong(strong&& o) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;
  constexpr strong& operator=(strong&& o) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;

  constexpr strong(strong const& o) = default;
  constexpr strong& operator=(strong const& o) = default;

  static constexpr strong invalid() {
    return strong{std::numeric_limits<T>::max()};
  }

  constexpr strong& operator++() {
    ++v_;
    return *this;
  }

  constexpr strong operator++(int) {
    auto cpy = *this;
    ++v_;
    return cpy;
  }

  constexpr strong& operator--() {
    --v_;
    return *this;
  }

  constexpr const strong operator--(int) {
    auto cpy = *this;
    --v_;
    return cpy;
  }

  constexpr strong operator+(strong const& s) const {
    return strong{static_cast<value_t>(v_ + s.v_)};
  }
  constexpr strong operator-(strong const& s) const {
    return strong{static_cast<value_t>(v_ - s.v_)};
  }
  constexpr strong operator*(strong const& s) const {
    return strong{static_cast<value_t>(v_ * s.v_)};
  }
  constexpr strong operator/(strong const& s) const {
    return strong{static_cast<value_t>(v_ / s.v_)};
  }
  constexpr strong operator+(T const& i) const {
    return strong{static_cast<value_t>(v_ + i)};
  }
  constexpr strong operator-(T const& i) const {
    return strong{static_cast<value_t>(v_ - i)};
  }
  constexpr strong operator*(T const& i) const {
    return strong{static_cast<value_t>(v_ * i)};
  }
  constexpr strong operator/(T const& i) const {
    return strong{static_cast<value_t>(v_ / i)};
  }

  constexpr strong& operator+=(T const& i) {
    v_ += i;
    return *this;
  }
  constexpr strong& operator-=(T const& i) {
    v_ -= i;
    return *this;
  }

  constexpr strong operator>>(T const& i) const {
    return strong{static_cast<value_t>(v_ >> i)};
  }
  constexpr strong operator<<(T const& i) const {
    return strong{static_cast<value_t>(v_ << i)};
  }
  constexpr strong operator>>(strong const& o) const { return v_ >> o.v_; }
  constexpr strong operator<<(strong const& o) const { return v_ << o.v_; }

  constexpr strong& operator|=(strong const& o) {
    v_ |= o.v_;
    return *this;
  }
  constexpr strong& operator&=(strong const& o) {
    v_ &= o.v_;
    return *this;
  }

  constexpr bool operator==(strong const& o) const { return v_ == o.v_; }
  constexpr bool operator!=(strong const& o) const { return v_ != o.v_; }
  constexpr bool operator<=(strong const& o) const { return v_ <= o.v_; }
  constexpr bool operator>=(strong const& o) const { return v_ >= o.v_; }
  constexpr bool operator<(strong const& o) const { return v_ < o.v_; }
  constexpr bool operator>(strong const& o) const { return v_ > o.v_; }

  constexpr bool operator==(T const& o) const { return v_ == o; }
  constexpr bool operator!=(T const& o) const { return v_ != o; }
  constexpr bool operator<=(T const& o) const { return v_ <= o; }
  constexpr bool operator>=(T const& o) const { return v_ >= o; }
  constexpr bool operator<(T const& o) const { return v_ < o; }
  constexpr bool operator>(T const& o) const { return v_ > o; }

  constexpr explicit operator T const&() const& noexcept { return v_; }

  friend std::ostream& operator<<(std::ostream& o, strong const& t) {
    return o << t.v_;
  }

  T v_;
};

template <typename T>
struct is_strong : std::false_type {};

template <typename T, typename Tag>
struct is_strong<strong<T, Tag>> : std::true_type {};

template <typename T>
constexpr auto const is_strong_v = is_strong<T>::value;

template <typename T, typename Tag>
CISTA_CUDA_COMPAT inline constexpr typename strong<T, Tag>::value_t to_idx(
    strong<T, Tag> const& s) {
  return s.v_;
}

template <typename T>
struct base_type {
  using type = T;
};

template <typename T, typename Tag>
struct base_type<strong<T, Tag>> {
  using type = T;
};

template <typename T>
using base_t = typename base_type<T>::type;

template <typename T>
CISTA_CUDA_COMPAT constexpr T to_idx(T const& t) {
  return t;
}

}  // namespace cista

#include <limits>

namespace std {

template <typename T, typename Tag>
class numeric_limits<cista::strong<T, Tag>> {
public:
  static constexpr cista::strong<T, Tag> min() noexcept {
    return cista::strong<T, Tag>{std::numeric_limits<T>::min()};
  }
  static constexpr cista::strong<T, Tag> max() noexcept {
    return cista::strong<T, Tag>{std::numeric_limits<T>::max()};
  }
  static constexpr bool is_integer = std::is_integral_v<T>;
};

template <typename T, typename Tag>
struct hash<cista::strong<T, Tag>> {
  size_t operator()(cista::strong<T, Tag> const& t) const {
    return hash<T>{}(t.v_);
  }
};

}  // namespace std

#if defined(CISTA_FMT)

#include "fmt/ostream.h"

template <typename T, typename Tag>
struct fmt::formatter<cista::strong<T, Tag>> : ostream_formatter {};

#endif
