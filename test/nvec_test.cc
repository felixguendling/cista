#include <memory>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/nvec.h"
#endif

TEST_CASE("nvec test") {
  struct transfer {
    bool operator==(transfer const& o) const { return o.i_ == i_; }
    int i_;
  };
  cista::raw::nvec<std::uint32_t, transfer, 2> v;
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

  {
    auto const bucket = v.at(2, 1);
    auto const entries = {transfer{14}, transfer{15}, transfer{16}};
    CHECK(std::equal(begin(bucket), end(bucket), begin(entries), end(entries)));
  }

  {
    auto const bucket = v.at(2, 2);
    auto const entries = {transfer{17}};
    CHECK(std::equal(begin(bucket), end(bucket), begin(entries), end(entries)));
  }

  for (auto const& t : v.at(1, 2)) {
    CHECK_EQ(t.i_, 11);
  }
  CHECK_EQ(2, v.size(0));
  CHECK_EQ(3, v.size(1));
  CHECK_EQ(2, v.size(0, 0));
  CHECK_EQ(3, v.size(0, 1));
  CHECK_EQ(2, v.size(1, 0));
  CHECK_EQ(3, v.size(1, 1));
  CHECK_EQ(1, v.size(1, 2));
  CHECK_EQ(2, v.size(2, 0));
  CHECK_EQ(3, v.size(2, 1));
  CHECK_EQ(1, v.size(2, 2));
}