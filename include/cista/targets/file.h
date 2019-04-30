#pragma once

#include <cstdio>
#include <memory>

#include "cista/chunk.h"
#include "cista/crc64.h"
#include "cista/offset_t.h"
#include "cista/serialized_size.h"
#include "cista/verify.h"

#ifdef _MSC_VER
inline FILE* s_open_file(char const* path, char const* mode) {
  FILE* ptr = nullptr;
  fopen_s(&ptr, path, mode);
  return ptr;
}
#else
inline FILE* s_open_file(char const* path, char const* mode) {
  return std::fopen(path, mode);
}
#endif

namespace cista {

struct sfile {
  sfile(char const* path, char const* mode) : f_(s_open_file(path, mode)) {
    verify(f_ != nullptr, "unable to open file");
  }

  ~sfile() {
    if (f_ != nullptr) {
      std::fclose(f_);
    }
    f_ = nullptr;
  }

  uint64_t checksum(offset_t const start = 0) const {
    constexpr auto const block_size = 512 * 1024;  // 512kB
    std::fseek(f_, start, SEEK_SET);
    auto c = uint64_t{0ULL};
    char buf[block_size];
    chunk(block_size, size_ - static_cast<size_t>(start),
          [&](auto const, auto const size) {
            verify(std::fread(buf, 1, size, f_) == size, "invalid read");
            c = crc64(std::string_view{buf, size}, c);
          });
    return c;
  }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    std::fseek(f_, static_cast<long>(pos), SEEK_SET);
    auto const w = std::fwrite(reinterpret_cast<unsigned char const*>(&val), 1,
                               serialized_size<T>(), f_);
    verify(w == serialized_size<T>(), "write error");
  }

  offset_t write(void const* ptr, std::size_t const size,
                 std::size_t alignment) {
    auto curr_offset = size_;
    if (alignment != 0 && alignment != 1) {
      auto unaligned_ptr = reinterpret_cast<void*>(size_);
      auto space = std::numeric_limits<std::size_t>::max();
      auto const aligned_ptr =
          std::align(alignment, size, unaligned_ptr, space);
      curr_offset = aligned_ptr ? reinterpret_cast<std::uintptr_t>(aligned_ptr)
                                : curr_offset;
      std::fseek(f_, static_cast<long>(curr_offset), SEEK_SET);
    } else {
      std::fseek(f_, 0, SEEK_END);
    }
    auto const w = std::fwrite(ptr, 1, size, f_);
    verify(w == size, "write error");
    size_ = curr_offset + size;
    return static_cast<offset_t>(curr_offset);
  }

  std::size_t size_ = 0u;
  FILE* f_;
};

}  // namespace cista
