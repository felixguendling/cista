#pragma once

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#endif

#include <cinttypes>
#include <memory>

#include "cista/buffer.h"
#include "cista/chunk.h"
#include "cista/hash.h"
#include "cista/offset_t.h"
#include "cista/serialized_size.h"
#include "cista/targets/file.h"
#include "cista/verify.h"

namespace cista {

#ifdef _MSC_VER
inline HANDLE open_file(char const* path, char const* mode) {
  bool read = std::strcmp(mode, "r") == 0;
  bool write = std::strcmp(mode, "w+") == 0;

  verify(read || write, "open file mode not supported");

  DWORD access = read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;
  DWORD create_mode = read ? OPEN_EXISTING : CREATE_ALWAYS;

  return CreateFileA(path, access, 0, nullptr, create_mode,
                     FILE_ATTRIBUTE_NORMAL, nullptr);
}

struct file {
  file(char const* path, char const* mode)
      : f_(open_file(path, mode)), size_{size()} {
    verify(f_ != INVALID_HANDLE_VALUE, "unable to open file");
  }

  ~file() {
    if (f_ != nullptr) {
      FlushFileBuffers(f_);
      CloseHandle(f_);
    }
  }

  file(file const&) = delete;
  file& operator=(file const&) = delete;

  file(file&& o) : f_{o.f_}, size_{o.size_} {
    o.f_ = nullptr;
    o.size_ = 0U;
  }

  file& operator=(file&& o) {
    f_ = o.f_;
    size_ = o.size_;
    o.f_ = nullptr;
    o.size_ = 0U;
    return *this;
  }

  size_t size() const {
    LARGE_INTEGER filesize;
    GetFileSizeEx(f_, &filesize);
    return filesize.QuadPart;
  }

  buffer content() const {
    constexpr auto block_size = 8192u;
    size_t const file_size = size();

    auto b = buffer(file_size);

    chunk(block_size, size(), [&](size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<DWORD>(from);
      overlapped.OffsetHigh = from >> 32u;
      ReadFile(f_, b.data() + from, static_cast<DWORD>(block_size), nullptr,
               &overlapped);
    });

    return b;
  }

  uint64_t checksum(offset_t const start = 0) const {
    constexpr auto const block_size = 512 * 1024;  // 512kB
    auto c = BASE_HASH;
    char buf[block_size];
    chunk(block_size, size_ - static_cast<size_t>(start),
          [&](auto const from, auto const size) {
            OVERLAPPED overlapped = {0};
            overlapped.Offset = static_cast<DWORD>(start + from);
            overlapped.OffsetHigh = static_cast<DWORD>((start + from) >> 32U);
            ReadFile(f_, buf, static_cast<DWORD>(size), nullptr, &overlapped);
            c = hash(std::string_view{buf, size}, c);
          });
    return c;
  }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    OVERLAPPED overlapped = {0};
    overlapped.Offset = static_cast<DWORD>(pos);
    overlapped.OffsetHigh = static_cast<DWORD>(pos >> 32U);
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

    constexpr auto block_size = 8192u;
    chunk(block_size, size, [&](size_t const from, unsigned block_size) {
      auto const pos = curr_offset + from;
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<DWORD>(pos);
      overlapped.OffsetHigh = static_cast<DWORD>(pos >> 32U);
      WriteFile(f_, reinterpret_cast<unsigned char const*>(ptr) + from,
                block_size, nullptr, &overlapped);
    });
    size_ = curr_offset + size;
    return curr_offset;
  }

  HANDLE f_;
  size_t size_{0U};
};
#else

#include <cstdio>

#include <sys/stat.h>

struct file {
  file(char const* path, char const* mode)
      : f_{std::fopen(path, mode)}, size_{size()} {
    verify(f_ != nullptr, "unable to open file");
  }

  ~file() {
    if (f_ != nullptr) {
      std::fclose(f_);
    }
    f_ = nullptr;
  }

  file(file const&) = delete;
  file& operator=(file const&) = delete;

  file(file&& o) : f_{o.f_}, size_{o.size_} {
    o.f_ = nullptr;
    o.size_ = 0U;
  }

  file& operator=(file&& o) {
    f_ = o.f_;
    size_ = o.size_;
    o.f_ = nullptr;
    o.size_ = 0U;
    return *this;
  }

  int fd() const {
    auto const fd = fileno(f_);
    verify(fd != -1, "invalid fd");
    return fd;
  }

  size_t size() const {
    struct stat s;
    verify(fstat(fd(), &s) != -1, "fstat error");
    return static_cast<size_t>(s.st_size);
  }

  buffer content() {
    auto b = buffer(size());
    verify(std::fread(b.data(), 1, b.size(), f_) == b.size(), "read error");
    return b;
  }

  uint64_t checksum(offset_t const start = 0) const {
    constexpr auto const block_size = static_cast<size_t>(512 * 1024);  // 512kB
    verify(size_ >= static_cast<size_t>(start), "invalid checksum offset");
    verify(!std::fseek(f_, static_cast<long>(start), SEEK_SET), "fseek error");
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
    verify(!std::fseek(f_, static_cast<long>(pos), SEEK_SET), "seek error");
    verify(std::fwrite(reinterpret_cast<unsigned char const*>(&val), 1,
                       serialized_size<T>(), f_) == serialized_size<T>(),
           "write error");
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
      verify(!std::fseek(f_, static_cast<long>(curr_offset), SEEK_SET),
             "seek error");
    } else {
      verify(!std::fseek(f_, 0, SEEK_END), "seek error");
    }
    verify(std::fwrite(ptr, 1, size, f_) == size, "write error");
    size_ = curr_offset + size;
    return static_cast<offset_t>(curr_offset);
  }

  FILE* f_;
  std::size_t size_ = 0u;
};
#endif

}  // namespace cista
