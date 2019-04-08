#include "doctest.h"

#include "cista.h"

namespace data = cista::offset;
using namespace cista;

static unsigned ctor{0}, cpy_ctor{0}, mov_ctor{0}, cpy_assign{0}, mov_assign{0},
    dtor{0};

void reset() {
  ctor = 0;
  cpy_ctor = 0;
  mov_ctor = 0;
  cpy_assign = 0;
  mov_assign = 0;
  dtor = 0;
}

struct array_test_struct {
  array_test_struct(int i) : i_{i} { ++ctor; }
  array_test_struct() { ++ctor; }
  array_test_struct(array_test_struct const& o) : i_{o.i_} { ++cpy_ctor; }
  array_test_struct(array_test_struct&& o) : i_{o.i_} { ++mov_ctor; }
  array_test_struct& operator=(array_test_struct const& o) {
    i_ = o.i_;
    ++cpy_assign;
    return *this;
  }
  array_test_struct& operator=(array_test_struct&& o) {
    i_ = o.i_;
    ++mov_assign;
    return *this;
  }
  ~array_test_struct() { ++dtor; }
  int i_{0};
};

hash_t type_hash(array_test_struct const&, hash_t const h) {
  return hash_combine(h, cista::type_hash<array_test_struct>());
}

TEST_CASE("array test move reverse") {
  reset();

  {
    data::array<array_test_struct, 2> arr1, arr2;
    arr1[0].i_ = 1;
    arr1[1].i_ = 2;

    std::copy(std::move_iterator(arr1.rbegin()),
              std::move_iterator(arr1.rend()), std::begin(arr2));

    CHECK(arr2.at(0).i_ == 2);
    CHECK(arr2[1].i_ == 1);
  }

  CHECK(ctor == 4);
  CHECK(cpy_ctor == 0);
  CHECK(mov_ctor == 0);
  CHECK(cpy_assign == 0);
  CHECK(mov_assign == 2);
  CHECK(dtor == 4);
}

TEST_CASE("array test copy forward") {
  reset();

  {
    data::array<array_test_struct, 2> arr1, arr2;
    arr1[0].i_ = 1;
    arr1[1].i_ = 2;

    std::copy(std::begin(arr1), std::end(arr1), std::begin(arr2));

    CHECK(arr2.at(0).i_ == 1);
    CHECK(arr2[1].i_ == 2);
  }

  CHECK(ctor == 4);
  CHECK(cpy_ctor == 0);
  CHECK(mov_ctor == 0);
  CHECK(cpy_assign == 2);
  CHECK(mov_assign == 0);
  CHECK(dtor == 4);
}

TEST_CASE("array serialize test") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::array<int, 4> d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, data::array<int, 4>{}}};
    obj.j_.d_[0] = 1;
    obj.j_.d_[1] = 2;
    obj.j_.d_[2] = 3;
    obj.j_.d_[3] = 4;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const deserialize = data::deserialize<serialize_me>(buf);
  CHECK(deserialize->j_.d_[0] == 1);
  CHECK(deserialize->j_.d_[1] == 2);
  CHECK(deserialize->j_.d_[2] == 3);
  CHECK(deserialize->j_.d_[3] == 4);
}