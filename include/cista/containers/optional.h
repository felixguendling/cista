#pragma once

#include <optional>
#include <type_traits>
#include <utility>

#include "cista/exception.h"

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

  optional(optional const& other) { copy(other); }
  optional& operator=(optional const& other) { return copy(other); }

  optional(optional&& other) noexcept { move(std::forward<optional>(other)); }
  optional& operator=(optional&& other) noexcept {
    return move(std::forward<optional>(other));
  }

  optional& copy(optional const& other) {
    if (other.has_value()) {
      new (&storage_[0]) T{other.value()};
    }

    valid_ = other.has_value();

    return *this;
  }

  optional& move(optional&& other) {
    if (other.has_value()) {
      new (&storage_[0]) T(std::move(other.value()));
    }

    valid_ = other.has_value();

    return *this;
  }

  T const& value() const {
    if (!valid_) {
      throw_exception(std::bad_optional_access{});
    }
    return *reinterpret_cast<T const*>(&storage_[0]);
  }

  T& value() {
    if (!valid_) {
      throw_exception(std::bad_optional_access{});
    }
    return *reinterpret_cast<T*>(&storage_[0]);
  }

  bool has_value() const noexcept { return valid_; }
  operator bool() const noexcept { return valid_; }

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
