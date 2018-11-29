#include "doctest.h"

#include <sstream>
#include <string>
#include <vector>

#include "cista.h"

struct a {
  static constexpr auto const id = 77;
  int i_ = 1;
  int j_ = 2;
  double d_ = 100.0;
  std::string s_ = "hello world";
};

struct ignore {};

template <typename T>
std::vector<std::string> encode(T&& m) {
  using Type = std::decay_t<T>;
  std::vector<std::string> fields = {std::to_string(Type::id)};
  std::stringstream ss;
  cista::for_each_field(m, [&](auto&& f) {
    if constexpr (!std::is_same_v<
                      std::remove_const_t<std::remove_reference_t<decltype(f)>>,
                      ignore>) {
      ss << f;
      fields.emplace_back(ss.str());
      ss.str("");
    }
  });
  return fields;
}

TEST_CASE("for_each_field") {
  a instance;
  cista::for_each_field(instance, [](auto&& m) { m = {}; });
  CHECK(instance.i_ == 0);
  CHECK(instance.j_ == 0);
  CHECK(instance.d_ == 0.0);
  CHECK(instance.s_ == "");
  CHECK(std::vector<std::string>({"77", "0", "0", "0", ""}) ==
        encode(instance));
}

struct current_time_req {
  static constexpr auto const id = 49;
  int version_ = 1;
  ignore s_;
};

TEST_CASE("for_each_field^_1") {
  current_time_req a;
  CHECK(std::vector<std::string>({"49", "1"}) == encode(a));
}