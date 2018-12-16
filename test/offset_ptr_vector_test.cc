#include "doctest.h"

#include "cista.h"

TEST_CASE("offset vector serialize") {
  cista::byte_buf buf;
  {
    cista::o_vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    int j = 1;
    for (auto const i : vec) {
      CHECK(i == j++);
    }

    buf = serialize(vec);
  }

  auto const vec = reinterpret_cast<cista::o_vector<int>*>(&buf[0]);
  int j = 1;
  for (auto const i : *vec) {
    CHECK(i == j++);
  }
}

TEST_CASE("offset string serialize") {
  constexpr auto const s = "The quick brown fox jumps over the lazy dog";

  cista::byte_buf buf;
  {
    cista::o_string str{s};
    buf = serialize(str);
  }

  auto const str = reinterpret_cast<cista::o_string*>(&buf[0]);
  CHECK(*str == s);
}

TEST_CASE("offset unique_ptr serialize") {
  cista::byte_buf buf;
  {
    auto const ptr = cista::make_o_unique<int>(33);
    buf = serialize(ptr);
  }

  auto const ptr = reinterpret_cast<cista::o_unique_ptr<int>*>(&buf[0]);
  CHECK(**ptr == 33);
}

TEST_CASE("offset_ptr serialize") {
  struct serialize_me {
    cista::o_unique_ptr<int> i_{cista::make_o_unique<int>(77)};
    cista::offset_ptr<int> raw_{i_.get()};
  };

  cista::byte_buf buf;

  {
    serialize_me obj;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const serialized = reinterpret_cast<serialize_me*>(&buf[0]);
  CHECK(serialized->raw_.get() == serialized->i_.get());
  CHECK(*serialized->raw_ == 77);
  CHECK(*serialized->i_.get() == 77);
}

TEST_CASE("offset_ptr serialize pending") {
  struct serialize_me {
    cista::offset_ptr<int> raw_;
    cista::o_unique_ptr<int> i_{cista::make_o_unique<int>(77)};
  };

  cista::byte_buf buf;

  {
    serialize_me obj;
    obj.raw_ = obj.i_.get();
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const serialized = reinterpret_cast<serialize_me*>(&buf[0]);
  CHECK(serialized->raw_.get() == serialized->i_.get());
  CHECK(*serialized->raw_ == 77);
  CHECK(*serialized->i_.get() == 77);
}