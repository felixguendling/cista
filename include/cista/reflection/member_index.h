#include "cista/reflection/for_each_field.h"

#include <type_traits>

namespace cista {

template <typename T, typename Interface>
size_t member_index(T Interface::*const member_ptr) {
  auto i = 0U, field_index = std::numeric_limits<unsigned>::max();
  Interface interface{};
  cista::for_each_field(interface, [&](auto&& m) {
    if constexpr (std::is_same_v<decltype(&m),
                                 decltype(&(interface.*member_ptr))>) {
      if (&m == &(interface.*member_ptr)) {
        field_index = i;
      }
    }
    ++i;
  });
  return field_index;
}

}  // namespace cista