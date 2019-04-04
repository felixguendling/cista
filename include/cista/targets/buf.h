#pragma once

#include <cstdio>
#include <cstring>
#include <memory>

#include "cista/offset_t.h"
#include "cista/serialized_size.h"
#include "cista/verify.h"

namespace cista {

constexpr auto const MAX_ALIGN = 16;
using byte_buf = std::vector<uint8_t>;

struct buf {
  uint8_t* addr(offset_t const offset) { return (&buf_[0]) + offset; }
  uint8_t* base() { return &buf_[0]; }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    cista_verify(
        buf_.size() >= pos + serialized_size<T>(),
        "out of bounds write")
    std::memcpy(&buf_[pos], &val, serialized_size<T>());
  }

  offset_t write(void const* ptr, std::size_t const size,
                 std::size_t alignment = 0) {
    auto aligned_size = size;

    if (alignment != 0 && alignment != 1 && buf_.size() != 0) {
      auto unaligned_ptr = static_cast<void*>(addr(curr_offset_));
      auto space = std::numeric_limits<std::size_t>::max();
      auto const aligned_ptr =
          std::align(alignment, size, unaligned_ptr, space);
      auto const new_offset = static_cast<offset_t>(
          aligned_ptr ? static_cast<uint8_t*>(aligned_ptr) - base() : 0);
      auto const adjustment =
          static_cast<std::size_t>(new_offset - curr_offset_);
      curr_offset_ += adjustment;
      aligned_size += adjustment;
    }

    auto const space_left =
        static_cast<int64_t>(buf_.size()) - static_cast<int64_t>(curr_offset_);
    if (space_left < static_cast<int64_t>(aligned_size)) {
      auto const missing = static_cast<std::size_t>(
          static_cast<int64_t>(aligned_size) - space_left);
      buf_.resize(buf_.size() + missing);
    }

    auto const start = curr_offset_;
    std::memcpy(addr(curr_offset_), ptr, size);
    curr_offset_ += size;
    return start;
  }

  byte_buf buf_;
  offset_t curr_offset_{0};
  FILE* f_;
};

}  // namespace cista
