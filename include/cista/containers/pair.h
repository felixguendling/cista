#pragma once

#include <utility>

#include "cista/decay.h"
#include "cista/offset_t.h"
#include "cista/reflection/comparable.h"

namespace cista {

template <typename T1, typename T2>
struct pair {
  CISTA_COMPARABLE()
  using first_type = T1;
  using second_type = T2;
  auto cista_members() { return std::tie(first, second); }
  alignas(serialized_alignment_v<T1>) T1 first{};
  alignas(serialized_alignment_v<T2>) T2 second{};
};

template <typename T1, typename T2>
pair(T1&&, T2&&) -> pair<decay_t<T1>, decay_t<T2>>;

namespace raw {
using cista::pair;
}  // namespace raw

namespace offset {
using cista::pair;
}  // namespace offset

}  // namespace cista
