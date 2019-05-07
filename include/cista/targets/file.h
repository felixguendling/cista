#pragma once

#include <cstdio>
#include <memory>

#include "cista/chunk.h"
#include "cista/file.h"
#include "cista/hash.h"
#include "cista/offset_t.h"
#include "cista/serialized_size.h"
#include "cista/verify.h"

namespace cista {

#ifdef _MSC_VER
struct sfile {
  sfile(char const* path, char const* mode)
      : f_(open_file(path, mode)), size_{size()} {
    verify(f_ != INVALID_HANDLE_VALUE, "unable to open file");
  }

  ~sfile() {
    if (f_ != nullptr) {
      FlushFileBuffers(f_);
      CloseHandle(f_);
    }
  }

  size_t size() {
    LARGE_INTEGER filesize;
    GetFileSizeEx(f_, &filesize);
    return filesize.QuadPart;
  }

  uint64_t checksum(offset_t const start = 0) const {
    constexpr auto const block_size = 512 * 1024;  // 512kB
    SetFilePointer(f_, 0, 0, FILE_BEGIN);
    auto c = BASE_HASH;
    char buf[block_size];
    chunk(block_size, size_ - static_cast<size_t>(start),
          [&](auto const from, auto const size) {
            OVERLAPPED overlapped = {0};
            overlapped.Offset = static_cast<DWORD>(from);
            overlapped.OffsetHigh = from >> 32u;
            ReadFile(f_, buf, static_cast<DWORD>(size), nullptr, &overlapped);
            c = hash(std::string_view{buf, size}, c);
          });
    return c;
  }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    OVERLAPPED overlapped = {0};
    overlapped.Offset = static_cast<DWORD>(pos);
    overlapped.OffsetHigh = pos >> 32u;
    WriteFile(f_, &val, sizeof(T), nullptr, &overlapped);
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
    }

    unsigned char const buf[16] = {0};
    auto const num_padding_bytes = static_cast<DWORD>(curr_offset - size_);
    OVERLAPPED overlapped = {0};
    overlapped.Offset = static_cast<uint32_t>(size_);
    overlapped.OffsetHigh = static_cast<uint32_t>(size_ >> 32u);
    WriteFile(f_, buf, num_padding_bytes, nullptr, &overlapped);
    size_ = curr_offset;

    constexpr auto block_size = 8192u;
    chunk(block_size, size, [&](size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = 0xFFFFFFFF;
      overlapped.OffsetHigh = 0xFFFFFFFF;
      WriteFile(f_, reinterpret_cast<unsigned char const*>(ptr) + from,
                block_size, nullptr, &overlapped);
    });

    auto const offset = size_;
    size_ += size;

    return offset;
  }

  HANDLE f_;
  size_t size_{0U};
};
#else
inline FILE* s_open_file(char const* path, char const* mode) {
  return std::fopen(path, mode);
}

struct sfile {
  sfile(char const* path, char const* mode)
      : f_{s_open_file(path, mode)}, size_{size()} {
    verify(f_ != nullptr, "unable to open file");
  }

  ~sfile() {
    if (f_ != nullptr) {
      std::fclose(f_);
    }
    f_ = nullptr;
  }

  size_t size() const {
    verify(!std::fseek(f_, 0, SEEK_END), "fseek to SEEK_END error");
    auto size = std::ftell(f_);
    std::rewind(f_);
    return static_cast<size_t>(size);
  }

  uint64_t checksum(offset_t const start = 0) const {
    verify(size_ >= static_cast<size_t>(start), "invalid checksum offset");
    constexpr auto const block_size = static_cast<size_t>(512 * 1024);  // 512kB
    std::fseek(f_, static_cast<long>(start), SEEK_SET);
    auto c = BASE_HASH;
    char buf[block_size];
    chunk(block_size, size_ - static_cast<size_t>(start),
          [&](auto const, auto const s) {
            verify(std::fread(buf, 1, s, f_) == s, "invalid read");
            c = hash(std::string_view{buf, s}, c);
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

  FILE* f_;
  std::size_t size_ = 0u;
};
#endif

}  // namespace cista
