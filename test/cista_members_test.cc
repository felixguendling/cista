#include <sstream>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/reflection/for_each_field.h"
#include "cista/reflection/printable.h"
#endif

TEST_CASE("cista_members to tuple") {
  struct x {};
  struct a : public x {
    auto cista_members() noexcept { return std::tie(a_, b_, c_); }
    int a_{1}, b_{2}, c_{3};
  };
  static_assert(cista::detail::has_cista_members_v<a>);
  auto const obj = a{};
  CHECK(std::get<0>(cista::to_tuple(obj)) == 1);
  CHECK(std::get<1>(cista::to_tuple(obj)) == 2);
  CHECK(std::get<2>(cista::to_tuple(obj)) == 3);
  static_assert(
      std::is_same_v<decltype(std::get<2>(cista::to_tuple(obj))), int const&>);
  static_assert(std::is_same_v<
                std::decay_t<decltype(std::get<2>(cista::to_ptr_tuple(obj)))>,
                int const*>);

  auto obj1 = a{};
  std::get<2>(cista::to_tuple(obj1)) = 77;
  static_assert(
      std::is_same_v<decltype(std::get<2>(cista::to_tuple(obj1))), int&>);
  static_assert(std::is_same_v<
                std::decay_t<decltype(std::get<2>(cista::to_ptr_tuple(obj1)))>,
                int*>);
  CHECK(obj1.c_ == 77);
}

namespace cista_members_printable {

struct x {};
struct a : public x {
  CISTA_PRINTABLE(a);
  auto cista_members() noexcept { return std::tie(a_, b_, c_); }
  int a_{1}, b_{2}, c_{3};
};

TEST_CASE("cista_members printable") {
  std::stringstream ss;
  ss << a{};
  CHECK(ss.str() == "{1, 2, 3}");
}

}  // namespace cista_members_printable
