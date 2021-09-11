#pragma once

#include "cista/reflection/for_each_field.h"

#include <limits>
#include <type_traits>

namespace cista {

template <typename T, typename MemberType>
size_t member_index(MemberType T::*const member_ptr) {
  auto i = 0U, field_index = std::numeric_limits<unsigned>::max();
  T t{};
  cista::for_each_field(t, [&](auto&& m) {
    if constexpr (std::is_same_v<decltype(&m), decltype(&(t.*member_ptr))>) {
      if (&m == &(t.*member_ptr)) {
        field_index = i;
      }
    }
    ++i;
  });
  return field_index;
}

}  // namespace cista
