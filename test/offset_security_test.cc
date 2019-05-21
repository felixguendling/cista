#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace data = cista::offset;

namespace {
inline void test_sec_value_overflow() {
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

inline void test_sec_string_overflow() {
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

inline void test_sec_vector_overflow() {
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

inline void test_sec_unique_ptr_overflow_unset() {
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

inline void test_sec_unique_ptr_overflow_set() {
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

inline void test_sec_array_overflow() {
  struct serialize_me {
    int a_{0};
    struct inner {
      int b_{0};
      int c_{0};
      data::array<int, 3> d_;
    } j_;
  };

  cista::byte_buf buf;

  {
    serialize_me obj{1, {2, 3, {}}};
    obj.j_.d_[0] = 1;
    obj.j_.d_[1] = 2;
    obj.j_.d_[2] = 3;
    buf = cista::serialize(obj);
  }  // EOL obj

  CHECK(buf.size() == sizeof(serialize_me));
  buf.resize(buf.size() - 1);
  CHECK_THROWS(cista::deserialize<serialize_me>(buf));
}
}  // namespace

TEST_CASE("sec offset test value overflow") { test_sec_value_overflow(); }
TEST_CASE("sec offset test string overflow") { test_sec_string_overflow(); }
TEST_CASE("sec offset test vector overflow") { test_sec_vector_overflow(); }
TEST_CASE("sec offset test unique ptr overflow unset") {
  test_sec_unique_ptr_overflow_unset();
}
TEST_CASE("sec offset test unique ptr overflow set") {
  test_sec_unique_ptr_overflow_set();
}
TEST_CASE("sec offset test array overflow") { test_sec_array_overflow(); }