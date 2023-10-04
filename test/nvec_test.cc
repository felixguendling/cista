#include <memory>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/nvec.h"
#include "cista/serialization.h"
#include "cista/targets/buf.h"
#endif

TEST_CASE("nvec test") {
  constexpr auto const kMode =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  struct transfer {
    bool operator==(transfer const& o) const { return o.i_ == i_; }
    int i_;
  };
  using container_t = cista::offset::nvec<std::uint32_t, transfer, 2>;

  cista::byte_buf buf;
  {
    container_t v;

    v.emplace_back(std::vector<std::vector<transfer>>{
        {transfer{1}, transfer{2}}, {transfer{3}, transfer{4}, transfer{5}}});
    v.emplace_back(std::vector<std::vector<transfer>>{
        {transfer{6}, transfer{7}},
        {transfer{8}, transfer{9}, transfer{10}},
        {transfer{11}}});
    v.emplace_back(std::vector<std::vector<transfer>>{
        {transfer{12}, transfer{13}},
        {transfer{14}, transfer{15}, transfer{16}},
        {transfer{17}}});
    buf = cista::serialize<kMode>(v);
  }

  auto const& v = *cista::deserialize<container_t, kMode>(buf);

  auto const all = std::vector<transfer>{
      transfer{1},  transfer{2},  transfer{3},  transfer{4},  transfer{5},
      transfer{6},  transfer{7},  transfer{8},  transfer{9},  transfer{10},
      transfer{11}, transfer{12}, transfer{13}, transfer{14}, transfer{15},
      transfer{16}, transfer{17}};
  auto l = 0U;
  for (auto i = 0U; i != v.size(); ++i) {
    for (auto j = 0U; j != v.size(i); ++j) {
      for (auto k = 0U; k != v.size(i, j); ++k) {
        CHECK_EQ(all[l++], v.at(i, j).at(k));
      }
    }
  }

  l = 0U;
  for (auto const a : v) {
    for (auto const b : a) {
      for (auto const c : b) {
        CHECK_EQ((all[l++]), c);
      }
    }
  }

  {
    auto const bucket = v.at(2U, 1U);
    auto const entries = {transfer{14}, transfer{15}, transfer{16}};
    CHECK(std::equal(begin(bucket), end(bucket), begin(entries), end(entries)));
  }

  {
    auto const bucket = v.at(2U, 2U);
    auto const entries = {transfer{17}};
    CHECK(std::equal(begin(bucket), end(bucket), begin(entries), end(entries)));
  }

  for (auto const& t : v.at(1U, 2U)) {
    CHECK_EQ(t.i_, 11);
  }
  CHECK_EQ(2U, v.size(0U));
  CHECK_EQ(3U, v.size(1U));
  CHECK_EQ(2U, v.size(0U, 0U));
  CHECK_EQ(3U, v.size(0U, 1U));
  CHECK_EQ(2U, v.size(1U, 0U));
  CHECK_EQ(3U, v.size(1U, 1U));
  CHECK_EQ(1U, v.size(1U, 2U));
  CHECK_EQ(2U, v.size(2U, 0U));
  CHECK_EQ(3U, v.size(2U, 1U));
  CHECK_EQ(1U, v.size(2U, 2U));

  CHECK_EQ(2U, v[0U].size());
  CHECK_EQ(3U, v[1U].size());
  CHECK_EQ(2U, v[0U][0U].size());
  CHECK_EQ(3U, v[0U][1U].size());
  CHECK_EQ(2U, v[1U][0U].size());
  CHECK_EQ(3U, v[1U][1U].size());
  CHECK_EQ(1U, v[1U][2U].size());
  CHECK_EQ(2U, v[2U][0U].size());
  CHECK_EQ(3U, v[2U][1U].size());
  CHECK_EQ(1U, v[2U][2U].size());
}