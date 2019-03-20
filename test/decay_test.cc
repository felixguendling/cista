#include "doctest.h"

#include "cista.h"

using cista::decay_t;

static_assert(std::is_same_v<decay_t<std::reference_wrapper<int const>>, int>);
static_assert(std::is_same_v<decay_t<std::reference_wrapper<int const&>>, int>);
static_assert(
    std::is_same_v<decay_t<std::reference_wrapper<int const&&>>, int>);
static_assert(
    std::is_same_v<decay_t<std::reference_wrapper<int const&&>&>, int>);
static_assert(std::is_same_v<decay_t<int const>, int>);
static_assert(std::is_same_v<decay_t<int const&>, int>);

void test(int const& i) {
  static_assert(std::is_same_v<decay_t<decltype(i)>, int>);
}

TEST_CASE("reference wrapper field") {
  int i;

  struct a {
    explicit a(int& i) : i_{i} {}
    std::reference_wrapper<int> i_;
  } instance{i};

  cista::for_each_field(instance, [](auto&& f) { f.get() = 7; });

  CHECK(i == 7);
}

template <typename T>
auto decay_type(T& type) {
  using DecayedType = decay_t<decltype(type)>;
  return DecayedType(type);
}

TEST_CASE("decay") {
  struct a {
    int number = 42;
  } instance;

  auto& simple = instance;
  auto wrapper = std::reference_wrapper<a>(instance);

  auto result1 = decay_type(instance);
  auto result2 = decay_type(simple);
  auto result3 = decay_type(wrapper);

  instance.number = 0;

  CHECK(result1.number == 42);
  CHECK(result2.number == 42);
  CHECK(result3.number == 42);
}