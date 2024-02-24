#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#include <string>
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

#ifdef _WIN32
namespace cista {

inline std::string last_error_str() {
  auto const err = ::GetLastError();
  if (err == 0) {
    return "no error";
  }

  struct buf {
    ~buf() {
      if (b_ != nullptr) {
        LocalFree(b_);
        b_ = nullptr;
      }
    }
    LPSTR b_ = nullptr;
  } b;
  auto const size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), b.b_, 0,
      nullptr);

  return size == 0 ? std::to_string(err) : std::string{b.b_, size};
}

inline HANDLE open_file(char const* path, char const* mode) {
  bool read = std::strcmp(mode, "r") == 0;
  bool write = std::strcmp(mode, "w+") == 0 || std::strcmp(mode, "r+") == 0;

  verify(read || write, "open file mode not supported");

  DWORD access = read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;
  DWORD create_mode = read ? OPEN_EXISTING : CREATE_ALWAYS;

  auto const f = CreateFileA(path, access, FILE_SHARE_READ, nullptr,
                             create_mode, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) {
    throw_exception(std::runtime_error{std::string{"cannot open path="} + path +
                                       ", mode=" + mode + ", message=\"" +
                                       last_error_str() + "\""});
  }
  return f;
}

struct file {
  file() = default;

  file(char const* path, char const* mode)
      : f_(open_file(path, mode)), size_{size()} {}

  ~file() {
    if (f_ != nullptr) {
      CloseHandle(f_);
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

  std::size_t size() const {
    if (f_ == nullptr) {
      return 0U;
    }
    LARGE_INTEGER filesize;
    verify(GetFileSizeEx(f_, &filesize), "file size error");
    return static_cast<std::size_t>(filesize.QuadPart);
  }

  buffer content() const {
    constexpr auto block_size = 8192u;
    std::size_t const file_size = size();

    auto b = buffer(file_size);

    chunk(block_size, size(), [&](std::size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<DWORD>(from);
#ifdef _WIN64
      overlapped.OffsetHigh = static_cast<DWORD>(from >> 32u);
#endif
      ReadFile(f_, b.data() + from, static_cast<DWORD>(block_size), nullptr,
               &overlapped);
    });

    return b;
  }

  std::uint64_t checksum(offset_t const start = 0) const {
    constexpr auto const block_size = 512U * 1024U;  // 512kB
    auto c = BASE_HASH;
    char buf[block_size];
    chunk(block_size, size_ - static_cast<std::size_t>(start),
          [&](auto const from, auto const size) {
            OVERLAPPED overlapped = {0};
            overlapped.Offset = static_cast<DWORD>(start + from);
#ifdef _WIN64
            overlapped.OffsetHigh = static_cast<DWORD>((start + from) >> 32U);
#endif
            DWORD bytes_read = {0};
            verify(ReadFile(f_, buf, static_cast<DWORD>(size), &bytes_read,
                            &overlapped),
                   "checksum read error");
            verify(bytes_read == size, "checksum read error bytes read");
            c = hash(std::string_view{buf, size}, c);
          });
    return c;
  }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    OVERLAPPED overlapped = {0};
    overlapped.Offset = static_cast<DWORD>(pos);
#ifdef _WIN64
    overlapped.OffsetHigh = pos >> 32u;
#endif
    DWORD bytes_written = {0};
    verify(WriteFile(f_, &val, sizeof(T), &bytes_written, &overlapped),
           "write(pos, val) write error");
    verify(bytes_written == sizeof(T),
           "write(pos, val) write error bytes written");
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

    std::uint8_t const buf[16U] = {0U};
    auto const num_padding_bytes = static_cast<DWORD>(curr_offset - size_);
    if (num_padding_bytes != 0U) {
      verify(num_padding_bytes < 16U, "invalid padding size");
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<std::uint32_t>(size_);
#ifdef _WIN64
      overlapped.OffsetHigh = static_cast<std::uint32_t>(size_ >> 32u);
#endif
      DWORD bytes_written = {0};
      verify(WriteFile(f_, buf, num_padding_bytes, &bytes_written, &overlapped),
             "write padding error");
      verify(bytes_written == num_padding_bytes,
             "write padding error bytes written");
      size_ = curr_offset;
    }

    constexpr auto block_size = 8192u;
    chunk(block_size, size, [&](std::size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = 0xFFFFFFFF;
      overlapped.OffsetHigh = 0xFFFFFFFF;
      DWORD bytes_written = {0};
      verify(WriteFile(f_, reinterpret_cast<std::uint8_t const*>(ptr) + from,
                       block_size, &bytes_written, &overlapped),
             "write error");
      verify(bytes_written == block_size, "write error bytes written");
    });

    auto const offset = size_;
    size_ += size;

    return offset;
  }

  HANDLE f_{nullptr};
  std::size_t size_{0U};
};
}  // namespace cista
#else

#include <cstdio>

#include <sys/stat.h>

namespace cista {

struct file {
  file() = default;

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

  std::size_t size() const {
    if (f_ == nullptr) {
      return 0U;
    }
    struct stat s;
    verify(fstat(fd(), &s) != -1, "fstat error");
    return static_cast<std::size_t>(s.st_size);
  }

  buffer content() {
    auto b = buffer(size());
    verify(std::fread(b.data(), 1U, b.size(), f_) == b.size(), "read error");
    return b;
  }

  std::uint64_t checksum(offset_t const start = 0) const {
    constexpr auto const block_size =
        static_cast<std::size_t>(512U * 1024U);  // 512kB
    verify(size_ >= static_cast<std::size_t>(start), "invalid checksum offset");
    verify(!std::fseek(f_, static_cast<long>(start), SEEK_SET), "fseek error");
    auto c = BASE_HASH;
    char buf[block_size];
    chunk(block_size, size_ - static_cast<std::size_t>(start),
          [&](auto const, auto const s) {
            verify(std::fread(buf, 1U, s, f_) == s, "invalid read");
            c = hash(std::string_view{buf, s}, c);
          });
    return c;
  }

  template <typename T>
  void write(std::size_t const pos, T const& val) {
    verify(!std::fseek(f_, static_cast<long>(pos), SEEK_SET), "seek error");
    verify(std::fwrite(reinterpret_cast<std::uint8_t const*>(&val), 1U,
                       serialized_size<T>(), f_) == serialized_size<T>(),
           "write error");
  }

  offset_t write(void const* ptr, std::size_t const size,
                 std::size_t alignment) {
    auto curr_offset = size_;
    auto seek_offset = long{0};
    auto seek_whence = int{SEEK_END};
    if (alignment > 1U) {
      auto unaligned_ptr = reinterpret_cast<void*>(size_);
      auto space = std::numeric_limits<std::size_t>::max();
      auto const aligned_ptr =
          std::align(alignment, size, unaligned_ptr, space);
      if (aligned_ptr != nullptr) {
        curr_offset = reinterpret_cast<std::uintptr_t>(aligned_ptr);
      }
      seek_offset = static_cast<long>(curr_offset);
      seek_whence = SEEK_SET;
    }
    verify(!std::fseek(f_, seek_offset, seek_whence), "seek error");
    verify(std::fwrite(ptr, 1U, size, f_) == size, "write error");
    size_ = curr_offset + size;
    return static_cast<offset_t>(curr_offset);
  }

  FILE* f_{nullptr};
  std::size_t size_{0U};
};

}  // namespace cista

#endif
