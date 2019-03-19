#pragma once

#include <type_traits>

namespace cista {

struct instance {
  template <typename Type>
  operator Type() const;
};

template <typename Aggregate, typename IndexSequence = std::index_sequence<>,
          typename = void>
struct arity_impl : IndexSequence {};

template <typename Aggregate, std::size_t... Indices>
struct arity_impl<
    Aggregate, std::index_sequence<Indices...>,
    std::void_t<decltype(Aggregate{(Indices, std::declval<instance>())...,
                                   std::declval<instance>()})>>
    : arity_impl<Aggregate,
                 std::index_sequence<Indices..., sizeof...(Indices)>> {};

template <typename Aggregate>
constexpr std::size_t arity() {
  using aggregate_decay = std::remove_reference_t<std::remove_cv_t<Aggregate>>;

  if constexpr (std::is_aggregate_v<aggregate_decay> &&
                std::is_standard_layout_v<aggregate_decay> &&
                !std::is_polymorphic_v<aggregate_decay>) {
    return arity_impl<aggregate_decay>().size();
  } else {
    return 0;
  }
}

}  // namespace cista
