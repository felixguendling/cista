#pragma once

#include <ostream>
#include <type_traits>

namespace cista {

template <typename T, typename Tag>
struct strong {
  using value_t = T;

  strong() = default;

  explicit strong(T const& v) : v_{v} {}
  explicit strong(T&& v) : v_{std::move(v)} {}

  strong(strong&& o) noexcept(std::is_nothrow_move_constructible_v<T>) =
      default;
  strong& operator=(strong&& o) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;

  strong(const strong& o) = default;
  strong& operator=(strong const& o) = default;

  template <typename X>
  friend constexpr X operator+(X const& t, strong const& s) {
    return t + s.v_;
  }

  template <typename X>
  friend constexpr X operator-(X const& t, strong const& s) {
    return t - s.v_;
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

private:
  T v_;
};

template <typename T>
struct is_strong : std::false_type {};

template <typename T, typename Tag>
struct is_strong<strong<T, Tag>> : std::true_type {};

template <typename T>
inline constexpr auto const is_strong_v = is_strong<T>::value;

}  // namespace cista
