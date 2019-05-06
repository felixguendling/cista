#pragma once

#include "cista/buffer.h"
#include "cista/chunk.h"
#include "cista/verify.h"

#ifdef _MSC_VER
#include <cstdio>

#define NOMINMAX
#include <windows.h>

namespace cista {

inline HANDLE open_file(char const* path, char const* mode) {
  bool read = std::strcmp(mode, "r") == 0;
  bool write = std::strcmp(mode, "w+") == 0;

  verify(read || write, "invalid open file mode");

  DWORD access = read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;
  DWORD create_mode = read ? OPEN_EXISTING : CREATE_ALWAYS;

  return CreateFileA(path, access, 0, nullptr, create_mode,
                     FILE_ATTRIBUTE_NORMAL, nullptr);
}

struct file {
  file(char const* path, char const* mode) : f_(open_file(path, mode)) {
    verify(f_ != INVALID_HANDLE_VALUE, "unable to open file");
  }

  ~file() {
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

  buffer content() {
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

  void write(void const* buf, size_t size) {
    constexpr auto block_size = 8192u;
    chunk(block_size, size, [&](size_t const from, unsigned block_size) {
      OVERLAPPED overlapped = {0};
      overlapped.Offset = static_cast<DWORD>(from);
      overlapped.OffsetHigh = from >> 32u;
      WriteFile(f_, static_cast<unsigned char const*>(buf) + from, block_size,
                nullptr, &overlapped);
    });
  }

  HANDLE f_;
};
#else
#include <cstdio>

namespace cista {

struct file {
  file(char const* path, char const* mode) : f_(std::fopen(path, mode)) {
    verify(f_ != nullptr, "unable to open file");
  }

  ~file() {
    if (f_ != nullptr) {
      fclose(f_);
    }
    f_ = nullptr;
  }

  size_t size() {
    auto err = std::fseek(f_, 0, SEEK_END);
    verify(!err, "fseek to SEEK_END error");
    auto size = std::ftell(f_);
    std::rewind(f_);
    return static_cast<size_t>(size);
  }

  buffer content() {
    auto file_size = size();
    auto b = buffer(file_size);
    auto bytes_read = std::fread(b.data(), 1, file_size, f_);
    verify(bytes_read == file_size, "file read error");
    return b;
  }

  void write(void const* buf, size_t size) {
    auto bytes_written = std::fwrite(buf, 1, size, f_);
    verify(bytes_written == size, "file write error");
  }

  operator FILE*() { return f_; }

  FILE* f_;
};
#endif

}  // namespace cista
