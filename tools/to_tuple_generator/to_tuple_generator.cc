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

#include <tuple>

#include "cista/reflection/arity.h"

namespace cista {

template <typename T>
constexpr auto to_tuple_works_v =
    std::is_aggregate_v<T> &&
#if !defined(_MSC_VER) || defined(NDEBUG)
   std::is_standard_layout_v < T>&&
#endif
    !std::is_polymorphic_v<T>;

template <typename T>
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
    std::cout << R"( else if constexpr (a == )" << i << R"() {
    auto& [)" << var_list(i, false)
              << R"(] = t;
    return std::tie()"
              << var_list(i, false) << R"();
  })";
  }
  std::cout << "\n}\n";

  std::cout << R"(
template <typename T>
inline auto to_ptr_tuple(T& t) {
  constexpr auto const a = arity<T>();
  static_assert(a <= )"
            << max_members << R"(, "Max. supported members: )" << max_members
            << R"(");)"
            << R"(
  if constexpr (a == 0) {
    return std::make_tuple();
  })";
  for (auto i = 1U; i <= max_members; ++i) {
    std::cout << R"( else if constexpr (a == )" << i << R"() {
    auto& [)" << var_list(i, false)
              << R"(] = t;
    return std::make_tuple()"
              << var_list(i, true) << R"();
  })";
  }
  std::cout << "\n}";
  std::cout << R"(

}  // namespace cista
)";
}