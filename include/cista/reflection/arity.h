#pragma once

#include <type_traits>

// Credits: Implementation by ChemiaAion
// https://gist.github.com/ChemiaAion/4118a3598f0b120b7f9c5884e9799a8b
//
// Resources:
// https://playfulprogramming.blogspot.com/2016/12/serializing-structs-with-c17-structured.html
// https://codereview.stackexchange.com/questions/142804/get-n-th-data-member-of-a-struct
// https://stackoverflow.com/questions/39768517/structured-bindings-width
// https://stackoverflow.com/questions/35463646/arity-of-aggregate-in-logarithmic-time
// https://stackoverflow.com/questions/38393302/returning-variadic-aggregates-struct-and-syntax-for-c17-variadic-template-c

namespace cista {

namespace detail {

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

}  // namespace detail

template <typename Aggregate>
constexpr std::size_t arity() {
  using AggregateDecay = std::remove_reference_t<std::remove_cv_t<Aggregate>>;
  return detail::arity_impl<AggregateDecay>().size();
}

}  // namespace cista
