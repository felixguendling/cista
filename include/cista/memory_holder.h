#pragma once

#include <variant>

#include "cista/buffer.h"
#include "cista/containers/unique_ptr.h"
#include "cista/mmap.h"
#include "cista/targets/buf.h"

namespace cista {

using memory_holder = std::variant<buf<mmap>, buffer, byte_buf>;

template <typename T>
struct wrapped {
  wrapped(memory_holder mem, T* el) : mem_{std::move(mem)}, el_{el} {
    el_.self_allocated_ = false;
  }
  explicit wrapped(raw::unique_ptr<T> el) : el_{std::move(el)} {}
  memory_holder mem_;
  raw::unique_ptr<T> el_;
};

}  // namespace cista
