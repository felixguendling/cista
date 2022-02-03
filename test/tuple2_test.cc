#include <string>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace data = cista::offset;

TEST_SUITE("tuple2") {
  TEST_CASE("tuple2 get") {
    auto t = cista::tuple2{1, .1, 'a'};

    cista::get<2>(t) = 'B';
    CHECK(cista::get<0>(t) == 1);
    CHECK(cista::get<1>(t) == .1);
    CHECK(cista::get<2>(t) == 'B');
    CHECK(cista::tuple_size_v<decltype(t)> == 3U);
    cista::apply([](auto&... f) { ((f = {}), ...); }, t);
    CHECK(cista::get<0>(t) == 0);
    CHECK(cista::get<1>(t) == 0);
    CHECK(cista::get<2>(t) == '\0');
  }

  TEST_CASE("tuple2 get with same types") {
    auto t = cista::tuple2{1, 42, 1336, 11};

    cista::get<2>(t) += 1;

    CHECK(cista::get<0>(t) == 1);
    CHECK(cista::get<1>(t) == 42);
    CHECK(cista::get<2>(t) == 1337);
    CHECK(cista::get<3>(t) == 11);
    CHECK(cista::tuple_size_v<decltype(t)> == 4U);
    cista::apply([](auto&... f) { ((f = {}), ...); }, t);
    CHECK(cista::get<0>(t) == 0);
    CHECK(cista::get<1>(t) == 0);
    CHECK(cista::get<2>(t) == 0);
    CHECK(cista::get<3>(t) == 0);
  }

  TEST_CASE("tuple2 structured bindings") {
    auto t = cista::tuple2{1, 42, 1336, 11};
    auto& [t0, t1, t2, t3] = t;
    t2 += 1;
    CHECK(t0 == 1);
    CHECK(t1 == 42);
    CHECK(t2 == 1337);
    CHECK(t3 == 11);
  }

  TEST_CASE("tuple2 comparison") {
    auto a = cista::tuple2{2, 'a'};
    auto b = cista::tuple2{1, 'b'};
    CHECK(!(a == b));
    CHECK(a != b);
    CHECK(b < a);
    CHECK(b <= a);
    CHECK(a > b);
    CHECK(!(b >= a));
  }

  TEST_CASE("type_at_position") {  // NOLINT
    static_assert(std::is_same_v<
                  uint32_t,
                  cista::type_at_position_t<
                      0, cista::tuple2<uint32_t, uint64_t, float, double>>>);
    static_assert(std::is_same_v<
                  uint64_t,
                  cista::type_at_position_t<
                      1, cista::tuple2<uint32_t, uint64_t, float, double>>>);
    static_assert(
        std::is_same_v<
            float, cista::type_at_position_t<
                       2, cista::tuple2<uint32_t, uint64_t, float, double>>>);
    static_assert(
        std::is_same_v<
            double, cista::type_at_position_t<
                        3, cista::tuple2<uint32_t, uint64_t, float, double>>>);
  }

  TEST_CASE("tuple2 - simple get") {  // NOLINT
    cista::tuple2<uint32_t, uint64_t, float, double> t = {42, 1082342034, 0.42F,
                                                          0.001337};

    CHECK(cista::get<0>(t) == 42);
    CHECK(cista::get<1>(t) == 1082342034);
    CHECK(cista::get<2>(t) == 0.42F);
    CHECK(cista::get<3>(t) == 0.001337);
  }

  TEST_CASE("tuple2 - simple get max") {  // NOLINT
    cista::tuple2<uint32_t, uint64_t, float, double> t2 = {
        std::numeric_limits<uint32_t>::max(),
        std::numeric_limits<uint64_t>::max(), std::numeric_limits<float>::max(),
        std::numeric_limits<double>::max()};

    CHECK(cista::get<0>(t2) == std::numeric_limits<uint32_t>::max());
    CHECK(cista::get<1>(t2) == std::numeric_limits<uint64_t>::max());
    CHECK(cista::get<2>(t2) == std::numeric_limits<float>::max());
    CHECK(cista::get<3>(t2) == std::numeric_limits<double>::max());
  }

  TEST_CASE("tuple2 - container get with preceding type") {  // NOLINT
    cista::tuple2<std::size_t, data::vector<float>> t = {100,
                                                         {0.1F, 0.2F, 10.11F}};

    auto& vec = cista::get<1>(t);
    CHECK(vec.front() == 0.1F);
    CHECK(vec[1] == 0.2F);
    CHECK(vec.back() == 10.11F);

    CHECK(cista::get<0>(t) == 100);
  }

  TEST_CASE("tuple2 - container get with succeeding type") {  // NOLINT
    cista::tuple2<data::vector<float>, std::size_t> t = {{0.1F, 0.2F, 10.11F},
                                                         100};

    auto& vec = cista::get<0>(t);
    CHECK(vec.front() == 0.1F);
    CHECK(vec[1] == 0.2F);
    CHECK(vec.back() == 10.11F);
    CHECK(cista::get<1>(t) == 100);
  }

  TEST_CASE("tuple2 - container get prec and suc type") {  // NOLINT
    cista::tuple2<std::size_t, data::vector<float>, std::size_t> t = {
        10, {0.1F, 0.2F, 10.11F}, 1000};

    CHECK(cista::get<0>(t) == 10);
    CHECK(cista::get<1>(t).front() == 0.1F);
    CHECK(cista::get<1>(t)[1] == 0.2F);
    CHECK(cista::get<1>(t).back() == 10.11F);
    CHECK(cista::get<2>(t) == 1000);
  }

  TEST_CASE("tuple2 - serialize primitives") {  // NOLINT
    using serialize_me = cista::tuple2<uint32_t, uint64_t, float, double>;

    std::vector<unsigned char> buf;
    {
      serialize_me t = {42, 1082342034, 0.42F, 0.001337};
      buf = cista::serialize(t);
    }

    serialize_me d_t = *cista::deserialize<serialize_me>(buf);

    cista::get<0>(d_t) += 1;

    CHECK(cista::get<0>(d_t) == 43);
    CHECK(cista::get<1>(d_t) == 1082342034);
    CHECK(cista::get<2>(d_t) == 0.42F);
    CHECK(cista::get<3>(d_t) == 0.001337);
  }

  TEST_CASE("tuple2 - serialize container") {  // NOLINT
    std::vector<unsigned char> buf;

    using serialize_me =
        cista::tuple2<std::size_t, data::vector<float>, std::size_t>;

    {
      serialize_me t = {10, {0.1F, 0.2F, 10.11F}, 1000};
      buf = cista::serialize(t);
    }
    auto d_t = *cista::deserialize<serialize_me>(buf);

    CHECK(cista::get<0>(d_t) == 10);
    CHECK(cista::get<1>(d_t).front() == 0.1F);
    CHECK(cista::get<1>(d_t)[1] == 0.2F);
    CHECK(cista::get<1>(d_t).back() == 10.11F);
    CHECK(cista::get<2>(d_t) == 1000);
  }

  TEST_CASE("serialize struct with tuple") {  // NOLINT
    struct s {
      cista::tuple2<int, data::vector<uint32_t>, int> t_;
    };

    std::vector<unsigned char> buf;

    {
      s s;
      s.t_ = {10, {1, 2, 3}, 1000};

      buf = cista::serialize(s);
    }

    auto d_s = *cista::deserialize<struct s>(buf);

    cista::get<1>(d_s.t_).push_back(4);

    CHECK(cista::get<0>(d_s.t_) == 10);
    CHECK(cista::get<1>(d_s.t_).front() == 1);
    CHECK(cista::get<1>(d_s.t_)[1] == 2);
    CHECK(cista::get<1>(d_s.t_)[2] == 3);
    CHECK(cista::get<1>(d_s.t_).back() == 4);
    CHECK(cista::get<2>(d_s.t_) == 1000);
  }

  TEST_CASE("tuple serialize") {
    namespace data = cista::offset;
    using serialize_me = cista::tuple2<
        data::vector<cista::tuple2<data::string, data::hash_set<data::string>>>,
        int>;
    constexpr auto const MODE = cista::mode::WITH_VERSION;

    cista::byte_buf buf;
    {
      serialize_me obj;
      std::get<0>(obj).emplace_back(cista::tuple2{
          data::string{"hello"},
          data::hash_set<data::string>{
              data::string{"1"},
              data::string{"this is a very very very long string"},
              data::string{"3"}}});
      std::get<0>(obj).emplace_back(cista::tuple2{
          data::string{"world"},
          data::hash_set<data::string>{
              data::string{"4"},
              data::string{"this is a very very very long string"},
              data::string{"6"}}});
      std::get<1>(obj) = 55;
      buf = cista::serialize<MODE>(obj);
    }  // EOL obj

    auto const& serialized =
        *cista::unchecked_deserialize<serialize_me, MODE>(buf);
    CHECK(
        std::get<0>(serialized).at(0) ==
        cista::tuple2{data::string{"hello"},
                      data::hash_set<data::string>{
                          data::string{"1"},
                          data::string{"this is a very very very long string"},
                          data::string{"3"}}});
    CHECK(
        std::get<0>(serialized).at(1) ==
        cista::tuple2{data::string{"world"},
                      data::hash_set<data::string>{
                          data::string{"4"},
                          data::string{"this is a very very very long string"},
                          data::string{"6"}}});
    CHECK(std::get<1>(serialized) == 55);
  }

  template <typename... Ts>
  void check_size() {
    static_assert(sizeof(std::tuple<Ts...>) == sizeof(cista::tuple2<Ts...>));
  }

  TEST_CASE("size check") {
    using pixbuf = std::array<uint32_t, 1920 * 1080>;
    enum class error_code { success, fail };

    check_size<pixbuf, error_code>();
    check_size<uint16_t, uint32_t, uint64_t>();
    check_size<uint32_t, uint64_t, uint16_t>();
    check_size<uint16_t, uint32_t, uint16_t, uint64_t>();
    check_size<uint16_t, uint32_t, uint16_t, uint16_t, uint64_t>();
    check_size<uint16_t, uint32_t, uint8_t, uint8_t, uint8_t, uint64_t>();
    check_size<uint64_t, uint32_t, float, uint8_t, uint8_t, bool, double,
               uint64_t, uint8_t>();
  }
}