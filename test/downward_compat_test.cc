#include "doctest.h"

#include "cista.h"

namespace cista {
using namespace cista::raw;
}  // namespace cista

struct v1 {
  cista::string s_;
  int dummy_for_msvc_{0};
};

struct v2 {
  cista::string s1_, s2_;
  double arr_;
};

struct data_v1 {
  int version_{1};
  cista::vector<cista::unique_ptr<v1>> values_;
};

struct data_v2 {
  int version_{2};
  cista::vector<cista::unique_ptr<v2>> values_;
};

struct version_detection {
  int version_{0};
  int dummy_for_msvc_{0};
};

TEST_CASE("downward compatibility test") {
  for (auto const i : {1, 2}) {
    std::vector<uint8_t> buf;
    switch (i) {
      case 1: {
        data_v1 values;
        values.values_.emplace_back(
            cista::make_unique<v1>(v1{cista::string{"A"}}));
        values.values_.emplace_back(
            cista::make_unique<v1>(v1{cista::string{"B"}}));
        values.values_.emplace_back(
            cista::make_unique<v1>(v1{cista::string{"C"}}));
        buf = cista::serialize(values);
        break;
      }

      case 2: {
        data_v2 values;
        values.values_.emplace_back(
            cista::make_unique<v2>(v2{cista::string{"A"}}));
        values.values_.emplace_back(
            cista::make_unique<v2>(v2{cista::string{"B"}}));
        values.values_.emplace_back(
            cista::make_unique<v2>(v2{cista::string{"C"}}));
        buf = cista::serialize(values);
        break;
      }

      default:;
    }

    if (cista::unchecked_deserialize<version_detection>(buf)->version_ == 1) {
      auto const v1 = cista::unchecked_deserialize<data_v1>(buf);
      CHECK(v1->values_[0]->s_ == "A");
      CHECK(v1->values_[1]->s_ == "B");
      CHECK(v1->values_[2]->s_ == "C");
    } else if (cista::unchecked_deserialize<version_detection>(buf)->version_ ==
               2) {
      auto const v2 = cista::unchecked_deserialize<data_v2>(buf);
      CHECK(v2->values_[0]->s1_ == "A");
      CHECK(v2->values_[1]->s1_ == "B");
      CHECK(v2->values_[2]->s1_ == "C");
    }
  }
}