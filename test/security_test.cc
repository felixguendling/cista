#include "cista.h"

#include "doctest.h"

namespace data = cista::raw;

TEST_CASE("sec value overflow") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::string d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, data::string{"testtes"}}};
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me));
  buf.resize(buf.size() - 1);

  CHECK_THROWS(cista::deserialize<serialize_me>(buf));
}

TEST_CASE("sec string overflow") {
  auto constexpr const long_str = "The quick brown fox jumps over the lazy dog";

  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::string d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, data::string{long_str}}};
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me) + std::strlen(long_str));
  buf.resize(buf.size() - 1);

  CHECK_THROWS(cista::deserialize<serialize_me>(buf));
}

TEST_CASE("sec vector overflow") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::vector<int> d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, data::vector<int>{}}};
    obj.j_.d_.push_back(1);
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me) + sizeof(int));
  buf.resize(buf.size() - 1);
  CHECK_THROWS(cista::deserialize<serialize_me>(buf));

  {
    serialize_me obj{1, {2, 3, data::vector<int>{}}};
    buf = cista::serialize(obj);
  }
  buf.resize(buf.size() - 1);
  CHECK_THROWS(cista::deserialize<serialize_me>(buf));
}

TEST_CASE("sec unique_ptr overflow unset") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::unique_ptr<int> d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, data::unique_ptr<int>{}}};
    buf = cista::serialize(obj);
  }
  buf.resize(buf.size() - 1);
  CHECK_THROWS(cista::deserialize<serialize_me>(buf));
}

TEST_CASE("sec unique_ptr overflow set") {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::unique_ptr<int> d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, data::unique_ptr<int>{}}};
    obj.j_.d_ = data::make_unique<int>(3);
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me) + sizeof(int));
  buf.resize(buf.size() - 1);
  CHECK_THROWS(cista::deserialize<serialize_me>(buf));
}