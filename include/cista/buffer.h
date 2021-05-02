#pragma once

#include <cstdlib>
#include <cstring>

#include "cista/verify.h"

namespace cista {

struct buffer final {
  buffer() noexcept : buf_(nullptr), size_(0) {}

  explicit buffer(std::size_t size) : buf_(std::malloc(size)), size_(size) {
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

  buffer(buffer&& o) noexcept : buf_(o.buf_), size_(o.size_) {
    o.buf_ = nullptr;
    o.size_ = 0;
  }

  buffer& operator=(buffer&& o) noexcept {
    buf_ = o.buf_;
    size_ = o.size_;
    o.buf_ = nullptr;
    o.size_ = 0;
    return *this;
  }

  inline std::size_t size() const noexcept { return size_; }

  inline unsigned char* data() noexcept {
    return static_cast<unsigned char*>(buf_);
  }
  inline unsigned char const* data() const noexcept {
    return static_cast<unsigned char const*>(buf_);
  }

  inline unsigned char* begin() noexcept { return data(); }
  inline unsigned char* end() noexcept { return data() + size_; }

  inline unsigned char const* begin() const noexcept { return data(); }
  inline unsigned char const* end() const noexcept { return data() + size_; }

  unsigned char& operator[](size_t i) noexcept { return data()[i]; }
  unsigned char const& operator[](size_t i) const noexcept { return data()[i]; }

  void* buf_;
  std::size_t size_;
};

}  // namespace cista
