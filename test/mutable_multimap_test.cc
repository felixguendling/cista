#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/mutable_fws_multimap.h"
#include "cista/reflection/comparable.h"
#include "cista/verify.h"
#endif

template <typename K, typename V>
using mutable_fws_multimap = cista::offset::mutable_fws_multimap<K, V>;

namespace utl {

template <typename Container, typename Element>
void erase(Container& c, Element elem) {
  c.erase(std::remove(begin(c), end(c), elem), end(c));
}

template <typename Container, typename Element>
void erase(Container&& c, Element elem) {
  using std::begin;
  using std::end;
  c.erase(std::remove(begin(c), end(c), elem), end(c));
}

template <typename Container, typename Predicate>
void erase_if(Container&& c, Predicate&& pred) {
  using std::begin;
  using std::end;
  c.erase(std::remove_if(begin(c), end(c), std::forward<Predicate>(pred)),
          end(c));
}

}  // namespace utl

struct test_edge {
  CISTA_COMPARABLE()

  std::uint32_t from_{};
  std::uint32_t to_{};
  std::uint32_t weight_{};
};

template <typename T1, typename T2>
bool ElementsAreArray(T1 const& t1, std::initializer_list<T2> t2) {
  CHECK(t1.size() == t2.size());
  auto it1 = begin(t1);
  auto it2 = begin(t2);
  auto end1 = end(t1);
  auto end2 = end(t2);
  for (; it1 != end1 && it2 != end2; ++it1, ++it2) {
    CHECK(*it1 == *it2);
  }
  return true;
}

std::ostream& operator<<(std::ostream& out, test_edge const& e) {
  return out << "{from=" << e.from_ << ", to=" << e.to_
             << ", weight=" << e.weight_ << "}";
}

mutable_fws_multimap<unsigned, int> build_test_map_1() {
  mutable_fws_multimap<unsigned, int> mm;

  mm[0].push_back(4);
  mm[0].push_back(8);

  mm[1].push_back(15);
  mm[1].push_back(16);
  mm[1].push_back(23);
  mm[1].push_back(42);

  mm[2].push_back(100);
  mm[2].push_back(200);
  mm[2].push_back(250);
  mm[2].push_back(300);
  mm[2].push_back(350);
  mm[2].push_back(400);

  return mm;
}

TEST_CASE("mutable_fws_multimap_test, int_1") {
  mutable_fws_multimap<unsigned, int> mm;

  REQUIRE(0 == mm.element_count());
  REQUIRE(0 == mm.size());

  mm[0].push_back(42);
  REQUIRE(1 == mm.element_count());
  REQUIRE(1 == mm.size());
  CHECK(ElementsAreArray(mm[0], {42}));
  CHECK(1 == mm[0].size());

  mm[1].push_back(4);
  REQUIRE(2 == mm.element_count());
  REQUIRE(2 == mm.size());
  CHECK(ElementsAreArray(mm[0], {42}));
  CHECK(1 == mm[0].size());
  CHECK(ElementsAreArray(mm[1], {4}));
  CHECK(1 == mm[1].size());

  mm[1].push_back(8);
  REQUIRE(3 == mm.element_count());
  REQUIRE(2 == mm.size());
  CHECK(ElementsAreArray(mm[0], {42}));
  CHECK(1 == mm[0].size());
  CHECK(ElementsAreArray(mm[1], {4, 8}));
  CHECK(2 == mm[1].size());

  mm[1].emplace_back(15);
  REQUIRE(4 == mm.element_count());
  REQUIRE(2 == mm.size());
  CHECK(ElementsAreArray(mm[0], {42}));
  CHECK(1 == mm[0].size());
  CHECK(ElementsAreArray(mm[1], {4, 8, 15}));
  CHECK(3 == mm[1].size());

  mm[1].push_back(16);
  REQUIRE(5 == mm.element_count());
  REQUIRE(2 == mm.size());
  CHECK(ElementsAreArray(mm[0], {42}));
  CHECK(1 == mm[0].size());
  CHECK(ElementsAreArray(mm[1], {4, 8, 15, 16}));
  CHECK(4 == mm[1].size());

  mm[0].push_back(23);
  REQUIRE(6 == mm.element_count());
  REQUIRE(2 == mm.size());
  CHECK(ElementsAreArray(mm[0], {42, 23}));
  CHECK(2 == mm[0].size());
  CHECK(ElementsAreArray(mm[1], {4, 8, 15, 16}));
  CHECK(4 == mm[1].size());
}

TEST_CASE("mutable_fws_multimap_test, graph_1") {
  mutable_fws_multimap<unsigned, test_edge> mm;

  mm[0].emplace_back(0U, 1U, 10U);
  mm[1].emplace_back(1U, 2U, 20U);
  mm[1].emplace_back(1U, 3U, 30U);
  mm[3].emplace_back(3U, 0U, 50U);
  mm[2].emplace_back(2U, 3U, 5U);

  REQUIRE(4 == mm.size());
  CHECK(5 == mm.element_count());

  CHECK(ElementsAreArray(mm[0], {test_edge{0U, 1U, 10U}}));
  CHECK(ElementsAreArray(mm[1],
                         {test_edge{1U, 2U, 20U}, test_edge{1U, 3U, 30U}}));
  CHECK(ElementsAreArray(mm[2], {test_edge{2U, 3U, 5U}}));
  CHECK(ElementsAreArray(mm[3], {test_edge{3U, 0U, 50U}}));
}

TEST_CASE("mutable_fws_multimap_test, int_2") {
  auto const mm = build_test_map_1();

  REQUIRE(3 == mm.size());
  CHECK(12 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));

  CHECK(ElementsAreArray(mm.front(), {4, 8}));
  CHECK(ElementsAreArray(mm.back(), {100, 200, 250, 300, 350, 400}));
  CHECK(15 == mm[1].front());
  CHECK(42 == mm[1].back());
}

TEST_CASE("mutable_fws_multimap_test, int_insert_1") {
  auto mm = build_test_map_1();

  mm[1].insert(std::next(mm[1].begin(), 2), 20);

  REQUIRE(3 == mm.size());
  CHECK(13 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 20, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));
}

TEST_CASE("mutable_fws_multimap_test, int_insert_2") {
  auto mm = build_test_map_1();

  auto const val = 20;
  mm[1].insert(std::next(mm[1].begin(), 2), val);

  REQUIRE(3 == mm.size());
  CHECK(13 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 20, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));
}

TEST_CASE("mutable_fws_multimap_test, int_erase_1") {
  auto mm = build_test_map_1();

  utl::erase(mm[1], 16);

  REQUIRE(3 == mm.size());
  CHECK(11 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));

  utl::erase(mm[2], 100);

  REQUIRE(3 == mm.size());
  CHECK(10 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {200, 250, 300, 350, 400}));

  utl::erase(mm[2], 400);

  REQUIRE(3 == mm.size());
  CHECK(9 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {200, 250, 300, 350}));

  utl::erase(mm[2], 250);

  REQUIRE(3 == mm.size());
  CHECK(8 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {200, 300, 350}));

  utl::erase(mm[1], 404);

  REQUIRE(3 == mm.size());
  CHECK(8 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {200, 300, 350}));
}

TEST_CASE("mutable_fws_multimap_test, int_erase_2") {
  auto mm = build_test_map_1();

  utl::erase_if(mm[2], [](int e) { return e % 100 == 0; });

  REQUIRE(3 == mm.size());
  CHECK(8 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {250, 350}));
}

TEST_CASE("mutable_fws_multimap_test, int_resize_1") {
  auto mm = build_test_map_1();

  mm[0].resize(4);

  REQUIRE(3 == mm.size());
  CHECK(14 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8, 0, 0}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));

  mm[1].resize(3);

  REQUIRE(3 == mm.size());
  CHECK(13 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8, 0, 0}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));

  mm[1].resize(6, 123);

  REQUIRE(3 == mm.size());
  CHECK(16 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8, 0, 0}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 123, 123, 123}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));
}

TEST_CASE("mutable_fws_multimap_test, pop_back_1") {
  auto mm = build_test_map_1();

  mm[2].pop_back();

  REQUIRE(3 == mm.size());
  CHECK(11 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350}));

  mm[1].pop_back();

  REQUIRE(3 == mm.size());
  CHECK(10 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350}));

  mm[0].pop_back();

  REQUIRE(3 == mm.size());
  CHECK(9 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350}));

  mm[0].pop_back();

  REQUIRE(3 == mm.size());
  CHECK(8 == mm.element_count());
  CHECK(mm[0].empty());
  CHECK(ElementsAreArray(mm[1], {15, 16, 23}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350}));
}

TEST_CASE("mutable_fws_multimap_test, clear_1") {
  auto mm = build_test_map_1();

  mm[0].clear();

  REQUIRE(3 == mm.size());
  CHECK(10 == mm.element_count());
  CHECK(mm[0].empty());
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 42}));
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));
}

TEST_CASE("mutable_fws_multimap_test, clear_2") {
  auto mm = build_test_map_1();

  mm[1].clear();

  REQUIRE(3 == mm.size());
  CHECK(8 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(mm[1].empty());
  CHECK(ElementsAreArray(mm[2], {100, 200, 250, 300, 350, 400}));
}

TEST_CASE("mutable_fws_multimap_test, clear_3") {
  auto mm = build_test_map_1();

  mm[2].clear();

  REQUIRE(3 == mm.size());
  CHECK(6 == mm.element_count());
  CHECK(ElementsAreArray(mm[0], {4, 8}));
  CHECK(ElementsAreArray(mm[1], {15, 16, 23, 42}));
  CHECK(mm[2].empty());
}

template <typename T>
T get_order_loop(T const size) {
  for (auto order = T{0U}, value = T{1}; order < static_cast<T>(sizeof(T) * 8);
       ++order, value = value << 1) {
    if (value == size) {
      return order;
    }
  }
  cista::verify(false, "mutable_fws_multimap::get_order: not found for");
  return T{};
}

TEST_CASE("mutable_fws_multimap_test, get_order_loop") {
  for (std::uint16_t i = 0U; i < 16; ++i) {
    CHECK(get_order_loop(static_cast<std::uint16_t>(1U) << i) == i);
  }

  for (std::uint32_t i = 0U; i < 32; ++i) {
    CHECK(get_order_loop(1U << i) == i);
  }

  for (std::uint64_t i = 0ULL; i < 64; ++i) {
    CHECK(get_order_loop(1ULL << i) == i);
  }
}
