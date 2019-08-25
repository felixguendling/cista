#pragma once

#include "cista/containers/offset_ptr.h"

namespace cista {

namespace raw {

template <typename T>
using ptr = T*;

}  // namespace raw

namespace offset {

template <typename T>
using ptr = cista::offset_ptr<T>;

}  // namespace offset

}  // namespace cista