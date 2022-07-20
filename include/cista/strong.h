#pragma once

#include <limits>
#include <ostream>
#include <type_traits>

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
    return strong{v_ + s.v_};
  }
  constexpr strong operator-(strong const& s) const {
    return strong{v_ - s.v_};
  }
  constexpr strong operator*(strong const& s) const {
    return strong{v_ * s.v_};
  }
  constexpr strong operator/(strong const& s) const {
    return strong{v_ / s.v_};
  }
  constexpr strong operator+(T const& i) const { return strong{v_ + i}; }
  constexpr strong operator-(T const& i) const { return strong{v_ - i}; }
  constexpr strong operator*(T const& i) const { return strong{v_ * i}; }
  constexpr strong operator/(T const& i) const { return strong{v_ / i}; }

  constexpr strong& operator+=(T const& i) const {
    v_ += i;
    return *this;
  }
  constexpr strong& operator-=(T const& i) const {
    v_ -= i;
    return *this;
  }

  constexpr strong operator>>(T const& i) const { return strong{v_ >> i}; }
  constexpr strong operator<<(T const& i) const { return strong{v_ << i}; }
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

  explicit operator T const&() const& { return v_; }

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
inline constexpr auto const is_strong_v = is_strong<T>::value;

template <typename T, typename Tag>
constexpr typename strong<T, Tag>::value_t to_idx(strong<T, Tag> const& s) {
  return s.v_;
}

template <typename T>
T to_idx(T const& t) {
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

}  // namespace std
