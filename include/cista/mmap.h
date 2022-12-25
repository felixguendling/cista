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
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "cista/next_power_of_2.h"
#include "cista/targets/file.h"

namespace cista {

struct mmap {
  static constexpr auto const OFFSET = 0ULL;
  static constexpr auto const ENTIRE_FILE =
      std::numeric_limits<std::size_t>::max();
  enum class protection { READ, WRITE, MODIFY };

  mmap() = default;

  explicit mmap(char const* path, protection const prot = protection::WRITE)
      : f_{path, prot == protection::MODIFY
                     ? "r+"
                     : (prot == protection::READ ? "r" : "w+")},
        prot_{prot},
        size_{f_.size()},
        used_size_{f_.size()},
        addr_{size_ == 0U ? nullptr : map()} {}

  ~mmap() {
    if (addr_ != nullptr) {
      sync();
      size_ = used_size_;
      unmap();
      if (size_ != f_.size()) {
        resize_file();
      }
    }
  }

  mmap(mmap const&) = delete;
  mmap& operator=(mmap const&) = delete;

  mmap(mmap&& o)
      : f_{std::move(o.f_)},
        prot_{o.prot_},
        size_{o.size_},
        used_size_{o.used_size_},
        addr_{o.addr_} {
#ifdef _WIN32
    file_mapping_ = o.file_mapping_;
#endif
    o.addr_ = nullptr;
  }

  mmap& operator=(mmap&& o) {
    f_ = std::move(o.f_);
    prot_ = o.prot_;
    size_ = o.size_;
    used_size_ = o.used_size_;
    addr_ = o.addr_;
#ifdef _WIN32
    file_mapping_ = o.file_mapping_;
#endif
    o.addr_ = nullptr;
    return *this;
  }

  void sync() {
    if ((prot_ == protection::WRITE || prot_ == protection::MODIFY) &&
        addr_ != nullptr) {
#ifdef _WIN32
      verify(::FlushViewOfFile(addr_, size_) != 0, "flush error");
      verify(::FlushFileBuffers(f_.f_) != 0, "flush error");
#else
      verify(::msync(addr_, size_, MS_SYNC) == 0, "sync error");
#endif
    }
  }

  void resize(std::size_t const new_size) {
    verify(prot_ == protection::WRITE || prot_ == protection::MODIFY,
           "read-only not resizable");
    if (size_ < new_size) {
      resize_map(next_power_of_two(new_size));
    }
    used_size_ = new_size;
  }

  void reserve(std::size_t const new_size) {
    verify(prot_ == protection::WRITE || prot_ == protection::MODIFY,
           "read-only not resizable");
    if (size_ < new_size) {
      resize_map(next_power_of_two(new_size));
    }
  }

  std::size_t size() const noexcept { return used_size_; }

  std::string_view view() const noexcept {
    return {static_cast<char const*>(addr_), size()};
  }
  std::uint8_t* data() noexcept { return static_cast<std::uint8_t*>(addr_); }
  std::uint8_t const* data() const noexcept {
    return static_cast<std::uint8_t const*>(addr_);
  }

  std::uint8_t* begin() noexcept { return data(); }
  std::uint8_t* end() noexcept { return data() + used_size_; }
  std::uint8_t const* begin() const noexcept { return data(); }
  std::uint8_t const* end() const noexcept { return data() + used_size_; }

  std::uint8_t& operator[](std::size_t const i) noexcept { return data()[i]; }
  std::uint8_t const& operator[](std::size_t const i) const noexcept {
    return data()[i];
  }

private:
  void unmap() {
#ifdef _WIN32
    if (addr_ != nullptr) {
      verify(::UnmapViewOfFile(addr_), "unmap error");
      addr_ = nullptr;

      verify(::CloseHandle(file_mapping_), "close file mapping error");
      file_mapping_ = nullptr;
    }
#else
    if (addr_ != nullptr) {
      ::munmap(addr_, size_);
      addr_ = nullptr;
    }
#endif
  }

  void* map() {
#ifdef _WIN32
    auto const size_low = static_cast<DWORD>(size_);
#ifdef _WIN64
    auto const size_high = static_cast<DWORD>(size_ >> 32U);
#else
    auto const size_high = static_cast<DWORD>(0U);
#endif
    const auto fm = ::CreateFileMapping(
        f_.f_, 0, prot_ == protection::READ ? PAGE_READONLY : PAGE_READWRITE,
        size_high, size_low, 0);
    verify(fm != NULL, "file mapping error");
    file_mapping_ = fm;

    auto const addr = ::MapViewOfFile(
        fm, prot_ == protection::READ ? FILE_MAP_READ : FILE_MAP_WRITE, OFFSET,
        OFFSET, size_);
    verify(addr != nullptr, "map error");

    return addr;
#else
    auto const addr =
        ::mmap(nullptr, size_,
               prot_ == protection::READ ? PROT_READ : PROT_READ | PROT_WRITE,
               MAP_SHARED, f_.fd(), OFFSET);
    verify(addr != MAP_FAILED, "map error");
    return addr;
#endif
  }

  void resize_file() {
    if (prot_ == protection::READ) {
      return;
    }

#ifdef _WIN32
    LARGE_INTEGER Size = {0};
    verify(::GetFileSizeEx(f_.f_, &Size), "resize: get file size error");

    LARGE_INTEGER Distance = {0};
    Distance.QuadPart = size_ - Size.QuadPart;
    verify(::SetFilePointerEx(f_.f_, Distance, nullptr, FILE_END),
           "resize error");
    verify(::SetEndOfFile(f_.f_), "resize set eof error");
#else
    verify(::ftruncate(f_.fd(), static_cast<off_t>(size_)) == 0,
           "resize error");
#endif
  }

  void resize_map(std::size_t const new_size) {
    if (prot_ == protection::READ) {
      return;
    }

    unmap();
    size_ = new_size;
    resize_file();
    addr_ = map();
  }

  file f_;
  protection prot_;
  std::size_t size_;
  std::size_t used_size_;
  void* addr_;
#ifdef _WIN32
  HANDLE file_mapping_;
#endif
};

}  // namespace cista
