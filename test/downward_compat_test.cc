#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace data = cista::raw;

struct v1 {
  data::string s_;
};

struct v2 {
  data::string s1_, s2_;
  double arr_;
};

struct data_v1 {
  int version_{1};
  data::vector<data::unique_ptr<v1>> values_;
};

struct data_v2 {
  int version_{2};
  data::vector<data::unique_ptr<v2>> values_;
};

struct version_detection {
  int version_{0};
};

TEST_CASE("downward compatibility test") {
  for (auto const i : {1, 2}) {
    std::vector<uint8_t> buf;
    switch (i) {
      case 1: {
        data_v1 values;
        values.values_.emplace_back(
            data::make_unique<v1>(v1{data::string{"A"}}));
        values.values_.emplace_back(
            data::make_unique<v1>(v1{data::string{"B"}}));
        values.values_.emplace_back(
            data::make_unique<v1>(v1{data::string{"C"}}));
        buf = cista::serialize(values);
        break;
      }

      case 2: {
        data_v2 values;
        values.values_.emplace_back(data::make_unique<v2>(
            v2{data::string{"A"}, data::string{"1"}, .0}));
        values.values_.emplace_back(data::make_unique<v2>(
            v2{data::string{"B"}, data::string{"2"}, .1}));
        values.values_.emplace_back(data::make_unique<v2>(
            v2{data::string{"C"}, data::string{"3"}, .2}));
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