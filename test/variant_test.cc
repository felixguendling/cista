#include "doctest.h"

#include <array>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/variant.h"
#endif

namespace data = cista::offset;

namespace variant_test {
enum entry_idx_t {
  A_DEFAULT_CTOR,
  A_COPY_CTOR,
  A_MOVE_CTOR,
  A_MOVE_ASSIGN,
  A_COPY_ASSIGN,
  A_DTOR,
  B_DEFAULT_CTOR,
  B_COPY_CTOR,
  B_MOVE_CTOR,
  B_MOVE_ASSIGN,
  B_COPY_ASSIGN,
  B_DTOR,
  NUM_ENTRIES
};
static std::array<int, NUM_ENTRIES> entries;

struct a {
  a() { ++entries[A_DEFAULT_CTOR]; }
  a(a const&) { ++entries[A_COPY_CTOR]; }
  a(a&&) { ++entries[A_MOVE_CTOR]; }
  a& operator=(a&&) {
    ++entries[A_MOVE_ASSIGN];
    return *this;
  }
  a& operator=(a const&) {
    ++entries[A_COPY_ASSIGN];
    return *this;
  }
  ~a() { ++entries[A_DTOR]; }
};

struct b {
  b() { ++entries[B_DEFAULT_CTOR]; }
  b(b const&) { ++entries[B_COPY_CTOR]; }
  b(b&&) { ++entries[B_MOVE_CTOR]; }
  b& operator=(b&&) {
    ++entries[B_MOVE_ASSIGN];
    return *this;
  }
  b& operator=(b const&) {
    ++entries[B_COPY_ASSIGN];
    return *this;
  }
  ~b() { ++entries[B_DTOR]; }
};

}  // namespace variant_test

using namespace variant_test;

TEST_CASE("variant basic methods") {
  data::variant<a, b> v{a{}}, u{b{}};

  CHECK(entries == std::array<int, NUM_ENTRIES>{1, 0, 1, 0, 0, 1,  //
                                                1, 0, 1, 0, 0, 1});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, a>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 0,  //
                                                0, 0, 0, 0, 0, 0});
  v = b{};
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 1,  //
                                                1, 0, 1, 0, 0, 1});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, b>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 0,  //
                                                0, 0, 0, 0, 0, 0});

  v = a{};
  CHECK(entries == std::array<int, NUM_ENTRIES>{1, 0, 1, 0, 0, 1,  //
                                                0, 0, 0, 0, 0, 1});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, a>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 0,  //
                                                0, 0, 0, 0, 0, 0});

  v = u;
  v.apply(
      [](auto&& e) { CHECK(std::is_same_v<std::decay_t<decltype(e)>, b>); });
  CHECK(entries == std::array<int, NUM_ENTRIES>{0, 0, 0, 0, 0, 1,  //
                                                0, 1, 0, 0, 0, 0});
  entries = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

TEST_CASE("variant comparison") {
  data::variant<int, float> v{1}, u{0.5f};
  CHECK(u > v);
}