#include <sstream>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/reflection/for_each_field.h"
#include "cista/reflection/printable.h"
#include "cista/serialization.h"
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
  CISTA_PRINTABLE(a)
  auto cista_members() noexcept { return std::tie(a_, b_, c_); }
  int a_{1}, b_{2}, c_{3};
};
struct b : public x {
  CISTA_PRINTABLE(b, "a", "b", "c")
  auto cista_members() noexcept { return std::tie(a_, b_, c_); }
  int a_{1}, b_{2}, c_{3};
};

TEST_CASE("cista_members printable") {
  std::stringstream ss;
  ss << a{};
  CHECK(ss.str() == "{1, 2, 3}");
}

TEST_CASE("cista_members printable names") {
  std::stringstream ss;
  ss << b{};
  CHECK(ss.str() == "{a=1, b=2, c=3}");
}

struct parent {
  parent() = default;
  explicit parent(double u) : u_{u} {}
  auto cista_members() noexcept { return std::tie(u_, v_, w_); }
  double u_{1}, v_{2}, w_{3};
};
struct serialize_me : public parent {
  static int s_;
  auto cista_members() noexcept { return std::tie(u_, v_, w_, a_, j_); }
  int a_{0};
  struct inner : public parent {
    auto cista_members() noexcept { return std::tie(u_, v_, w_, b_, c_, d_); }
    int b_{0};
    int c_{0};
    cista::raw::string d_;
  } j_;
};

TEST_CASE("cista_members serialization") {
  cista::byte_buf buf;

  {
    serialize_me obj;
    obj.a_ = 1;
    obj.j_.b_ = 2;
    obj.j_.c_ = 3;
    obj.j_.d_ = cista::raw::string{"testtes"};
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me));

  auto const serialized =
      cista::deserialize<serialize_me>(&buf[0], &buf[0] + buf.size());
  CHECK(serialized->a_ == 1);
  CHECK(serialized->j_.b_ == 2);
  CHECK(serialized->j_.c_ == 3);
  CHECK(serialized->j_.d_ == cista::raw::string{"testtes"});
}

}  // namespace cista_members_printable
