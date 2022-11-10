#pragma once

#include <cstdlib>
#include <cstring>

#include "cista/verify.h"

namespace cista {

struct buffer final {
  constexpr buffer() noexcept = default;

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

  inline std::size_t size() const noexcept { return size_; }

  inline uint8_t* data() noexcept { return static_cast<uint8_t*>(buf_); }
  inline uint8_t const* data() const noexcept {
    return static_cast<uint8_t const*>(buf_);
  }

  inline uint8_t* begin() noexcept { return data(); }
  inline uint8_t* end() noexcept { return data() + size_; }

  inline uint8_t const* begin() const noexcept { return data(); }
  inline uint8_t const* end() const noexcept { return data() + size_; }

  uint8_t& operator[](size_t const i) noexcept { return data()[i]; }
  uint8_t const& operator[](size_t const i) const noexcept { return data()[i]; }

  void reset() noexcept {
    buf_ = nullptr;
    size_ = 0U;
  }

  void* buf_{};
  std::size_t size_{};
};

}  // namespace cista
