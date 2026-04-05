#pragma once

#include "cista/containers.h"
#include "cista/mode.h"

namespace cista {

struct free_self_allocated_context {
  static constexpr auto const MODE = mode::NONE;

  template <typename T>
  void add_checked(T&&) {}
};

template <typename T>
void free_self_allocated_impl(T*) {}

template <typename T, template <typename> typename Ptr, bool Indexed,
          typename TemplateSizeType>
void free_self_allocated_impl(
    basic_vector<T, Ptr, Indexed, TemplateSizeType>* el) {
  el->deallocate();
}

template <typename Ptr>
void free_self_allocated_impl(generic_string<Ptr>* el) {
  el->reset();
}

template <typename Ptr>
void free_self_allocated_impl(basic_string<Ptr>* el) {
  free_self_allocated_impl(static_cast<generic_string<Ptr>*>(el));
}

template <typename Ptr>
void free_self_allocated_impl(generic_cstring<Ptr>* el) {
  el->reset();
}

template <typename Ptr>
void free_self_allocated_impl(basic_cstring<Ptr>* el) {
  free_self_allocated_impl(static_cast<generic_cstring<Ptr>*>(el));
}

template <typename T, typename Ptr>
void free_self_allocated_impl(basic_unique_ptr<T, Ptr>* el) {
  el->reset();
}

template <typename T, template <typename> typename Ptr, typename GetKey,
          typename GetValue, typename Hash, typename Eq>
void free_self_allocated_impl(
    hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq>* el) {
  if (el->self_allocated_) {
    el->clear();
    el->reset();
  }
}

template <typename Ctx, typename T>
void free_self_allocated(Ctx& c, T* el) {
  recurse(c, el, [&](auto* entry) { free_self_allocated(c, entry); });
  free_self_allocated_impl(el);
}

template <typename T>
void free_self_allocated(T* el) {
  free_self_allocated_context c;
  free_self_allocated(c, el);
}

}  // namespace cista
