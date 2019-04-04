#pragma once

#include <iostream>
#include <limits>
#include <map>
#include <vector>

#include "cista/containers.h"
#include "cista/crc64.h"
#include "cista/decay.h"
#include "cista/offset_t.h"
#include "cista/reflection/for_each_field.h"
#include "cista/serialized_size.h"
#include "cista/targets/buf.h"
#include "cista/targets/file.h"
#include "cista/type_hash.h"

#ifndef cista_member_offset
#define cista_member_offset(s, m) (static_cast<cista::offset_t>(offsetof(s, m)))
#endif

namespace cista {

// =============================================================================
// HASH
// -----------------------------------------------------------------------------
template <typename T>
struct use_standard_hash : public std::false_type {};

template <typename T>
hash_t type_hash(T const& el, hash_t hash) {
#pragma warning(push)
#pragma warning(disable : 4307)  // overflow is desired behaviour in fnv1a_hash
  constexpr auto const POINTER = fnv1a_hash("pointer");
  constexpr auto const STRUCT = fnv1a_hash("struct");
#pragma warning(pop)

  using Type = decay_t<T>;
  if constexpr (use_standard_hash<T>()) {
    return fnv1a_hash(detail::nameof_type<Type>(), hash);
  } else if constexpr (!std::is_scalar_v<Type>) {
    static_assert(std::is_aggregate_v<Type> &&
                      std::is_standard_layout_v<Type> &&
                      !std::is_polymorphic_v<Type>,
                  "Please implement custom type hash.");
    hash = hash_combine(hash, STRUCT);
    for_each_field(el,
                   [&](auto const& member) { hash = type_hash(member, hash); });
    return hash;
  } else if constexpr (std::is_pointer_v<Type>) {
    hash = hash_combine(hash, POINTER);
    return type_hash(typename std::remove_pointer_t<Type>{}, hash);
  } else if constexpr (std::is_enum_v<Type>) {
    return fnv1a_hash(detail::nameof_enum<Type>(), hash);
  } else {
    return fnv1a_hash(detail::nameof_type<Type>(), hash);
  }
}

template <typename T, size_t Size>
hash_t type_hash(cista::array<T, Size> const&, hash_t hash) {
  hash = hash_combine(hash, fnv1a_hash("CISTA_ARRAY"));
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(offset::ptr<T> const&, hash_t hash) {
  hash = hash_combine(hash, fnv1a_hash("CISTA_OFFSET_PTR"));
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(offset::vector<T> const&, hash_t hash) {
  hash = hash_combine(hash, fnv1a_hash("CISTA_OFFSET_VECTOR"));
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(offset::unique_ptr<T> const&, hash_t hash) {
  hash = hash_combine(hash, fnv1a_hash("CISTA_OFFSET_UNIQUE_PTR"));
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(raw::vector<T> const&, hash_t hash) {
  hash = hash_combine(hash, fnv1a_hash("CISTA_RAW_VECTOR"));
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(raw::unique_ptr<T> const&, hash_t hash) {
  hash = hash_combine(hash, fnv1a_hash("CISTA_RAW_UNIQUE_PTR"));
  return type_hash(T{}, hash);
}

template <typename T>
hash_t type_hash(T const& el) {
  return type_hash(el, cista::fnv1a_hash());
}

template <>
struct use_standard_hash<offset::string> : public std::true_type {};

template <>
struct use_standard_hash<raw::string> : public std::true_type {};

// =============================================================================
// SERIALIZE
// -----------------------------------------------------------------------------
enum class pointer_type { ABSOLUTE_PTR, RELATIVE_PTR };
struct pending_offset {
  void* origin_ptr_;
  offset_t pos_;
  pointer_type type_;
};

template <typename Target>
struct serialization_context {
  explicit serialization_context(Target& t) : t_{t} {}

  offset_t write(void const* ptr, std::size_t const size,
                 std::size_t const alignment = 0) {
    return t_.write(ptr, size, alignment);
  }

  template <typename T>
  void write(offset_t const pos, T const& val) {
    t_.write(static_cast<std::size_t>(pos), val);
  }

  uint64_t crc64(offset_t const from) const {
    (void)from;
    // t_.crc64(from);
    return 0ull;
  }

  std::map<void*, offset_t> offsets_;
  std::vector<pending_offset> pending_;
  Target& t_;
};

template <typename Ctx, typename T>
void serialize(Ctx& c, T const* origin, offset_t const pos) {
  using Type = decay_t<T>;
  if constexpr (!std::is_scalar_v<Type>) {
    static_assert(std::is_aggregate_v<Type> &&
                      std::is_standard_layout_v<Type> &&
                      !std::is_polymorphic_v<Type>,
                  "Please implement custom serializer.");
    for_each_ptr_field(*origin, [&](auto& member) {
      auto const member_offset =
          static_cast<offset_t>(reinterpret_cast<char const*>(member) -
                                reinterpret_cast<char const*>(origin));
      serialize(c, member, pos + member_offset);
    });
  } else if constexpr (std::is_pointer_v<Type>) {
    if (*origin == nullptr) {
      c.write(pos, NULLPTR_OFFSET);
    } else if (auto const it = c.offsets_.find(*origin);
               it != end(c.offsets_)) {
      c.write(pos, it->second);
    } else {
      c.pending_.emplace_back(
          pending_offset{*origin, pos, pointer_type::ABSOLUTE_PTR});
    }
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, offset_ptr<T> const* origin, offset_t const pos) {
  if (*origin == nullptr) {
    return;
  } else if (auto const it = c.offsets_.find(const_cast<T*>(origin->get()));
             it != end(c.offsets_)) {
    c.write(pos, it->second - pos);
  } else {
    c.pending_.emplace_back(pending_offset{const_cast<T*>(origin->get()), pos,
                                           pointer_type::RELATIVE_PTR});
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, raw::vector<T> const* origin, offset_t const pos) {
  auto const size = serialized_size<T>() * origin->used_size_;
  auto const start = origin->el_ == nullptr
                         ? NULLPTR_OFFSET
                         : c.write(origin->el_, size, std::alignment_of_v<T>);

  c.write(pos + cista_member_offset(raw::vector<T>, el_), start);
  c.write(pos + cista_member_offset(raw::vector<T>, allocated_size_),
          origin->used_size_);
  c.write(pos + cista_member_offset(raw::vector<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    auto i = 0u;
    for (auto it = start; it != start + static_cast<offset_t>(size);
         it += serialized_size<T>()) {
      serialize(c, origin->el_ + i++, it);
    }
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, offset::vector<T> const* origin, offset_t const pos) {
  auto const size = serialized_size<T>() * origin->used_size_;
  auto const start = origin->el_ == nullptr ? NULLPTR_OFFSET
                                            : c.write(origin->el_.get(), size,
                                                      std::alignment_of_v<T>);

  c.write(pos + cista_member_offset(offset::vector<T>, el_),
          start == NULLPTR_OFFSET
              ? start
              : start - cista_member_offset(offset::vector<T>, el_) - pos);
  c.write(pos + cista_member_offset(offset::vector<T>, allocated_size_),
          origin->used_size_);
  c.write(pos + cista_member_offset(offset::vector<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    auto i = 0u;
    for (auto it = start; it != start + static_cast<offset_t>(size);
         it += serialized_size<T>()) {
      serialize(c, (origin->el_ + i++).get(), it);
    }
  }
}

template <typename Ctx>
void serialize(Ctx& c, raw::string const* origin, offset_t const pos) {
  if (origin->is_short()) {
    return;
  }

  auto const start = (origin->h_.ptr_ == nullptr)
                         ? NULLPTR_OFFSET
                         : c.write(origin->data(), origin->size());
  c.write(pos + cista_member_offset(raw::string, h_.ptr_), start);
  c.write(pos + cista_member_offset(raw::string, h_.self_allocated_), false);
}

template <typename Ctx>
void serialize(Ctx& c, offset::string const* origin, offset_t const pos) {
  if (origin->is_short()) {
    return;
  }

  auto const start = (origin->h_.ptr_ == nullptr)
                         ? NULLPTR_OFFSET
                         : c.write(origin->data(), origin->size());
  c.write(pos + cista_member_offset(offset::string, h_.ptr_),
          start == NULLPTR_OFFSET
              ? start
              : start - cista_member_offset(offset::string, h_.ptr_) - pos);
  c.write(pos + cista_member_offset(offset::string, h_.self_allocated_), false);
}

template <typename Ctx, typename T>
void serialize(Ctx& c, raw::unique_ptr<T> const* origin, offset_t const pos) {
  auto const start =
      origin->el_ == nullptr
          ? NULLPTR_OFFSET
          : c.write(origin->el_, serialized_size<T>(), std::alignment_of_v<T>);

  c.write(pos + cista_member_offset(raw::unique_ptr<T>, el_), start);
  c.write(pos + cista_member_offset(raw::unique_ptr<T>, self_allocated_),
          false);

  if (origin->el_ != nullptr) {
    c.offsets_[origin->el_] = start;
    serialize(c, origin->el_, start);
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, offset::unique_ptr<T> const* origin,
               offset_t const pos) {
  auto const start =
      origin->el_ == nullptr
          ? NULLPTR_OFFSET
          : c.write(origin->el_, serialized_size<T>(), std::alignment_of_v<T>);

  c.write(pos + cista_member_offset(offset::unique_ptr<T>, el_),
          start == NULLPTR_OFFSET
              ? start
              : start - cista_member_offset(offset::unique_ptr<T>, el_) - pos);
  c.write(pos + cista_member_offset(offset::unique_ptr<T>, self_allocated_),
          false);

  if (origin->el_ != nullptr) {
    c.offsets_[const_cast<T*>(origin->el_.get())] = start;
    serialize(c, origin->el_.get(), start);
  }
}

template <typename Ctx, typename T, size_t Size>
void serialize(Ctx& c, array<T, Size> const* origin, offset_t const pos) {
  auto const size =
      static_cast<offset_t>(serialized_size<T>() * origin->size());
  auto i = 0u;
  for (auto it = pos; it != pos + size; it += serialized_size<T>()) {
    serialize(c, origin->el_ + i++, it);
  }
}

enum class mode { NONE = 0, WITH_VERSION = 1 << 1, WITH_INTEGRITY = 1 << 2 };
inline mode operator|(mode const& a, mode const& b) {
  return mode{static_cast<std::underlying_type_t<mode>>(a) |
              static_cast<std::underlying_type_t<mode>>(b)};
}
inline mode operator&(mode const& a, mode const& b) {
  return mode{static_cast<std::underlying_type_t<mode>>(a) &
              static_cast<std::underlying_type_t<mode>>(b)};
}

template <typename Target, typename T>
void serialize(Target& t, T& value, mode const m = mode::NONE) {
  serialization_context<Target> c{t};

  if ((m & mode::WITH_VERSION) == mode::WITH_VERSION) {
    auto const hash = type_hash(value);
    c.write(&hash, sizeof(hash));
  }

  auto integrity_offset = offset_t{0};
  if ((m & mode::WITH_INTEGRITY) == mode::WITH_INTEGRITY) {
    auto const hash = uint64_t{};
    integrity_offset = c.write(&hash, sizeof(hash));
  }

  serialize(c, &value,
            c.write(&value, serialized_size<T>(),
                    std::alignment_of_v<decay_t<decltype(value)>>));

  for (auto& p : c.pending_) {
    if (auto const it = c.offsets_.find(p.origin_ptr_); it != end(c.offsets_)) {
      c.write(p.pos_, (p.type_ == pointer_type::ABSOLUTE_PTR)
                          ? it->second
                          : it->second - p.pos_);
    } else {
      std::cout << "warning: dangling pointer " << p.origin_ptr_
                << " serialized at offset " << p.pos_ << "\n";
    }
  }

  if ((m & mode::WITH_INTEGRITY) == mode::WITH_INTEGRITY) {
    c.write(integrity_offset, c.crc64(integrity_offset));
  }
}

template <typename T>
byte_buf serialize(T& el, mode const m = mode::NONE) {
  auto b = buf{};
  serialize(b, el, m);
  return std::move(b.buf_);
}

// =============================================================================
// DESERIALIZE
// -----------------------------------------------------------------------------
template <typename Arg, typename... Args>
Arg checked_addition(Arg a1, Args... aN) {
  using Type = decay_t<Arg>;
  auto add_if_ok = [&](auto x) {
    if (a1 >
        (std::is_pointer_v<Type> ? reinterpret_cast<Type>(0xffffffffffffffff)
                                 : std::numeric_limits<Type>::max()) -
            x) {
      throw std::overflow_error("addition overflow");
    }
    a1 = a1 + x;
  };
  (add_if_ok(aN), ...);
  return a1;
}

template <typename Arg, typename... Args>
Arg checked_multiplication(Arg a1, Args... aN) {
  using Type = decay_t<Arg>;
  auto multiply_if_ok = [&](auto x) {
    if (a1 != 0 && ((std::numeric_limits<Type>::max() / a1) < x)) {
      throw std::overflow_error("addition overflow");
    }
    a1 = a1 * x;
  };
  (multiply_if_ok(aN), ...);
  return a1;
}

struct deserialization_context {
  deserialization_context(uint8_t* from, uint8_t* to) : from_{from}, to_{to} {}

  template <typename T, typename Ptr>
  T deserialize(Ptr* ptr) const {
    auto const offset = reinterpret_cast<offset_t>(ptr);
    return offset == NULLPTR_OFFSET ? nullptr
                                    : reinterpret_cast<T>(from_ + offset);
  }

  template <typename T>
  void check(T* el, size_t size) const {
    auto const* pos = reinterpret_cast<uint8_t const*>(el);
    if (to_ != nullptr && pos != nullptr &&
        (pos < from_ || checked_addition(pos, size) > to_)) {
      throw std::runtime_error("pointer out of bounds");
    }
  }

  void check(bool condition, char const* msg) const {
    if (!condition) {
      throw std::runtime_error(msg);
    }
  }

  uint8_t *from_, *to_;
};

namespace raw {

template <typename T>
void deserialize(deserialization_context const&, T*);

template <typename T>
void deserialize(deserialization_context const&, vector<T>*);

inline void deserialize(deserialization_context const&, string*);

template <typename T>
void deserialize(deserialization_context const&, unique_ptr<T>*);

template <typename T, size_t Size>
void deserialize(deserialization_context const&, array<T, Size>*);

template <typename T>
void deserialize(deserialization_context const& c, T* el) {
  using written_type_t = decay_t<T>;
  if constexpr (std::is_pointer_v<written_type_t>) {
    *el = c.deserialize<written_type_t>(*el);
    c.check(*el, sizeof(*std::declval<written_type_t>()));
  } else if constexpr (std::is_scalar_v<written_type_t>) {
    c.check(el, sizeof(T));
  } else {
    for_each_ptr_field(*el, [&](auto& f) { deserialize(c, f); });
  }
}

template <typename T>
void deserialize(deserialization_context const& c, vector<T>* el) {
  c.check(el, sizeof(vector<T>));
  el->el_ = c.deserialize<T*>(el->el_);
  c.check(el->el_, checked_multiplication(
                       static_cast<size_t>(el->allocated_size_), sizeof(T)));
  c.check(el->allocated_size_ == el->used_size_, "vector size mismatch");
  c.check(!el->self_allocated_, "vector self-allocated");
  for (auto& m : *el) {
    deserialize(c, &m);
  }
}

inline void deserialize(deserialization_context const& c, string* el) {
  c.check(el, sizeof(string));
  if (!el->is_short()) {
    el->h_.ptr_ = c.deserialize<char*>(el->h_.ptr_);
    c.check(el->h_.ptr_, el->h_.size_);
    c.check(!el->h_.self_allocated_, "string self-allocated");
  }
}

template <typename T>
void deserialize(deserialization_context const& c, unique_ptr<T>* el) {
  c.check(el, sizeof(unique_ptr<T>));
  el->el_ = c.deserialize<T*>(el->el_);
  c.check(el->el_, sizeof(T));
  c.check(!el->self_allocated_, "unique_ptr self-allocated");
  deserialize(c, el->el_);
}

template <typename T, size_t Size>
void deserialize(deserialization_context const& c, array<T, Size>* el) {
  c.check(el, sizeof(array<T, Size>));
  for (auto& m : *el) {
    deserialize(c, &m);
  }
}

template <typename T>
T* deserialize(uint8_t* from, uint8_t* to = nullptr) {
  deserialization_context c{from, to};
  auto const el = reinterpret_cast<T*>(from);
  deserialize(c, el);
  return el;
}

template <typename T, typename Container>
T* deserialize(Container& c) {
  return deserialize<T>(&c[0], &c[0] + c.size());
}

// -----------------------------------------------------------------------------

template <typename T>
void unchecked_deserialize(deserialization_context const&, T*);

template <typename T>
void unchecked_deserialize(deserialization_context const&, vector<T>*);

inline void unchecked_deserialize(deserialization_context const&, string*);

template <typename T>
void unchecked_deserialize(deserialization_context const&, unique_ptr<T>*);

template <typename T, size_t Size>
void unchecked_deserialize(deserialization_context const&, array<T, Size>*);

template <typename T>
void unchecked_deserialize(deserialization_context const& c, T* el) {
  using written_type_t = decay_t<T>;
  if constexpr (std::is_pointer_v<written_type_t>) {
    *el = c.deserialize<written_type_t>(*el);
  } else if constexpr (std::is_scalar_v<written_type_t>) {
    // Do nothing.
  } else {
    for_each_ptr_field(*el, [&](auto& f) { unchecked_deserialize(c, f); });
  }
}

template <typename T>
void unchecked_deserialize(deserialization_context const& c, vector<T>* el) {
  el->el_ = c.deserialize<T*>(el->el_);
  for (auto& m : *el) {
    unchecked_deserialize(c, &m);
  }
}

inline void unchecked_deserialize(deserialization_context const& c,
                                  string* el) {
  if (!el->is_short()) {
    el->h_.ptr_ = c.deserialize<char*>(el->h_.ptr_);
  }
}

template <typename T>
void unchecked_deserialize(deserialization_context const& c,
                           unique_ptr<T>* el) {
  el->el_ = c.deserialize<T*>(el->el_);
  unchecked_deserialize(c, el->el_);
}

template <typename T, size_t Size>
void unchecked_deserialize(deserialization_context const& c,
                           array<T, Size>* el) {
  for (auto& m : *el) {
    unchecked_deserialize(c, &m);
  }
}

template <typename T>
T* unchecked_deserialize(uint8_t* from, uint8_t* to = nullptr) {
  deserialization_context c{from, to};
  auto const el = reinterpret_cast<T*>(from);
  unchecked_deserialize(c, el);
  return el;
}

template <typename T, typename Container>
T* unchecked_deserialize(Container& c) {
  return unchecked_deserialize<T>(&c[0], &c[0] + c.size());
}

}  // namespace raw

namespace offset {

template <typename T>
void deserialize(deserialization_context const&, offset_ptr<T>*);

template <typename T>
void deserialize(deserialization_context const&, vector<T>*);

void deserialize(deserialization_context const&, string*);

template <typename T>
void deserialize(deserialization_context const&, unique_ptr<T>*);

template <typename T, size_t Size>
void deserialize(deserialization_context const&, array<T, Size>*);

template <typename T>
void deserialize(deserialization_context const& c, T* el) {
  using written_type_t = decay_t<T>;
  if constexpr (std::is_scalar_v<written_type_t>) {
    c.check(el, sizeof(T));
  } else {
    for_each_ptr_field(*el, [&](auto& f) { deserialize(c, f); });
  }
}

template <typename T>
void deserialize(deserialization_context const& c, offset_ptr<T>* el) {
  using written_type_t = decay_t<T>;
  c.check(el->get(), sizeof(std::declval<written_type_t>()));
}

template <typename T>
void deserialize(deserialization_context const& c, vector<T>* el) {
  c.check(el, sizeof(vector<T>));
  c.check(el->el_.get(),
          checked_multiplication(static_cast<size_t>(el->allocated_size_),
                                 sizeof(T)));
  c.check(el->allocated_size_ == el->used_size_, "vector size mismatch");
  c.check(!el->self_allocated_, "vector self-allocated");
  for (auto& m : *el) {
    deserialize(c, &m);
  }
}

inline void deserialize(deserialization_context const& c, string* el) {
  c.check(el, sizeof(string));
  if (!el->is_short()) {
    c.check(el->h_.ptr_.get(), el->h_.size_);
    c.check(!el->h_.self_allocated_, "string self-allocated");
  }
}

template <typename T>
void deserialize(deserialization_context const& c, unique_ptr<T>* el) {
  c.check(el, sizeof(unique_ptr<T>));
  c.check(el->el_.get(), sizeof(T));
  c.check(!el->self_allocated_, "unique_ptr self-allocated");
  deserialize(c, el->el_.get());
}

template <typename T, size_t Size>
void deserialize(deserialization_context const& c, array<T, Size>* el) {
  c.check(el, sizeof(array<T, Size>));
  for (auto& m : *el) {
    deserialize(c, &m);
  }
}

template <typename T>
T* deserialize(uint8_t* from, uint8_t* to = nullptr) {
  deserialization_context c{from, to};
  auto const el = reinterpret_cast<T*>(from);
  deserialize(c, el);
  return el;
}

template <typename T, typename Container>
T* deserialize(Container& c) {
  return deserialize<T>(&c[0], &c[0] + c.size());
}

template <typename T>
T* unchecked_deserialize(uint8_t* from, uint8_t* to = nullptr) {
  (void)to;
  return reinterpret_cast<T*>(from);
}

template <typename T, typename Container>
T* unchecked_deserialize(Container& c) {
  return unchecked_deserialize<T>(&c[0], &c[0] + c.size());
}

}  // namespace offset

}  // namespace cista

#undef cista_member_offset