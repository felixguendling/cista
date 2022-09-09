#pragma once

#include <optional>
#include <type_traits>
#include <utility>

namespace cista {

template <typename T>
struct optional {
  optional() = default;

  template <typename... Ts>
  optional(Ts&&... t) noexcept(std::is_nothrow_constructible_v<T, Ts...>) {
    new (&storage_[0]) T{std::forward<Ts>(t)...};
    valid_ = true;
  }

  template <typename... Ts>
  optional(std::nullopt_t) noexcept {}

  T const& value() const {
    if (!valid_) {
      throw std::bad_optional_access{};
    }
    return *reinterpret_cast<T const*>(&storage_[0]);
  }

  T& value() {
    if (!valid_) {
      throw std::bad_optional_access{};
    }
    return *reinterpret_cast<T*>(&storage_[0]);
  }

  bool has_value() const noexcept { return valid_; }

  T* operator->() noexcept { return reinterpret_cast<T*>(&storage_[0]); }
  T const* operator->() const noexcept {
    return reinterpret_cast<T const*>(&storage_[0]);
  }

  T& operator*() noexcept { return *reinterpret_cast<T*>(&storage_[0]); }
  T const& operator*() const noexcept {
    return *reinterpret_cast<T const*>(&storage_[0]);
  }

  bool valid_{false};
  alignas(std::alignment_of_v<T>) unsigned char storage_[sizeof(T)];
};

}  // namespace cista