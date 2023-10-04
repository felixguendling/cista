#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/rtree.h"
#include "cista/serialization.h"
#include "cista/type_hash/static_type_hash.h"
#endif

TEST_CASE("x") {
  using rtree_t = cista::rtree<void*>;

  cista::byte_buf buf;

  {
    rtree_t rt;
    for (auto i = 0.F; i != 1000.F; ++i) {
      rt.insert({i, i}, {i + 1.F, i + 1.F}, nullptr);
    }
    buf = cista::serialize(rt);
  }  // EOL s

  auto const rt = cista::deserialize<rtree_t>(buf);
  rt->search(
      {0.0, 0.0}, {2.0, 2.0},
      [](rtree_t::coord_t const& min, rtree_t::coord_t const& max, void* data) {
        CHECK((data == nullptr));
        CHECK((min == rtree_t::coord_t{1.0, 1.0}));
        CHECK((max == rtree_t::coord_t{2.0, 2.0}));
        return true;
      });
}