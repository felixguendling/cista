#pragma once

#include <cinttypes>
#include <algorithm>

namespace cista {

template <typename Fn>
inline void chunk(unsigned const chunk_size, size_t const total, Fn fn) {
  size_t offset = 0;
  size_t remaining = total;
  while (remaining != 0) {
    auto const curr_chunk_size = static_cast<unsigned>(
        std::min(remaining, static_cast<size_t>(chunk_size)));
    fn(offset, curr_chunk_size);
    offset += curr_chunk_size;
    remaining -= curr_chunk_size;
  }
}

}  // namespace cista
