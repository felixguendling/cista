#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

constexpr auto const LONG_STR = "aaahelloworldtestaaa";
constexpr auto const SHORT_STR = "ahelloworldtest";

TEST_CASE("cstring serialization long_str") {
  cista::raw::string s = LONG_STR;

  cista::byte_buf buf = cista::serialize(s);
  auto const serialized =
      cista::deserialize<cista::raw::string>(&buf[0], &buf[0] + buf.size());
  CHECK(*serialized == std::string_view{LONG_STR});
}

TEST_CASE("cstring serialization short_str") {
  cista::raw::string s = SHORT_STR;

  cista::byte_buf buf = cista::serialize(s);
  CHECK(buf.size() == sizeof(cista::raw::string));

  auto const serialized =
      cista::deserialize<cista::raw::string>(&buf[0], &buf[0] + buf.size());
  CHECK(*serialized == std::string_view{SHORT_STR});
}

using cista::raw::u16string;
using cista::raw::u16string_view;

const auto constexpr U16STR_SHORT = u"0123";
const auto constexpr U16STR_SHORT_CORNER_CASE = u"0123456";
const auto constexpr U16STR_LONG_CORNER_CASE = u"01234567";
const auto constexpr U16STR_LONG = u"0123456789ABCDEF\xD83D\xDCBB";

TEST_CASE("u16string serialization short") {
  u16string s = U16STR_SHORT;

  cista::byte_buf buf = cista::serialize(s);
  CHECK(buf.size() == sizeof(u16string));

  auto const serialized = cista::deserialize<u16string>(buf);
  CHECK(*serialized == U16STR_SHORT);
}

TEST_CASE("u16string serialization long") {
  u16string s = U16STR_LONG;

  cista::byte_buf buf = cista::serialize(s);
  CHECK(buf.size() == sizeof(u16string) + s.size() * sizeof(char16_t));

  auto const serialized = cista::deserialize<u16string>(buf);
  CHECK(*serialized == U16STR_LONG);
}

TEST_CASE("u16string serialization with additional checks") {
  u16string s_s = U16STR_SHORT_CORNER_CASE, s_l = U16STR_LONG_CORNER_CASE;
  constexpr auto mode = cista::mode::WITH_VERSION |
                        cista::mode::WITH_INTEGRITY | cista::mode::DEEP_CHECK;

  cista::byte_buf buf_s = cista::serialize<mode>(s_s),
                  buf_l = cista::serialize<mode>(s_l);
  u16string *serialized_s, *serialized_l;

  CHECK_NOTHROW(serialized_s = cista::deserialize<u16string, mode>(buf_s));
  CHECK(*serialized_s == U16STR_SHORT_CORNER_CASE);
  CHECK_NOTHROW(serialized_l = cista::deserialize<u16string, mode>(buf_l));
  CHECK(*serialized_l == U16STR_LONG_CORNER_CASE);
}

TEST_CASE("u16string serialization endian short") {
  cista::byte_buf str_le = {0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33,
                            0x00, 0x34, 0x00, 0x35, 0x00, 0x36, 0x00};
  cista::byte_buf str_be = {0x00, 0x30, 0x00, 0x31, 0x00, 0x32, 0x00,
                            0x33, 0x00, 0x34, 0x00, 0x35, 0x00, 0x36};
  u16string s = U16STR_SHORT_CORNER_CASE;

  cista::byte_buf buf_le = cista::serialize(s);
  cista::byte_buf buf_be =
      cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(s);

  CHECK(std::equal(buf_le.begin() + offsetof(u16string, s_.s_), buf_le.end(),
                   str_le.begin()));
  CHECK(std::equal(buf_be.begin() + offsetof(u16string, s_.s_), buf_be.end(),
                   str_be.begin()));

  auto const serialized_be =
      cista::deserialize<u16string, cista::mode::SERIALIZE_BIG_ENDIAN>(buf_be);

  CHECK(*serialized_be == U16STR_SHORT_CORNER_CASE);
}

TEST_CASE("u16string serialization endian long") {
  cista::byte_buf str_le = {0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00,
                            0x34, 0x00, 0x35, 0x00, 0x36, 0x00, 0x37, 0x00};
  cista::byte_buf str_be = {0x00, 0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33,
                            0x00, 0x34, 0x00, 0x35, 0x00, 0x36, 0x00, 0x37};
  u16string s = U16STR_LONG_CORNER_CASE;

  cista::byte_buf buf_le = cista::serialize(s);
  cista::byte_buf buf_be =
      cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(s);

  CHECK(std::equal(buf_le.begin() + sizeof(u16string), buf_le.end(),
                   str_le.begin()));
  CHECK(std::equal(buf_be.begin() + sizeof(u16string), buf_be.end(),
                   str_be.begin()));

  auto const serialized_be =
      cista::deserialize<u16string, cista::mode::SERIALIZE_BIG_ENDIAN>(buf_be);

  CHECK(*serialized_be == U16STR_LONG_CORNER_CASE);
}

using cista::raw::u32string;
using cista::raw::u32string_view;

const auto constexpr U32STR_SHORT = U"01";
const auto constexpr U32STR_SHORT_CORNER_CASE = U"012";
const auto constexpr U32STR_LONG_CORNER_CASE = U"0123";
const auto constexpr U32STR_LONG = U"0123456789ABCDEF\U0001F4BB";

TEST_CASE("u32string serialization short") {
  u32string s = U32STR_SHORT;

  cista::byte_buf buf = cista::serialize(s);
  CHECK(buf.size() == sizeof(u32string));

  auto const serialized = cista::deserialize<u32string>(buf);
  CHECK(*serialized == U32STR_SHORT);
}

TEST_CASE("u32string serialization long") {
  u32string s = U32STR_LONG;

  cista::byte_buf buf = cista::serialize(s);
  CHECK(buf.size() == sizeof(u32string) + s.size() * sizeof(char32_t));

  auto const serialized =
      cista::deserialize<u32string>(&buf[0], &buf[0] + buf.size());
  CHECK(*serialized == U32STR_LONG);
}

TEST_CASE("u32string serialization with additional checks") {
  u32string s_s = U32STR_SHORT_CORNER_CASE, s_l = U32STR_LONG_CORNER_CASE;
  constexpr auto mode = cista::mode::WITH_VERSION |
                        cista::mode::WITH_INTEGRITY | cista::mode::DEEP_CHECK;

  cista::byte_buf buf_s = cista::serialize<mode>(s_s),
                  buf_l = cista::serialize<mode>(s_l);
  u32string *serialized_s, *serialized_l;

  CHECK_NOTHROW(serialized_s = cista::deserialize<u32string, mode>(buf_s));
  CHECK(*serialized_s == U32STR_SHORT_CORNER_CASE);
  CHECK_NOTHROW(serialized_l = cista::deserialize<u32string, mode>(buf_l));
  CHECK(*serialized_l == U32STR_LONG_CORNER_CASE);
}

TEST_CASE("u32string serialization endian short") {
  cista::byte_buf str_le = {0x30, 0x00, 0x00, 0x00, 0x31, 0x00,
                            0x00, 0x00, 0x32, 0x00, 0x00, 0x00};
  cista::byte_buf str_be = {0x00, 0x00, 0x00, 0x30, 0x00, 0x00,
                            0x00, 0x31, 0x00, 0x00, 0x00, 0x32};
  u32string s = U32STR_SHORT_CORNER_CASE;

  cista::byte_buf buf_le = cista::serialize(s);
  cista::byte_buf buf_be =
      cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(s);

  CHECK(std::equal(buf_le.begin() + offsetof(u32string, s_.s_), buf_le.end(),
                   str_le.begin()));
  CHECK(std::equal(buf_be.begin() + offsetof(u32string, s_.s_), buf_be.end(),
                   str_be.begin()));

  auto const serialized_be =
      cista::deserialize<u32string, cista::mode::SERIALIZE_BIG_ENDIAN>(buf_be);

  CHECK(*serialized_be == U32STR_SHORT_CORNER_CASE);
}

TEST_CASE("u32string serialization endian long") {
  cista::byte_buf str_le = {0x30, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00,
                            0x32, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00};
  cista::byte_buf str_be = {0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x31,
                            0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x33};
  u32string s = U32STR_LONG_CORNER_CASE;

  cista::byte_buf buf_le = cista::serialize(s);
  cista::byte_buf buf_be =
      cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(s);

  CHECK(std::equal(buf_le.begin() + sizeof(u32string), buf_le.end(),
                   str_le.begin()));
  CHECK(std::equal(buf_be.begin() + sizeof(u32string), buf_be.end(),
                   str_be.begin()));

  auto const serialized_be =
      cista::deserialize<u32string, cista::mode::SERIALIZE_BIG_ENDIAN>(buf_be);

  CHECK(*serialized_be == U32STR_LONG_CORNER_CASE);
}

TEST_CASE_TEMPLATE("string serialization capacity", StrT, cista::raw::string,
                   u16string, u32string) {
  using CharT = typename StrT::CharT;
  auto get_short = []() -> CharT const* {
    void const* ptr;
    switch (sizeof(CharT)) {
      case sizeof(char): ptr = SHORT_STR; break;
      case sizeof(char16_t): ptr = U16STR_SHORT; break;
      case sizeof(char32_t): ptr = U32STR_SHORT; break;
    }
    return static_cast<CharT const*>(ptr);
  };
  auto get_long = []() -> CharT const* {
    void const* ptr;
    switch (sizeof(CharT)) {
      case sizeof(char): ptr = LONG_STR; break;
      case sizeof(char16_t): ptr = U16STR_LONG; break;
      case sizeof(char32_t): ptr = U32STR_LONG; break;
    }
    return static_cast<CharT const*>(ptr);
  };

  StrT s_s = get_short(), s_l = get_long();
  cista::byte_buf buf_s = cista::serialize(s_s), buf_l = cista::serialize(s_l);
  StrT *serialized_s = cista::deserialize<StrT>(buf_s),
       *serialized_l = cista::deserialize<StrT>(buf_l);
  CharT const *ptr_s = serialized_s->data(), *ptr_l = serialized_l->data();

  CHECK(serialized_s->capacity() == StrT::short_length_limit);
  CHECK(serialized_l->capacity() == 0);

  serialized_s->shrink_to_fit();
  serialized_l->shrink_to_fit();

  CHECK(serialized_s->capacity() == StrT::short_length_limit);
  CHECK(serialized_l->capacity() == 256);
  CHECK(ptr_s == serialized_s->data());
  CHECK(ptr_l != serialized_l->data());
  CHECK(*serialized_s == get_short());
  CHECK(*serialized_l == get_long());
}
