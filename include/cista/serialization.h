#pragma once

#include <iostream>
#include <limits>
#include <map>
#include <vector>

#include "cista/containers/string.h"
#include "cista/containers/unique_ptr.h"
#include "cista/containers/vector.h"
#include "cista/offset_t.h"
#include "cista/reflection/for_each_field.h"
#include "cista/targets/buf.h"
#include "cista/targets/file.h"

namespace cista {

template <typename T, typename TemplateSizeType = uint32_t>
using o_vector = vector<T, offset_ptr<T>, TemplateSizeType>;

// =============================================================================
// SERIALIZE
// -----------------------------------------------------------------------------
struct pending_offset {
  void* origin_ptr_;
  offset_t pos_;
};

template <typename Target>
struct serialization_context {
  explicit serialization_context(Target& t) : t_{t} {}

  offset_t write(void const* ptr, offset_t const size, offset_t alignment = 0) {
    return t_.write(ptr, size, alignment);
  }

  template <typename T>
  void write(offset_t const pos, T const& val) {
    t_.write(pos, val);
  }

  std::map<void*, offset_t> offsets_;
  std::vector<pending_offset> pending_;
  Target& t_;
};

template <typename Ctx, typename T>
void serialize(Ctx& c, T const* origin, offset_t const pos) {
  using Type = std::remove_reference_t<std::remove_const_t<T>>;
  if constexpr (!std::is_scalar_v<Type>) {
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
      c.pending_.emplace_back(pending_offset{*origin, pos});
    }
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, offset_ptr<T> const* origin, offset_t const pos) {
  if (*origin == nullptr) {
    return;
  } else if (auto const it = c.offsets_.find(const_cast<T*>(origin->get()));
             it != end(c.offsets_)) {
    c.write(pos, pos - it->second);
  } else {
    // TODO(felixguendling): Create a new pending map for offset_ptr.
    // c.pending_.emplace_back(pending_offset{*origin, pos});
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, vector<T> const* origin, offset_t const pos) {
  auto const size = sizeof(T) * origin->used_size_;
  auto const start = origin->el_ == nullptr
                         ? NULLPTR_OFFSET
                         : c.write(origin->el_, size, std::alignment_of_v<T>);

  c.write(pos + offsetof(vector<T>, el_), start);
  c.write(pos + offsetof(vector<T>, allocated_size_), origin->used_size_);
  c.write(pos + offsetof(vector<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    auto i = 0u;
    for (auto it = start; it != start + size; it += sizeof(T)) {
      serialize(c, origin->el_ + i++, it);
    }
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, o_vector<T> const* origin, offset_t const pos) {
  auto const size = sizeof(T) * origin->used_size_;
  auto const start = origin->el_ == nullptr ? NULLPTR_OFFSET
                                            : c.write(origin->el_.get(), size,
                                                      std::alignment_of_v<T>);

  c.write(pos + offsetof(o_vector<T>, el_),
          start == NULLPTR_OFFSET ? start
                                  : start - offsetof(o_vector<T>, el_) - pos);

  if (origin->el_ != nullptr) {
    auto i = 0u;
    for (auto it = start; it != start + size; it += sizeof(T)) {
      serialize(c, (origin->el_ + i++).get(), it);
    }
  }
}

template <typename Ctx>
void serialize(Ctx& c, string const* origin, offset_t const pos) {
  if (origin->is_short()) {
    return;
  }

  auto const start = (origin->h_.ptr_ == nullptr)
                         ? NULLPTR_OFFSET
                         : c.write(origin->data(), origin->size());
  c.write(pos + offsetof(string, h_.ptr_), start);
  c.write(pos + offsetof(string, h_.self_allocated_), false);
}

template <typename Ctx>
void serialize(Ctx& c, o_string const* origin, offset_t const pos) {
  if (origin->is_short()) {
    return;
  }

  auto const start = (origin->h_.ptr_ == nullptr)
                         ? NULLPTR_OFFSET
                         : c.write(origin->data(), origin->size());
  c.write(pos + offsetof(o_string, h_.ptr_),
          start == NULLPTR_OFFSET ? start
                                  : start - offsetof(o_string, h_.ptr_) - pos);
  c.write(pos + offsetof(o_string, h_.self_allocated_), false);
}

template <typename Ctx, typename T>
void serialize(Ctx& c, unique_ptr<T> const* origin, offset_t const pos) {
  auto const start = origin->el_ == nullptr ? NULLPTR_OFFSET
                                            : c.write(origin->el_, sizeof(T),
                                                      std::alignment_of_v<T>);

  c.write(pos + offsetof(unique_ptr<T>, el_), start);
  c.write(pos + offsetof(unique_ptr<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    c.offsets_[origin->el_] = start;
    serialize(c, origin->el_, start);
  }
}

template <typename Ctx, typename T>
void serialize(Ctx& c, o_unique_ptr<T> const* origin, offset_t const pos) {
  auto const start = origin->el_ == nullptr ? NULLPTR_OFFSET
                                            : c.write(origin->el_, sizeof(T),
                                                      std::alignment_of_v<T>);

  c.write(pos + offsetof(o_unique_ptr<T>, el_),
          start == NULLPTR_OFFSET
              ? start
              : start - offsetof(o_unique_ptr<T>, el_) - pos);
  c.write(pos + offsetof(o_unique_ptr<T>, self_allocated_), false);

  if (origin->el_ != nullptr) {
    c.offsets_[const_cast<int*>(origin->el_.get())] = start;
    serialize(c, origin->el_.get(), start);
  }
}

template <typename Target, typename T>
void serialize(Target& t, T& value) {
  serialization_context<Target> c{t};

  serialize(
      c, &value,
      c.write(
          &value, sizeof(value),
          std::alignment_of_v<
              std::remove_reference_t<std::remove_const_t<decltype(value)>>>));

  for (auto& p : c.pending_) {
    if (auto const it = c.offsets_.find(p.origin_ptr_); it != end(c.offsets_)) {
      c.write(p.pos_, it->second);
    } else {
      std::cout << "warning: dangling pointer " << p.origin_ptr_
                << " serialized at offset " << p.pos_ << "\n";
    }
  }
}

template <typename T>
byte_buf serialize(T& el) {
  auto b = buf{};
  serialize(b, el);
  return std::move(b.buf_);
}

// =============================================================================
// DESERIALIZE
// -----------------------------------------------------------------------------
template <typename Arg, typename... Args>
Arg checked_addition(Arg a1, Args... aN) {
  using Type = std::remove_reference_t<std::remove_const_t<Arg>>;
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
  using Type = std::remove_reference_t<std::remove_const_t<Arg>>;
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
  deserialization_context(bool checked, uint8_t* from, uint8_t* to)
      : checked_{checked}, from_{from}, to_{to} {}

  template <typename T, typename Ptr>
  T deserialize(Ptr* ptr) const {
    auto const offset = reinterpret_cast<offset_t>(ptr);
    return offset == NULLPTR_OFFSET ? nullptr
                                    : reinterpret_cast<T>(from_ + offset);
  }

  template <typename T>
  void check(T* el, size_t size) const {
    auto const* pos = reinterpret_cast<uint8_t const*>(el);
    if (checked_ && to_ && pos != nullptr &&
        (pos < from_ || checked_addition(pos, size) > to_)) {
      throw std::runtime_error("pointer out of bounds");
    }
  }

  void check(bool condition, char const* msg) const {
    if (!condition) {
      throw std::runtime_error(msg);
    }
  }

  bool checked_;
  uint8_t *from_, *to_;
};

template <typename T>
void deserialize(deserialization_context const& c, T* el) {
  using written_type_t = std::remove_reference_t<std::remove_const_t<T>>;
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

template <typename T>
T* deserialize(uint8_t* from, uint8_t* to = nullptr, bool checked = true) {
  deserialization_context c{checked, from, to};
  auto const el = reinterpret_cast<T*>(from);
  deserialize(c, el);
  return el;
}

template <typename T, typename Container>
T* deserialize(Container& c, bool checked = true) {
  return deserialize<T>(&c[0], &c[0] + c.size(), checked);
}

}  // namespace cista
