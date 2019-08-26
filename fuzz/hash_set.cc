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
    e1.emplace_back("short");
    e1.emplace_back("long long long long long long long",
                    data::string::owning_t{});
    return e1;
  };

  auto const make_e2 = []() {
    data::vector<data::string> e2;
    e2.emplace_back("hello");
    e2.emplace_back("world");
    e2.emplace_back("yeah!");
    return e2;
  };

  auto const make_e3 = []() {
    data::vector<data::string> e3;
    e3.emplace_back("This");
    e3.emplace_back("is");
    e3.emplace_back("Sparta");
    e3.emplace_back("!!!");
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
static bool __attribute__((used)) found;
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
        found = true;
      }
    }
  } catch (std::exception const&) {
  }
  return 0;
}
#endif