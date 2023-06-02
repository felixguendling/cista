#include "doctest.h"

#include <sstream>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/serialization.h"
#endif

namespace data = cista::offset;

TEST_CASE("union_test") {
  union union_type {
    int32_t a_;
    float b_;
  };
  static_assert(!cista::to_tuple_works_v<union_type>);

  cista::byte_buf buf;

  {
    union_type obj;
    obj.b_ = 33.4F;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const deserialize = cista::deserialize<union_type>(buf);
  CHECK(deserialize->b_ == 33.4F);
}

struct Base {};
struct Derive : Base {
  int field;
};

template <typename Ctx>
void serialize(Ctx&, Derive const*, cista::offset_t const) {}

template <typename Ctx>
void deserialize(Ctx const&, Derive*) {}

TEST_CASE("derive_test") {
  cista::byte_buf buf;

  {
    Derive obj;
    obj.field = 33;
    buf = cista::serialize(obj);
  }  // EOL obj

  auto const deserialize = cista::deserialize<Derive>(buf);
  CHECK(deserialize->field == 33);
}

union union_type {
  union_type() : type_{type_t::NONE} {}
  ~union_type() {
    switch (type_) {
      case type_t::MAP: a_.~a(); break;
      case type_t::VEC: b_.~b(); break;
      case type_t::NONE: break;
    }
  }

  friend std::ostream& operator<<(std::ostream& out, union_type const& u) {
    switch (u.type_) {
      case union_type::type_t::NONE: out << "{empty}"; break;
      case union_type::type_t::MAP:
        for (auto const& [k, v] : u.a_.map_) {
          out << k << ", " << v << "\n";
        }
        break;
      case union_type::type_t::VEC:
        for (auto const& [k, v] : u.b_.vec_) {
          out << k << ", " << v << "\n";
        }
        break;
    }
    return out;
  }

  enum class type_t : int8_t { NONE, MAP, VEC } type_;
  struct a {
    int8_t type_;
    data::hash_map<int, int> map_;
  } a_;
  struct b {
    int8_t type_;
    data::vector<data::pair<int, int>> vec_;
  } b_;
};

template <typename Ctx>
void serialize(Ctx& c, union_type const* el, cista::offset_t const pos) {
  switch (el->type_) {
    case union_type::type_t::MAP: serialize(c, &el->a_, pos); break;
    case union_type::type_t::VEC: serialize(c, &el->b_, pos); break;
    case union_type::type_t::NONE: break;
  }
}

template <typename Ctx>
void deserialize(Ctx const& c, union_type* el) {
  switch (el->type_) {
    case union_type::type_t::MAP: deserialize(c, &el->a_); break;
    case union_type::type_t::VEC: deserialize(c, &el->b_); break;
    case union_type::type_t::NONE: break;
  }
}

TEST_CASE("complex union") {
  cista::byte_buf buf;
  {
    union_type obj;
    obj.type_ = union_type::type_t::MAP;
    obj.a_.map_ = {{1, 2}, {3, 4}};
    buf = cista::serialize(obj);
  }

  auto const u = cista::deserialize<union_type>(buf);

  std::stringstream ss;
  ss << *u;
  auto const check = ss.str() == "1, 2\n3, 4\n" || ss.str() == "3, 4\n1, 2\n";
  CHECK(check);
}