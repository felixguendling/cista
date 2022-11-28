#include <iostream>
#include <sstream>
#include <string>

std::string var_list(unsigned num, bool address_of) {
  std::stringstream ss;
  for (int i = 0; i < num; ++i) {
    if (address_of) {
      ss << "&";
    }
    ss << "p" << (i + 1);
    if (i != num - 1) {
      ss << ", ";
    }
  }
  return ss.str();
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " [max members]\n";
    return 1;
  }

  auto const max_members = std::stoi(argv[1]);

  std::cout << R"(
#pragma once

#include <functional>
#include <tuple>

#include "cista/reflection/arity.h"

namespace cista {

namespace detail {

template<typename T, typename = void>
struct has_cista_members : std::false_type {};

template<typename T>
struct has_cista_members<
  T,
  std::void_t<decltype(std::declval<std::decay_t<T>>().cista_members())>>
  : std::true_type {};

template <typename T>
inline constexpr auto const has_cista_members_v = has_cista_members<T>::value;

template <typename... Ts, std::size_t... I>
constexpr inline auto add_const_helper(std::tuple<Ts...>&& t,
                                       std::index_sequence<I...>) {
  return std::make_tuple(std::cref(std::get<I>(t))...);
}

template <typename T>
constexpr inline auto add_const(T&& t) {
  return add_const_helper(
      std::forward<T>(t),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(t)>>>());
}

template <typename... Ts, std::size_t... I>
auto to_ptrs_helper(std::tuple<Ts...>&& t, std::index_sequence<I...>) {
  return std::make_tuple(&std::get<I>(t)...);
}

template <typename T>
auto to_ptrs(T&& t) {
  return to_ptrs_helper(
      std::forward<T>(t),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<T>>>());
}

}  // namespace detail

template <typename T>
inline constexpr auto to_tuple_works_v =
    detail::has_cista_members_v<T> ||
      (std::is_aggregate_v<T> &&
#if !defined(_MSC_VER) || defined(NDEBUG)
       std::is_standard_layout_v < T>&&
#endif
       !std::is_polymorphic_v<T>);

template <typename T,
          std::enable_if_t<detail::has_cista_members_v<T> && std::is_const_v<T>,
                           void*> = nullptr>
constexpr inline auto to_tuple(T& t) {
  return detail::add_const(
      const_cast<std::add_lvalue_reference_t<std::remove_const_t<T>>>(t)
          .cista_members());
}

template <typename T,
          std::enable_if_t<detail::has_cista_members_v<T> && !std::is_const_v<T>,
                           void*> = nullptr>
constexpr inline auto to_tuple(T&& t) {
  return t.cista_members();
}

template <typename T,
          std::enable_if_t<!detail::has_cista_members_v<T>, void*> = nullptr>
inline auto to_tuple(T& t) {
  constexpr auto const a = arity<T>();
  static_assert(a <= )"
            << max_members << R"(, "Max. supported members: )" << max_members
            << R"(");)"
            << R"(
  if constexpr (a == 0) {
    return std::tie();
  })";

  for (auto i = 1U; i <= max_members; ++i) {
    std::cout << R"( else if constexpr (a == )" << i << R"(U) {
    auto& [)" << var_list(i, false)
              << R"(] = t;
    return std::tie()"
              << var_list(i, false) << R"();
  })";
  }
  std::cout << "\n}\n";

  std::cout << R"(
template <typename T>
inline auto to_ptr_tuple(T&& t) {
  return detail::to_ptrs(to_tuple(std::forward<T>(t)));
}
}  // namespace cista
)";
}
