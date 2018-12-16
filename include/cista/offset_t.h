#pragma once

#include <cinttypes>

namespace cista {

using offset_t = int64_t;

constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::max();

}  // namespace cista
