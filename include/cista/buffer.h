#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "cista/verify.h"

namespace cista {

struct buffer final {
  constexpr buffer() noexcept : buf_(nullptr), size_(0U) {}

  explicit buffer(std::size_t const size)
      : buf_(std::malloc(size)), size_(size) {
    verify(buf_ != nullptr, "buffer initialization failed");
  }

  explicit buffer(char const* str) : buffer(std::strlen(str)) {
    std::memcpy(buf_, str, size_);
  }

  buffer(char const* str, std::size_t size) : buffer(size) {
    std::memcpy(buf_, str, size_);
  }

  ~buffer() {
    std::free(buf_);
    buf_ = nullptr;
  }

  buffer(buffer const&) = delete;
  buffer& operator=(buffer const&) = delete;

  buffer(buffer&& o) noexcept : buf_(o.buf_), size_(o.size_) { o.reset(); }

  buffer& operator=(buffer&& o) noexcept {
    buf_ = o.buf_;
    size_ = o.size_;
    o.reset();
    return *this;
  }

  std::size_t size() const noexcept { return size_; }

  std::uint8_t* data() noexcept { return static_cast<std::uint8_t*>(buf_); }
  std::uint8_t const* data() const noexcept {
    return static_cast<std::uint8_t const*>(buf_);
  }

  std::uint8_t* begin() noexcept { return data(); }
  std::uint8_t* end() noexcept { return data() + size_; }

  std::uint8_t const* begin() const noexcept { return data(); }
  std::uint8_t const* end() const noexcept { return data() + size_; }

  std::uint8_t& operator[](std::size_t const i) noexcept { return data()[i]; }
  std::uint8_t const& operator[](std::size_t const i) const noexcept {
    return data()[i];
  }

  void reset() noexcept {
    buf_ = nullptr;
    size_ = 0U;
  }

  void* buf_;
  std::size_t size_;
};

}  // namespace cista
