#include "doctest.h"

#include <vector>

#include "cista.h"

namespace std {

template <typename Ctx, typename T>
inline void serialize(Ctx& c, std::vector<T> const* origin,
                      cista::offset_t const offset) {
  static_assert(sizeof(std::vector<T>) >=
                sizeof(cista::NULLPTR_OFFSET) + sizeof(uint64_t));

  auto const start = origin->empty()
                         ? cista::NULLPTR_OFFSET
                         : c.write(origin->data(), origin->size() * sizeof(T),
                                   std::alignment_of_v<T>);

  c.write(offset, static_cast<uint64_t>(origin->size()));
  c.write(offset + static_cast<cista::offset_t>(sizeof(uint64_t)), start);

  for (auto i = 0u; i < origin->size(); ++i) {
    cista::serialize(
        c, &(*origin)[i],
        start + static_cast<cista::offset_t>(i * cista::serialized_size<T>()));
  }
}

template <typename T>
inline void unchecked_deserialize(cista::deserialization_context const& c,
                                  std::vector<T>* el) {
  auto const size = *reinterpret_cast<uint64_t*>(el);
  auto const data = reinterpret_cast<T*>(
      c.from_ + *reinterpret_cast<cista::offset_t*>(
                    reinterpret_cast<uint8_t*>(el) + sizeof(uint64_t)));

  for (auto it = data; it != data + size; ++it) {
    cista::raw::unchecked_deserialize(c, it);
  }

  auto& vec = *new (static_cast<void*>(el)) std::vector<T>();

  if (size != 0 && data != nullptr) {
    vec.resize(size);
    vec.insert(begin(vec), std::move_iterator(data),
               std::move_iterator(data + size));
  }
}

template <typename T>
cista::hash_t type_hash(std::vector<T> const&, cista::hash_t h) {
  h = cista::hash_combine(h, cista::type_hash<std::vector<T>>());
  return cista::type_hash(T{}, h);
}

}  // namespace std

namespace data = cista::raw;

TEST_CASE("vector test") {
  using serialize_me = std::vector<data::vector<std::vector<int64_t>>>;

  cista::byte_buf buf;

  {
    serialize_me obj(3);
    obj[0].emplace_back(std::vector<int64_t>({1, 2, 3}));
    obj[0].emplace_back(std::vector<int64_t>({2, 3, 4}));
    obj[0].emplace_back(std::vector<int64_t>({3, 4, 5}));
    obj[1].emplace_back(std::vector<int64_t>({4, 5, 6}));
    obj[1].emplace_back(std::vector<int64_t>({5, 6, 7}));
    obj[2].emplace_back(std::vector<int64_t>({6, 7, 8}));
    buf = serialize(obj);
  }  // EOL obj

  auto const& serialized = *data::unchecked_deserialize<serialize_me>(buf);
  CHECK(serialized[0][0][0] == 1);
  CHECK(serialized[0][1][0] == 2);
  CHECK(serialized[0][2][0] == 3);
  CHECK(serialized[1][0][0] == 4);
  CHECK(serialized[1][1][0] == 5);
  CHECK(serialized[2][0][0] == 6);

  for (auto& v1 : serialized) {
    for (auto& v2 : v1) {
      v2.~vector();
    }
  }
  serialized.~vector();
}
