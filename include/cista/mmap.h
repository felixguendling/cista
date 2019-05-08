#pragma once

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
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
  static constexpr auto const ENTIRE_FILE = std::numeric_limits<size_t>::max();
  enum class protection { READ, WRITE };

  explicit mmap(char const* path, protection const prot = protection::WRITE)
      : f_{path, prot == protection::READ ? "r" : "w+"},
        prot_{prot},
        size_{f_.size()},
        used_size_{f_.size()},
        addr_{size_ == 0U ? nullptr : map()} {}

  ~mmap() {
    if (addr_ != nullptr) {
      sync();
      size_ = used_size_;
      resize_file();
      unmap();
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
    o.addr_ = nullptr;
  }

  mmap& operator=(mmap&& o) {
    f_ = std::move(o.f_);
    prot_ = o.prot_;
    size_ = o.size_;
    used_size_ = o.used_size_;
    addr_ = o.addr_;
    o.addr_ = nullptr;
    return *this;
  }

  void sync() {
    if (addr_ != nullptr) {
#ifdef _MSC_VER
#else
      verify(::msync(addr_, size_, MS_SYNC) == 0, "sync error");
#endif
    }
  }

  void resize(size_t const new_size) {
    if (size_ < new_size) {
      resize_map(next_power_of_two(new_size));
    }
    used_size_ = new_size;
  }

  void reserve(size_t const new_size) {
    if (size_ < new_size) {
      resize_map(next_power_of_two(new_size));
    }
  }

  size_t size() const { return used_size_; }

  inline uint8_t* data() { return static_cast<unsigned char*>(addr_); }
  inline uint8_t const* data() const {
    return static_cast<unsigned char const*>(addr_);
  }

  inline uint8_t* begin() { return data(); }
  inline uint8_t* end() { return data() + size_; }
  inline uint8_t const* begin() const { return data(); }
  inline uint8_t const* end() const { return data() + size_; }

  unsigned char& operator[](size_t i) { return *(data() + i); }
  unsigned char const& operator[](size_t i) const { return *(data() + i); }

private:
  void unmap() {
#ifdef _MSC_VER
#else
    if (addr_ != nullptr) {
      ::munmap(addr_, size_);
      addr_ = nullptr;
    }
#endif
  }

  void* map() {
#ifdef _MSC_VER
#else
    auto const addr = ::mmap(nullptr, size_,
                             prot_ == protection::READ ? PROT_READ : PROT_WRITE,
                             MAP_SHARED, f_.fd(), OFFSET);
    verify(addr != nullptr, "map error");
    return addr;
#endif
  }

  void resize_file() {
#ifdef _MSC_VER
    verify(::_chsize_s(f_.fd(), static_cast<int64_t>(size_)) != 0,
           "resize error");
#else
    verify(::ftruncate(f_.fd(), static_cast<off_t>(size_)) == 0,
           "resize error");
#endif
  }

  void resize_map(size_t const new_size) {
    unmap();
    size_ = new_size;
    resize_file();
    addr_ = map();
  }

  file f_;
  protection prot_;
  size_t size_;
  size_t used_size_;
  void* addr_;
};

}  // namespace cista
