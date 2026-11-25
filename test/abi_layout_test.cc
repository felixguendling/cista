#include <cstddef>
#include <cstdint>
#include <array>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/bitset.h"
#include "cista/containers/bitvec.h"
#include "cista/containers/cstring.h"
#include "cista/containers/hash_map.h"
#include "cista/containers/hash_set.h"
#include "cista/containers/mutable_fws_multimap.h"
#include "cista/containers/optional.h"
#include "cista/containers/paged.h"
#include "cista/containers/pair.h"
#include "cista/containers/rtree.h"
#include "cista/containers/string.h"
#include "cista/containers/tuple.h"
#include "cista/containers/unique_ptr.h"
#include "cista/containers/variant.h"
#include "cista/containers/vector.h"
#include "cista/containers/vecvec.h"
#include "cista/serialization.h"
#include "cista/strong.h"
#endif

namespace abi_layout_test {

namespace data = cista::offset;

struct tag {};

using ptr = data::ptr<int>;
using vector_i64 = data::vector<std::int64_t>;
using vector_f64 = data::vector<double>;
using vector_u8 = data::vector<std::uint8_t>;
using string = data::string;
using cstring = data::cstring;
using unique_ptr = data::unique_ptr<std::int64_t>;
using hash_map = data::hash_map<std::int32_t, std::int64_t>;
using hash_set = data::hash_set<std::int64_t>;
using optional_i64 = cista::optional<std::int64_t>;
using optional_f64 = cista::optional<double>;
using optional_array = cista::optional<std::array<double, 2>>;
using variant_u8_i64 = data::variant<std::uint8_t, std::int64_t>;
using variant_i32_f64 = data::variant<std::int32_t, double>;
using tuple = cista::tuple<std::uint8_t, std::int64_t, double>;
using pair_i32_i64 = cista::pair<std::int32_t, std::int64_t>;
using pair_u8_f64 = cista::pair<std::uint8_t, double>;
using strong = cista::strong<std::int64_t, tag>;
using bitset = cista::bitset<100>;
using bitvec = data::bitvec;
using multimap = data::mutable_fws_multimap<std::uint64_t, std::int32_t>;
using multimap_index = multimap::index_type;
using page = cista::page<std::uint64_t, std::uint16_t>;
using rtree = data::rtree<std::int64_t, 2U, double>;
using rtree_rect = rtree::rect;
using rtree_node = rtree::node;
using vecvec = data::vecvec<std::uint32_t, std::int64_t>;

template <typename T, std::size_t Size, std::size_t Align>
constexpr bool has_64bit_layout() {
  return sizeof(cista::offset_t) != 8U ||
         (sizeof(T) == Size && alignof(T) == Align);
}

static_assert(has_64bit_layout<ptr, 8U, 8U>(), "ptr");
static_assert(has_64bit_layout<vector_i64, 24U, 8U>(), "vector_i64");
static_assert(has_64bit_layout<vector_f64, 24U, 8U>(), "vector_f64");
static_assert(has_64bit_layout<vector_u8, 24U, 8U>(), "vector_u8");
static_assert(has_64bit_layout<string, 16U, 8U>(), "string");
static_assert(has_64bit_layout<cstring, 16U, 8U>(), "cstring");
static_assert(has_64bit_layout<unique_ptr, 16U, 8U>(), "unique_ptr");
static_assert(has_64bit_layout<hash_map, 48U, 8U>(), "hash_map");
static_assert(has_64bit_layout<hash_set, 48U, 8U>(), "hash_set");
static_assert(has_64bit_layout<optional_i64, 16U, 8U>(), "optional_i64");
static_assert(has_64bit_layout<optional_f64, 16U, 8U>(), "optional_f64");
static_assert(has_64bit_layout<optional_array, 24U, 8U>(), "optional_array");
static_assert(has_64bit_layout<variant_u8_i64, 16U, 8U>(), "variant_u8_i64");
static_assert(has_64bit_layout<variant_i32_f64, 16U, 8U>(), "variant_i32_f64");
static_assert(has_64bit_layout<tuple, 24U, 8U>(), "tuple");
static_assert(has_64bit_layout<pair_i32_i64, 16U, 8U>(), "pair_i32_i64");
static_assert(has_64bit_layout<pair_u8_f64, 16U, 8U>(), "pair_u8_f64");
static_assert(has_64bit_layout<strong, 8U, 8U>(), "strong");
static_assert(has_64bit_layout<bitset, 16U, 8U>(), "bitset");
static_assert(has_64bit_layout<bitvec, 32U, 8U>(), "bitvec");
static_assert(has_64bit_layout<multimap, 560U, 8U>(), "multimap");
static_assert(has_64bit_layout<multimap_index, 24U, 8U>(), "multimap_index");
static_assert(has_64bit_layout<page, 16U, 8U>(), "page");
static_assert(has_64bit_layout<rtree, 136U, 8U>(), "rtree");
static_assert(has_64bit_layout<rtree_rect, 32U, 8U>(), "rtree_rect");
static_assert(has_64bit_layout<rtree_node, 2568U, 8U>(), "rtree_node");
static_assert(has_64bit_layout<vecvec, 48U, 8U>(), "vecvec");

static_assert(sizeof(cista::offset_t) != 8U ||
                  (cista::serialized_alignment_v<std::int64_t> == 8U &&
                   cista::serialized_alignment_v<double> == 8U &&
                   cista::serialized_alignment_v<std::array<double, 3>> == 8U),
              "serialized_alignment");

struct user_type {
  std::uint8_t a_;
  alignas(8) std::int64_t b_;
  data::vector<double> c_;
  std::uint16_t d_;
  cista::optional<std::int64_t> e_;
  std::uint32_t f_;
  alignas(8) double g_;
  data::string h_;
  cista::pair<std::uint8_t, std::int64_t> i_;
  bool j_;
  strong k_;
  data::hash_map<std::int32_t, double> l_;
  std::uint8_t m_;
};

template <std::size_t Offset, std::size_t Expected>
constexpr bool at_64bit_offset() {
  return sizeof(cista::offset_t) != 8U || Offset == Expected;
}

static_assert(has_64bit_layout<user_type, 184U, 8U>(), "user_type");
static_assert(at_64bit_offset<offsetof(user_type, a_), 0U>(), "user_type::a_");
static_assert(at_64bit_offset<offsetof(user_type, b_), 8U>(), "user_type::b_");
static_assert(at_64bit_offset<offsetof(user_type, c_), 16U>(), "user_type::c_");
static_assert(at_64bit_offset<offsetof(user_type, d_), 40U>(), "user_type::d_");
static_assert(at_64bit_offset<offsetof(user_type, e_), 48U>(), "user_type::e_");
static_assert(at_64bit_offset<offsetof(user_type, f_), 64U>(), "user_type::f_");
static_assert(at_64bit_offset<offsetof(user_type, g_), 72U>(), "user_type::g_");
static_assert(at_64bit_offset<offsetof(user_type, h_), 80U>(), "user_type::h_");
static_assert(at_64bit_offset<offsetof(user_type, i_), 96U>(), "user_type::i_");
static_assert(at_64bit_offset<offsetof(user_type, j_), 112U>(),
              "user_type::j_");
static_assert(at_64bit_offset<offsetof(user_type, k_), 120U>(),
              "user_type::k_");
static_assert(at_64bit_offset<offsetof(user_type, l_), 128U>(),
              "user_type::l_");
static_assert(at_64bit_offset<offsetof(user_type, m_), 176U>(),
              "user_type::m_");

static_assert(cista::has_serialized_layout<user_type>(), "user_type layout");

struct misaligned_type {
  std::uint8_t a_;
  std::int64_t b_;
};

static_assert(sizeof(cista::offset_t) <= sizeof(void*) ||
                  alignof(std::int64_t) == 8U ||
                  !cista::has_serialized_layout<misaligned_type>(),
              "misaligned_type has to be detected");

TEST_CASE("abi layout: serialize a user type") {
  auto buf = cista::byte_buf{};
  {
    auto u = user_type{};
    u.a_ = 1U;
    u.b_ = -(1LL << 40U);
    u.c_ = {0.5, -1.5};
    u.d_ = 2U;
    u.e_ = std::int64_t{1LL << 41U};
    u.f_ = 3U;
    u.g_ = 2.5;
    u.h_ = "a string that is too long for the short string optimization";
    u.i_ = cista::pair<std::uint8_t, std::int64_t>{4U, -(1LL << 42U)};
    u.j_ = true;
    u.k_ = strong{1LL << 43U};
    u.l_.emplace(5, 6.5);
    u.m_ = 7U;
    buf = cista::serialize(u);
  }

  auto const& u = *cista::deserialize<user_type>(buf);
  CHECK(u.a_ == 1U);
  CHECK(u.b_ == -(1LL << 40U));
  CHECK(u.c_ == data::vector<double>{0.5, -1.5});
  CHECK(u.d_ == 2U);
  CHECK((u.e_.has_value() && *u.e_ == (1LL << 41U)));
  CHECK(u.f_ == 3U);
  CHECK(u.g_ == doctest::Approx(2.5));
  CHECK(u.h_ == "a string that is too long for the short string optimization");
  CHECK((u.i_.first == 4U && u.i_.second == -(1LL << 42U)));
  CHECK(u.j_);
  CHECK(u.k_ == strong{1LL << 43U});
  CHECK(u.l_.at(5) == doctest::Approx(6.5));
  CHECK(u.m_ == 7U);
}

}  // namespace abi_layout_test
