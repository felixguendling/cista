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
  c.write(offset + sizeof(uint64_t), start);

  for (auto i = size_t{0}; i < origin->size(); ++i) {
    cista::serialize(c, &(*origin)[i], start + i * cista::serialized_size<T>());
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

}  // namespace std

namespace data = cista::raw;

TEST_CASE("vector test") {
  cista::byte_buf buf;

  {
    std::vector<data::vector<int64_t>> obj(3);
    obj[0].emplace_back(1);
    obj[0].emplace_back(2);
    obj[0].emplace_back(3);
    obj[1].emplace_back(4);
    obj[1].emplace_back(5);
    obj[2].emplace_back(6);
    buf = serialize(obj);
  }  // EOL obj

  auto const& serialized =
      *data::unchecked_deserialize<std::vector<data::vector<int64_t>>>(buf);
  CHECK(serialized[0][0] == 1);
  CHECK(serialized[0][1] == 2);
  CHECK(serialized[0][2] == 3);
  CHECK(serialized[1][0] == 4);
  CHECK(serialized[1][1] == 5);
  CHECK(serialized[2][0] == 6);
}