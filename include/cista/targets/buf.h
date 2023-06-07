#pragma once

#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "cista/hash.h"
#include "cista/offset_t.h"
#include "cista/serialized_size.h"
#include "cista/verify.h"

namespace cista {

using byte_buf = std::vector<std::uint8_t>;

template <typename Buf = byte_buf>
struct buf {
  buf() = default;
  explicit buf(Buf&& buf) : buf_{std::forward<Buf>(buf)} {}

  std::uint8_t* addr(offset_t const offset) noexcept {
    return (&buf_[0U]) + offset;
  }
  std::uint8_t* base() noexcept { return &buf_[0U]; }

  std::uint64_t checksum(offset_t const start = 0U) const noexcept {
    return hash(std::string_view{
        reinterpret_cast<char const*>(&buf_[static_cast<std::size_t>(start)]),
        buf_.size() - static_cast<std::size_t>(start)});
  }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    verify(buf_.size() >= pos + serialized_size<T>(), "out of bounds write");
    std::memcpy(&buf_[pos], &val, serialized_size<T>());
  }

  offset_t write(void const* ptr, std::size_t const num_bytes,
                 std::size_t alignment = 0U) {
    auto start = static_cast<offset_t>(size());
    if (alignment > 1U && buf_.size() != 0U) {
      auto unaligned_ptr = static_cast<void*>(addr(start));
      auto space = std::numeric_limits<std::size_t>::max();
      auto const aligned_ptr =
          std::align(alignment, num_bytes, unaligned_ptr, space);
      auto const new_offset = static_cast<offset_t>(
          aligned_ptr ? static_cast<std::uint8_t*>(aligned_ptr) - base() : 0U);
      auto const adjustment = static_cast<offset_t>(new_offset - start);
      start += adjustment;
    }

    auto const space_left =
        static_cast<int64_t>(buf_.size()) - static_cast<int64_t>(start);
    if (space_left < static_cast<int64_t>(num_bytes)) {
      auto const missing = static_cast<std::size_t>(
          static_cast<int64_t>(num_bytes) - space_left);
      buf_.resize(buf_.size() + missing);
    }
    std::memcpy(addr(start), ptr, num_bytes);

    return start;
  }

  std::uint8_t& operator[](std::size_t const i) noexcept { return buf_[i]; }
  std::uint8_t const& operator[](std::size_t const i) const noexcept {
    return buf_[i];
  }
  std::size_t size() const noexcept { return buf_.size(); }
  void reset() { buf_.resize(0U); }

  Buf buf_;
};

template <typename Buf>
buf(Buf&&) -> buf<Buf>;

}  // namespace cista
