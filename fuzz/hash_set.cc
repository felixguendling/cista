#include <cinttypes>
#include <queue>
#include <set>

#include "cista/serialization.h"
#include "cista/targets/file.h"

namespace data = cista::raw;

struct hash {
  size_t operator()(data::vector<data::string> const& v) {
    auto hash = cista::BASE_HASH;
    for (auto const& s : v) {
      hash = cista::hash(s, hash);
    }
    return hash;
  }
};

struct eq {
  bool operator()(data::vector<data::string> const& a,
                  data::vector<data::string> const& b) {
    if (a.size() != b.size()) {
      return false;
    }
    for (auto ia = a.begin(), ib = b.begin(); ia != a.end(); ++ia, ++ib) {
      if (*ia != *ib) {
        return false;
      }
    }
    return true;
  }
};

using serialize_me_t = data::hash_set<data::vector<data::string>, hash, eq>;

#if defined(GENERATE_SEED)
int main(int argc, char** argv) {
  if (argc != 2) {
    printf("usage: %s [seed file]\n", argv[0]);
    return 0;
  }

  auto const make_e1 = []() {
    data::vector<data::string> e1;
    e1.emplace_back(data::string{"short", data::string::owning_t{}});
    e1.emplace_back(
        data::string{"long long long long long long long", data::string::owning_t{}});
    return e1;
  };

  auto const make_e2 = []() {
    data::vector<data::string> e2;
    e2.emplace_back(data::string{"hello", data::string::owning_t{}});
    e2.emplace_back(data::string{"world", data::string::owning_t{}});
    e2.emplace_back(data::string{"yeah!", data::string::owning_t{}});
    return e2;
  };

  auto const make_e3 = []() {
    data::vector<data::string> e3;
    e3.emplace_back(data::string{"This", data::string::non_owning_t{}});
    e3.emplace_back(data::string{"is", data::string::non_owning_t{}});
    e3.emplace_back(data::string{"Sparta", data::string::non_owning_t{}});
    e3.emplace_back(data::string{"!!!", data::string::non_owning_t{}});
    return e3;
  };

  serialize_me_t s;
  s.emplace(make_e1());
  s.emplace(make_e2());
  s.emplace(make_e3());

  cista::file f{argv[1], "w+"};
  cista::serialize(f, s);
}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
  try {
    cista::buffer b{reinterpret_cast<char const*>(data), size};
    for (auto const& el :
         *cista::deserialize<serialize_me_t, cista::mode::DEEP_CHECK>(b)) {
      if (auto const it = std::find_if(
              begin(el), end(el),
              [&](auto&& el) {
                return el == "The quick brown fox jumps over the lazy dog";
              });
          it != end(el)) {
        printf("found!\n");
      }
    }
  } catch (std::exception const&) {
  }
  return 0;
}
#endif