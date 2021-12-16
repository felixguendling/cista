#pragma once

#include <variant>

#include "cista/buffer.h"
#include "cista/mmap.h"
#include "cista/targets/buf.h"

namespace cista {

using memory_holder = std::variant<buf<mmap>, buffer, byte_buf>;

}  // namespace cista
