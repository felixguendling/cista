#include <memory>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/vector.h"
#include "cista/equal_to.h"
#include "cista/is_iterable.h"
#include "cista/reflection/printable.h"
#include "cista/serialization.h"
#include "cista/type_hash/type_name.h"
#include "cista/verify.h"
#endif

TEST_CASE("insert begin test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(begin(v), 0);

  CHECK(v == vector<int>{0, 1, 2, 3});
}

TEST_CASE("insert end test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(end(v), 4);

  CHECK(v == vector<int>{1, 2, 3, 4});
}

TEST_CASE("insert middle test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  v.insert(begin(v) + 1, 4);

  CHECK(v == vector<int>{1, 4, 2, 3});
}

TEST_CASE("range insert begin test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{8, 9};
  v.insert(begin(v), begin(w), end(w));

  CHECK(v == vector<int>{8, 9, 1, 2, 3});
}

TEST_CASE("range insert end test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{8, 9};
  v.insert(end(v), begin(w), end(w));

  CHECK(v == vector<int>{1, 2, 3, 8, 9});
}

TEST_CASE("range insert middle test") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{8, 9};
  v.insert(begin(v) + 1, begin(w), end(w));

  CHECK(v == vector<int>{1, 8, 9, 2, 3});
}

TEST_CASE("range insert nothing") {
  using cista::raw::vector;

  auto v = vector<int>{1, 2, 3};
  auto const w = vector<int>{};
  v.insert(begin(v) + 1, begin(w), end(w));

  CHECK(v == vector<int>{1, 2, 3});
}

struct move_only_int {
  explicit move_only_int(int i) : i_{i} {}
  move_only_int(move_only_int const&) = delete;
  move_only_int(move_only_int&& o) = default;
  move_only_int& operator=(move_only_int const&) = delete;
  move_only_int& operator=(move_only_int&& o) = default;
  ~move_only_int() = default;

  friend bool operator==(move_only_int const& lhs, move_only_int const& rhs) {
    return lhs.i_ == rhs.i_;
  }

  friend std::ostream& operator<<(std::ostream& out, move_only_int const& e) {
    out << e.i_;
    return out;
  }

  int i_;
};

cista::raw::vector<move_only_int> make_move_only_int_vector(
    std::initializer_list<int> init) {
  cista::raw::vector<move_only_int> v;
  for (auto i : init) {
    v.emplace_back(i);
  }
  return v;
}

TEST_CASE("range insert middle move only test") {
  auto v = make_move_only_int_vector({1, 2, 3});
  auto w = make_move_only_int_vector({8, 9});

  v.insert(begin(v) + 1, std::make_move_iterator(begin(w)),
           std::make_move_iterator(end(w)));

  CHECK(v == make_move_only_int_vector({1, 8, 9, 2, 3}));
}

struct input_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = move_only_int;
  using difference_type = int;
  using pointer = move_only_int*;
  using reference = move_only_int&;

  explicit input_iterator(int i) : i_{i}, consumed_{std::make_shared<int>(i)} {}

  input_iterator& operator++() {
    cista::verify(i_ == *consumed_, "input_iterator: iterate a stale copy");
    *consumed_ = ++i_;
    return *this;
  }

  move_only_int operator*() const {
    cista::verify(i_ == *consumed_, "input_iterator: dereference a stale copy");
    return move_only_int{i_};
  }

  friend bool operator==(input_iterator const& lhs, input_iterator const& rhs) {
    return lhs.i_ == rhs.i_;
  }

  int i_;
  std::shared_ptr<int> consumed_;
};

TEST_CASE("input iterator works") {
  input_iterator a{42};
  auto cpy = a;
  ++a;
  REQUIRE_THROWS(++cpy);
  REQUIRE_THROWS(*cpy);
}

TEST_CASE("range insert input_iterator begin test") {
  auto v = make_move_only_int_vector({1, 2, 3});
  v.insert(begin(v), input_iterator{8}, input_iterator{10});

  CHECK(v == make_move_only_int_vector({8, 9, 1, 2, 3}));
}

TEST_CASE("range insert input_iterator end test") {
  auto v = make_move_only_int_vector({1, 2, 3});
  v.insert(end(v), input_iterator{8}, input_iterator{10});

  CHECK(v == make_move_only_int_vector({1, 2, 3, 8, 9}));
}

TEST_CASE("range insert input_iterator middle test") {
  auto v = make_move_only_int_vector({1, 2, 3});
  v.insert(begin(v) + 1, input_iterator{8}, input_iterator{10});

  CHECK(v == make_move_only_int_vector({1, 8, 9, 2, 3}));
}

TEST_CASE("range insert input_iterator nothing") {
  auto v = make_move_only_int_vector({1, 2, 3});
  v.insert(begin(v) + 1, input_iterator{0}, input_iterator{0});

  CHECK(v == make_move_only_int_vector({1, 2, 3}));
}

TEST_CASE("erase duplicates") {
  using cista::raw::vector;

  auto v = vector<int>{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 9};
  std::sort(begin(v), end(v));
  v.erase(std::unique(begin(v), end(v)), end(v));

  CHECK(v == vector<int>{1, 2, 3, 4, 5, 6, 9});
}

TEST_CASE("iterable comparison") {
  std::vector<int> std_v{1, 2, 3};
  cista::raw::vector<int> cista_v{1, 2, 3};
  std::set<int> std_s{1, 2, 3};
  CHECK(cista::equal_to<decltype(std_v)>{}(std_v, cista_v));
  CHECK(cista::equal_to<decltype(std_s)>{}(std_s, cista_v));
}

TEST_CASE("to_vec") {
  namespace data = cista::raw;

  cista::buf<std::vector<uint8_t>> buf;
  for (auto i = 0U; i != 3; ++i) {  // Serialize.
    buf.reset();

    auto const v = std::vector<double>({1.0, 2.0});
    auto o = data::to_vec(v);
    CHECK(o == data::to_vec(v, [](auto&& e) { return e; }));
    CHECK(o == data::to_vec(begin(v), end(v), [](auto&& e) { return e; }));
    cista::serialize(buf, o);
  }

  // Deserialize.
  std::string s{reinterpret_cast<char*>(&buf.buf_[0]), buf.buf_.size()};
  auto const& deserialized =
      *cista::deserialize<data::vector<double>>(&s[0], &s[0] + s.size());
  REQUIRE(deserialized.size() == 2);
  CHECK(deserialized[0] == 1.0);
  CHECK(deserialized[1] == 2.0);
}

TEST_CASE("to_indexed_vec") {
  namespace data = cista::raw;

  std::vector<unsigned char> buf;
  {  // Serialize.
    auto const v = std::vector<double>({1.0, 2.0});
    auto o = data::to_indexed_vec(v);
    CHECK(o == data::to_indexed_vec(v, [](auto&& e) { return e; }));
    CHECK(o ==
          data::to_indexed_vec(begin(v), end(v), [](auto&& e) { return e; }));
    buf = cista::serialize(o);
  }

  // Deserialize.
  auto const& deserialized =
      *cista::deserialize<data::indexed_vector<double>>(buf);
  REQUIRE(deserialized.size() == 2);
  CHECK(deserialized[0] == 1.0);
  CHECK(deserialized[1] == 2.0);
}

TEST_CASE("complex_type_insert") {
  namespace data = cista::raw;

  auto v = data::vector<data::vector<data::string>>{
      {"abc", "def", "longlonglonglonglonglonglonglonglonglonglongstring"},
      {"uvw", "xyz", "longlonglonglonglonglonglonglonglonglonglongstring"}};
  auto u = v;
  for (int i = 0; i < 10; ++i) {
    auto const v_copy = v;
    v.insert(begin(v), begin(u), end(u));
    u.insert(begin(u), begin(v_copy), end(v_copy));
  }
  REQUIRE(u.size() == v.size());
  REQUIRE(u.size() == 2048);
}

TEST_CASE("erase then serialize") {
  using cista::raw::vector;

  std::vector<uint8_t> buf;
  {
    auto v = vector<int>{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 9};
    v.erase(std::remove_if(begin(v), end(v), [](auto&& i) { return i > 5; }),
            end(v));

    buf = cista::serialize(v);
  }

  auto deserialized = cista::deserialize<vector<int>>(buf);

  CHECK(*deserialized == vector<int>{3, 1, 4, 1, 5, 2, 5, 3, 5});
}

TEST_CASE("erase until empty then serialize") {
  using cista::raw::vector;

  std::vector<uint8_t> buf;

  {
    auto v = vector<int>{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 9};
    v.erase(std::remove_if(begin(v), end(v), [](auto&& i) { return i != 10; }),
            end(v));

    buf = cista::serialize(v);
  }

  auto deserialized = cista::deserialize<vector<int>>(buf);

  CHECK(*deserialized == vector<int>{});
}
