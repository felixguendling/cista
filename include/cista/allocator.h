#pragma once

#include <memory>

namespace cista {

template <typename T, template <typename> typename Ptr>
class allocator : public std::allocator<T> {
public:
  using size_type = std::size_t;
  using pointer = Ptr<T>;
  using const_pointer = Ptr<T const>;

  template <typename T1>
  struct rebind {
    using other = allocator<T1, Ptr>;
  };

  using std::allocator<T>::allocator;
  using std::allocator<T>::allocate;
  using std::allocator<T>::deallocate;
};

}  // namespace cista