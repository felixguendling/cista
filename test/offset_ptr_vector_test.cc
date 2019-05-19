#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace data = cista::offset;

TEST_CASE("offset vector serialize") {
  cista::byte_buf buf;
  {
    data::vector<int32_t> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    int32_t j = 1;
    for (auto const i : vec) {
      CHECK(i == j++);
    }

    buf = serialize(vec);
  }

  auto const vec = data::deserialize<data::vector<int32_t>>(buf);
  int32_t j = 1;
  for (auto const i : *vec) {
    CHECK(i == j++);
  }
}

TEST_CASE("offset string serialize") {
  constexpr auto const s = "The quick brown fox jumps over the lazy dog";

  cista::byte_buf buf;
  {
    data::string str{s};
    buf = serialize(str);
  }

  auto const str = data::deserialize<data::string>(buf);
  CHECK(*str == s);
}

TEST_CASE("offset unique_ptr serialize") {
  cista::byte_buf buf;
  {
    auto const ptr = data::make_unique<int>(33);
    buf = serialize(ptr);
  }

  auto const ptr = data::deserialize<data::unique_ptr<int>>(buf);
  CHECK(**ptr == 33);
}

TEST_CASE("offset_ptr serialize") {
  struct serialize_me {
    data::unique_ptr<int> i_{data::make_unique<int>(77)};
    data::ptr<int> raw_{i_.get()};
  };

  cista::byte_buf buf;

  {
    serialize_me obj;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const deserialized = data::deserialize<serialize_me>(buf);
  CHECK(deserialized->raw_.get() == deserialized->i_.get());
  CHECK(*deserialized->raw_ == 77);
  CHECK(*deserialized->i_.get() == 77);
}

TEST_CASE("offset_ptr serialize pending") {
  struct serialize_me {
    data::ptr<int> raw_{nullptr};
    data::unique_ptr<int> i_{data::make_unique<int>(77)};
  };

  cista::byte_buf buf;

  {
    serialize_me obj;
    obj.raw_ = obj.i_.get();
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const serialized = data::deserialize<serialize_me>(buf);
  CHECK(serialized->raw_.get() == serialized->i_.get());
  CHECK(*serialized->raw_ == 77);
  CHECK(*serialized->i_.get() == 77);
}