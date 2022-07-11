#pragma once

#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <vector>

#include "cista/aligned_alloc.h"
#include "cista/containers.h"
#include "cista/decay.h"
#include "cista/endian/conversion.h"
#include "cista/hash.h"
#include "cista/mode.h"
#include "cista/offset_t.h"
#include "cista/reflection/for_each_field.h"
#include "cista/serialized_size.h"
#include "cista/strong.h"
#include "cista/targets/buf.h"
#include "cista/targets/file.h"
#include "cista/type_hash/type_hash.h"
#include "cista/unused_param.h"
#include "cista/verify.h"

#ifndef cista_member_offset
#define cista_member_offset(s, m) (static_cast<cista::offset_t>(offsetof(s, m)))
#endif

namespace cista {

// =============================================================================
// SERIALIZE
// -----------------------------------------------------------------------------
struct pending_offset {
  void const* origin_ptr_;
  offset_t pos_;
};

struct vector_range {
  bool contains(void const* begin, void const* ptr) const noexcept {
    auto const ptr_int = reinterpret_cast<uintptr_t>(ptr);
    auto const from = reinterpret_cast<uintptr_t>(begin);
    auto const to =
        reinterpret_cast<uintptr_t>(begin) + static_cast<uintptr_t>(size_);
    return ptr_int >= from && ptr_int < to;
  }

  offset_t offset_of(void const* begin, void const* ptr) const noexcept {
    return start_ + reinterpret_cast<intptr_t>(ptr) -
           reinterpret_cast<intptr_t>(begin);
  }

  offset_t start_;
  size_t size_;
};

template <typename Target, mode Mode>
struct serialization_context {
  static constexpr auto const MODE = Mode;

  explicit serialization_context(Target& t) : t_{t} {}

  static bool compare(std::pair<void const*, vector_range> const& a,
                      std::pair<void const*, vector_range> const& b) noexcept {
    return a.first < b.first;
  }

  offset_t write(void const* ptr, std::size_t const size,
                 std::size_t const alignment = 0) {
    return t_.write(ptr, size, alignment);
  }

  template <typename T>
  void write(offset_t const pos, T const& val) {
    t_.write(static_cast<std::size_t>(pos), val);
  }

  template <typename T>
  bool resolve_pointer(offset_ptr<T> const& ptr, offset_t const pos,
                       bool add_pending = true) {
    return resolve_pointer(ptr.get(), pos, add_pending);
  }

  template <typename Ptr>
  bool resolve_pointer(Ptr ptr, offset_t const pos, bool add_pending = true) {
    if (std::is_same_v<decay_t<remove_pointer_t<Ptr>>, void> && add_pending) {
      write(pos, convert_endian<MODE>(NULLPTR_OFFSET));
      return true;
    } else if (ptr == nullptr) {
      write(pos, convert_endian<MODE>(NULLPTR_OFFSET));
      return true;
    } else if (auto const it = offsets_.find(ptr_cast(ptr));
               it != end(offsets_)) {
      write(pos, convert_endian<MODE>(it->second - pos));
      return true;
    } else if (auto const offset = resolve_vector_range_ptr(ptr);
               offset.has_value()) {
      write(pos, convert_endian<MODE>(*offset - pos));
      return true;
    } else if (add_pending) {
      write(pos, convert_endian<MODE>(NULLPTR_OFFSET));
      pending_.emplace_back(pending_offset{ptr_cast(ptr), pos});
      return true;
    }
    return false;
  }

  template <typename Ptr>
  std::optional<offset_t> resolve_vector_range_ptr(Ptr ptr) {
    if (vector_ranges_.empty()) {
      return std::nullopt;
    } else if (auto const vec_it = vector_ranges_.upper_bound(ptr);
               vec_it == begin(vector_ranges_)) {
      return std::nullopt;
    } else {
      auto const pred = std::prev(vec_it);
      return pred->second.contains(pred->first, ptr)
                 ? std::make_optional(pred->second.offset_of(pred->first, ptr))
                 : std::nullopt;
    }
  }

  uint64_t checksum(offset_t const from) const noexcept {
    return t_.checksum(from);
  }

  cista::raw::hash_map<void const*, offset_t> offsets_;
  std::map<void const*, vector_range> vector_ranges_;
  std::vector<pending_offset> pending_;
  Target& t_;
};

template <typename Ctx, typename T>
void serialize(Ctx& c, T const* origin, offset_t const pos) {
  using Type = decay_t<T>;
  if constexpr (std::is_union_v<Type>) {
    static_assert(std::is_standard_layout_v<Type> &&
                  std::is_trivially_copyable_v<Type>);
  } else if constexpr (is_pointer_v<Type>) {
    c.resolve_pointer(*origin, pos);
  } else if constexpr (is_indexed_v<Type>) {
    c.offsets_.emplace(origin, pos);
    serialize(c, static_cast<typename Type::value_type const*>(origin), pos);
  } else if constexpr (!std::is_scalar_v<Type>) {
    static_assert(to_tuple_works_v<Type>, "Please implement custom serializer");
    for_each_ptr_field(*origin, [&](auto& member) {
      auto const member_offset =
          static_cast<offset_t>(reinterpret_cast<intptr_t>(member) -
                                reinterpret_cast<intptr_t>(origin));
      serialize(c, member, pos + member_offset);
    });
  } else if constexpr (std::numeric_limits<Type>::is_integer) {
    c.write(pos, convert_endian<Ctx::MODE>(*origin));
  } else {
    (void)origin;
    (void)pos;
  }
}

template <typename Ctx, typename T, typename Ptr, bool Indexed,
          typename TemplateSizeType>
void serialize(Ctx& c,
               basic_vector<T, Ptr, Indexed, TemplateSizeType> const* origin,
               offset_t const pos) {
  using Type = basic_vector<T, Ptr, Indexed, TemplateSizeType>;

  auto const size = serialized_size<T>() * origin->used_size_;
  auto const start = origin->empty()
                         ? NULLPTR_OFFSET
                         : c.write(static_cast<T const*>(origin->el_), size,
                                   std::alignment_of_v<T>);

  c.write(pos + cista_member_offset(Type, el_),
          convert_endian<Ctx::MODE>(
              start == NULLPTR_OFFSET
                  ? start
                  : start - cista_member_offset(Type, el_) - pos));
  c.write(pos + cista_member_offset(Type, allocated_size_),
          convert_endian<Ctx::MODE>(origin->used_size_));
  c.write(pos + cista_member_offset(Type, used_size_),
          convert_endian<Ctx::MODE>(origin->used_size_));
  c.write(pos + cista_member_offset(Type, self_allocated_), false);

  if constexpr (Indexed) {
    if (origin->el_ != nullptr) {
      c.vector_ranges_.emplace(origin->el_, vector_range{start, size});
    }
  }

  if (origin->el_ != nullptr) {
    auto i = 0u;
    for (auto it = start; it != start + static_cast<offset_t>(size);
         it += serialized_size<T>()) {
      serialize(c, static_cast<T const*>(origin->el_ + i++), it);
    }
  }
}

template <typename Ctx, typename Ptr>
void serialize(Ctx& c, generic_string<Ptr> const* origin, offset_t const pos) {
  using Type = generic_string<Ptr>;

  if (origin->is_short()) {
    return;
  }

  auto const start = (origin->h_.ptr_ == nullptr)
                         ? NULLPTR_OFFSET
                         : c.write(origin->data(), origin->size());
  c.write(pos + cista_member_offset(Type, h_.ptr_),
          convert_endian<Ctx::MODE>(
              start == NULLPTR_OFFSET
                  ? start
                  : start - cista_member_offset(Type, h_.ptr_) - pos));
  c.write(pos + cista_member_offset(Type, h_.size_),
          convert_endian<Ctx::MODE>(origin->h_.size_));
  c.write(pos + cista_member_offset(Type, h_.self_allocated_), false);
}

template <typename Ctx, typename Ptr>
void serialize(Ctx& c, basic_string<Ptr> const* origin, offset_t const pos) {
  serialize(c, static_cast<generic_string<Ptr> const*>(origin), pos);
}

template <typename Ctx, typename Ptr>
void serialize(Ctx& c, basic_string_view<Ptr> const* origin,
               offset_t const pos) {
  serialize(c, static_cast<generic_string<Ptr> const*>(origin), pos);
}

template <typename Ctx, typename T, typename Ptr>
void serialize(Ctx& c, basic_unique_ptr<T, Ptr> const* origin,
               offset_t const pos) {
  using Type = basic_unique_ptr<T, Ptr>;

  auto const start =
      origin->el_ == nullptr
          ? NULLPTR_OFFSET
          : c.write(origin->el_, serialized_size<T>(), std::alignment_of_v<T>);

  c.write(pos + cista_member_offset(Type, el_),
          convert_endian<Ctx::MODE>(
              start == NULLPTR_OFFSET
                  ? start
                  : start - cista_member_offset(Type, el_) - pos));
  c.write(pos + cista_member_offset(Type, self_allocated_), false);

  if (origin->el_ != nullptr) {
    c.offsets_[origin->el_] = start;
    serialize(c, ptr_cast(origin->el_), start);
  }
}

template <typename Ctx, typename T, template <typename> typename Ptr,
          typename GetKey, typename GetValue, typename Hash, typename Eq>
void serialize(Ctx& c,
               hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq> const* origin,
               offset_t const pos) {
  using Type = hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq>;

  auto const start =
      origin->entries_ == nullptr
          ? NULLPTR_OFFSET
          : c.write(
                origin->entries_,
                static_cast<size_t>(origin->capacity_ * serialized_size<T>() +
                                    (origin->capacity_ + 1 + Type::WIDTH) *
                                        sizeof(typename Type::ctrl_t)),
                std::alignment_of_v<T>);
  auto const ctrl_start =
      start == NULLPTR_OFFSET
          ? c.write(Type::empty_group(), 16 * sizeof(typename Type::ctrl_t),
                    std::alignment_of_v<typename Type::ctrl_t>)
          : start +
                static_cast<offset_t>(origin->capacity_ * serialized_size<T>());

  c.write(pos + cista_member_offset(Type, entries_),
          convert_endian<Ctx::MODE>(
              start == NULLPTR_OFFSET
                  ? start
                  : start - cista_member_offset(Type, entries_) - pos));
  c.write(pos + cista_member_offset(Type, ctrl_),
          convert_endian<Ctx::MODE>(
              ctrl_start == NULLPTR_OFFSET
                  ? ctrl_start
                  : ctrl_start - cista_member_offset(Type, ctrl_) - pos));

  c.write(pos + cista_member_offset(Type, self_allocated_), false);

  c.write(pos + cista_member_offset(Type, size_),
          convert_endian<Ctx::MODE>(origin->size_));
  c.write(pos + cista_member_offset(Type, capacity_),
          convert_endian<Ctx::MODE>(origin->capacity_));
  c.write(pos + cista_member_offset(Type, growth_left_),
          convert_endian<Ctx::MODE>(origin->growth_left_));

  if (origin->entries_ != nullptr) {
    auto i = 0u;
    for (auto it = start;
         it != start + static_cast<offset_t>(origin->capacity_ *
                                             serialized_size<T>());
         it += serialized_size<T>(), ++i) {
      if (Type::is_full(origin->ctrl_[i])) {
        serialize(c, static_cast<T*>(origin->entries_ + i), it);
      }
    }
  }
}

template <typename Ctx, size_t Size>
void serialize(Ctx& c, bitset<Size> const* origin, offset_t const pos) {
  serialize(c, &origin->blocks_, pos);
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

template <typename Ctx, typename... T>
void serialize(Ctx& c, variant<T...> const* origin, offset_t const pos) {
  using Type = decay_t<decltype(*origin)>;
  c.write(pos + cista_member_offset(Type, idx_),
          convert_endian<Ctx::MODE>(origin->idx_));
  auto const offset = cista_member_offset(Type, storage_);
  origin->apply([&](auto&& t) { serialize(c, &t, pos + offset); });
}

template <typename Ctx, typename... T>
void serialize(Ctx& c, tuple<T...> const* origin,
               cista::offset_t const offset) {
  apply(
      [&](auto&&... args) {
        (serialize(c, &args,
                   offset + (reinterpret_cast<intptr_t>(&args) -
                             reinterpret_cast<intptr_t>(origin))),
         ...);
      },
      *origin);
}

template <typename Ctx, typename T, typename Tag>
void serialize(Ctx& c, strong<T, Tag> const* origin,
               cista::offset_t const offset) {
  serialize(c, &origin->v_, offset);
}

constexpr offset_t integrity_start(mode const m) noexcept {
  offset_t start = 0;
  if (is_mode_enabled(m, mode::WITH_VERSION)) {
    start += sizeof(uint64_t);
  }
  return start;
}

constexpr offset_t data_start(mode const m) noexcept {
  auto start = integrity_start(m);
  if (is_mode_enabled(m, mode::WITH_INTEGRITY)) {
    start += sizeof(uint64_t);
  }
  return start;
}

template <mode const Mode = mode::NONE, typename Target, typename T>
void serialize(Target& t, T& value) {
  serialization_context<Target, Mode> c{t};

  if constexpr (is_mode_enabled(Mode, mode::WITH_VERSION)) {
    auto const h = convert_endian<Mode>(type_hash<decay_t<T>>());
    c.write(&h, sizeof(h));
  }

  auto integrity_offset = offset_t{0};
  if constexpr (is_mode_enabled(Mode, mode::WITH_INTEGRITY)) {
    auto const h = hash_t{};
    integrity_offset = c.write(&h, sizeof(h));
  }

  serialize(c, &value,
            c.write(&value, serialized_size<T>(),
                    std::alignment_of_v<decay_t<decltype(value)>>));

  for (auto& p : c.pending_) {
    if (!c.resolve_pointer(p.origin_ptr_, p.pos_, false)) {
      printf("warning: dangling pointer at %" PRI_O " (origin=%p)\n", p.pos_,
             p.origin_ptr_);
    }
  }

  if constexpr (is_mode_enabled(Mode, mode::WITH_INTEGRITY)) {
    auto const csum =
        c.checksum(integrity_offset + static_cast<offset_t>(sizeof(hash_t)));
    c.write(integrity_offset, convert_endian<Mode>(csum));
  }
}

template <mode const Mode = mode::NONE, typename T>
byte_buf serialize(T& el) {
  auto b = buf{};
  serialize<Mode>(b, el);
  return std::move(b.buf_);
}

// =============================================================================
// DESERIALIZE
// -----------------------------------------------------------------------------
template <typename Arg, typename... Args>
Arg checked_addition(Arg a1, Args... aN) {
  using Type = decay_t<Arg>;

  auto add_if_ok = [&](Arg x) {
    if (x == 0) {
      return;
    } else if (x < 0) {
      if (a1 < std::numeric_limits<Type>::min() - x) {
        throw std::overflow_error("addition overflow");
      }
    } else if (x > 0) {
      if (a1 > std::numeric_limits<Type>::max() - x) {
        throw std::overflow_error("addition overflow");
      }
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

template <mode Mode>
struct deserialization_context {
  static constexpr auto const MODE = Mode;

  deserialization_context(uint8_t const* from, uint8_t const* to)
      : from_{reinterpret_cast<intptr_t>(from)},
        to_{reinterpret_cast<intptr_t>(to)} {}

  template <typename T>
  void convert_endian(T& el) const {
    if constexpr (endian_conversion_necessary<MODE>()) {
      el = ::cista::convert_endian<MODE>(el);
    }
  }

  template <typename Ptr>
  void deserialize_ptr(Ptr** ptr) const {
    auto const offset =
        reinterpret_cast<offset_t>(::cista::convert_endian<MODE>(*ptr));
    static_assert(is_mode_disabled(MODE, mode::_CONST),
                  "raw pointer deserialize is not const");
    *ptr = offset == NULLPTR_OFFSET
               ? nullptr
               : reinterpret_cast<Ptr*>(
                     checked_addition(reinterpret_cast<offset_t>(ptr), offset));
  }

  template <typename T>
  constexpr static size_t type_size() noexcept {
    using Type = decay_t<T>;
    if constexpr (std::is_same_v<Type, void>) {
      return 0;
    } else {
      return sizeof(Type);
    }
  }

  template <typename T>
  void check_ptr(offset_ptr<T> const& el,
                 size_t const size = type_size<T>()) const {
    if (el != nullptr) {
      checked_addition(el.offset_, reinterpret_cast<offset_t>(&el));
      check_ptr(el.get(), size);
    }
  }

  template <typename T>
  void check_ptr(T* el, size_t const size = type_size<T>()) const {
    if constexpr ((MODE & mode::UNCHECKED) == mode::UNCHECKED) {
      return;
    }

    if (el == nullptr || to_ == 0U) {
      return;
    }

    auto const pos = reinterpret_cast<intptr_t>(el);
    verify(pos >= from_, "underflow");
    verify(checked_addition(pos, static_cast<intptr_t>(size)) <= to_,
           "overflow");
    verify(size < static_cast<size_t>(std::numeric_limits<intptr_t>::max()),
           "size out of bounds");

    if constexpr (!std::is_same_v<T, void>) {
      verify((pos &
              static_cast<intptr_t>(std::alignment_of<decay_t<T>>() - 1)) == 0U,
             "ptr alignment");
    }
  }

  static void check_bool(bool const& b) {
    auto const val = *reinterpret_cast<uint8_t const*>(&b);
    verify(val <= 1U, "valid bool");
  }

  void require(bool condition, char const* msg) const {
    if constexpr ((MODE & mode::UNCHECKED) == mode::UNCHECKED) {
      return;
    }

    if (!condition) {
      throw std::runtime_error(msg);
    }
  }

  intptr_t from_, to_;
};

template <mode Mode>
struct deep_check_context : public deserialization_context<Mode> {
  using parent = deserialization_context<Mode>;

  using parent::parent;

  template <typename T>
  bool add_checked(T const* v) const {
    return checked_.emplace(type_hash<T>(), static_cast<void const*>(v)).second;
  }

  std::set<std::pair<hash_t, void const*>> mutable checked_;
};

template <typename T, mode const Mode = mode::NONE>
void check(uint8_t const* from, uint8_t const* to) {
  verify(to - from > data_start(Mode), "invalid range");

  if constexpr ((Mode & mode::WITH_VERSION) == mode::WITH_VERSION) {
    verify(convert_endian<Mode>(*reinterpret_cast<hash_t const*>(from)) ==
               type_hash<T>(),
           "invalid version");
  }

  if constexpr ((Mode & mode::WITH_INTEGRITY) == mode::WITH_INTEGRITY) {
    verify(convert_endian<Mode>(*reinterpret_cast<uint64_t const*>(
               from + integrity_start(Mode))) ==
               hash(std::string_view{
                   reinterpret_cast<char const*>(from + data_start(Mode)),
                   static_cast<size_t>(to - from - data_start(Mode))}),
           "invalid checksum");
  }
}

// --- GENERIC ---
template <typename Ctx, typename T>
void convert_endian_and_ptr(Ctx const& c, T* el) {
  using Type = decay_t<T>;
  if constexpr (std::is_pointer_v<Type>) {
    c.deserialize_ptr(el);
  } else if constexpr (std::numeric_limits<Type>::is_integer) {
    c.convert_endian(*el);
  } else {
    CISTA_UNUSED_PARAM(c)
    CISTA_UNUSED_PARAM(el)
  }
}

template <typename Ctx, typename T>
void check_state(Ctx const& c, T* el) {
  using Type = decay_t<T>;
  if constexpr (std::is_pointer_v<Type> && !std::is_same_v<Type, void*>) {
    c.check_ptr(*el);
  } else {
    CISTA_UNUSED_PARAM(c)
    CISTA_UNUSED_PARAM(el)
  }
}

template <typename Ctx, typename T, typename Fn>
void recurse(Ctx& c, T* el, Fn&& fn) {
  using Type = decay_t<T>;
  if constexpr (is_indexed_v<Type>) {
    fn(static_cast<typename T::value_type*>(el));
  } else if constexpr (std::is_aggregate_v<Type> && !std::is_union_v<Type>) {
    for_each_ptr_field(*el, [&](auto& f) { fn(f); });
  } else if constexpr (is_mode_enabled(Ctx::MODE, mode::_PHASE_II) &&
                       std::is_pointer_v<Type>) {
    if (*el != nullptr && c.add_checked(el)) {
      fn(*el);
    }
  } else {
    CISTA_UNUSED_PARAM(c)
    CISTA_UNUSED_PARAM(el)
    CISTA_UNUSED_PARAM(fn)
  }
}

template <typename Ctx, typename T>
void deserialize(Ctx const& c, T* el) {
  c.check_ptr(el);
  if constexpr (is_mode_disabled(Ctx::MODE, mode::_PHASE_II)) {
    convert_endian_and_ptr(c, el);
  }
  if constexpr (is_mode_disabled(Ctx::MODE, mode::UNCHECKED)) {
    check_state(c, el);
  }
  recurse(c, el, [&](auto* entry) { deserialize(c, entry); });
}

// --- OFFSET_PTR<T> ---
template <typename Ctx, typename T>
void convert_endian_and_ptr(Ctx const& c, offset_ptr<T>* el) {
  c.convert_endian(el->offset_);
}

template <typename Ctx, typename T>
void check_state(Ctx const& c, offset_ptr<T>* el) {
  c.check_ptr(*el);
}

template <typename Ctx, typename T, typename Fn>
void recurse(Ctx& c, offset_ptr<T>* el, Fn&& fn) {
  if constexpr (is_mode_enabled(Ctx::MODE, mode::_PHASE_II)) {
    if (*el != nullptr && c.add_checked(el)) {
      fn(static_cast<T*>(*el));
    }
  } else {
    CISTA_UNUSED_PARAM(c)
    CISTA_UNUSED_PARAM(el)
    CISTA_UNUSED_PARAM(fn)
  }
}

// --- VECTOR<T> ---
template <typename Ctx, typename T, typename Ptr, bool Indexed,
          typename TemplateSizeType>
void convert_endian_and_ptr(
    Ctx const& c, basic_vector<T, Ptr, Indexed, TemplateSizeType>* el) {
  deserialize(c, &el->el_);
  c.convert_endian(el->allocated_size_);
  c.convert_endian(el->used_size_);
}

template <typename Ctx, typename T, typename Ptr, bool Indexed,
          typename TemplateSizeType>
void check_state(Ctx const& c,
                 basic_vector<T, Ptr, Indexed, TemplateSizeType>* el) {
  c.check_ptr(el->el_,
              checked_multiplication(static_cast<size_t>(el->allocated_size_),
                                     sizeof(T)));
  c.check_bool(el->self_allocated_);
  c.require(!el->self_allocated_, "vec self-allocated");
  c.require(el->allocated_size_ == el->used_size_, "vec size mismatch");
  c.require((el->size() == 0) == (el->el_ == nullptr), "vec size=0 <=> ptr=0");
}

template <typename Ctx, typename T, typename Ptr, bool Indexed,
          typename TemplateSizeType, typename Fn>
void recurse(Ctx&, basic_vector<T, Ptr, Indexed, TemplateSizeType>* el,
             Fn&& fn) {
  for (auto& m : *el) {  // NOLINT(clang-analyzer-core.NullDereference)
    fn(&m);
  }
}

// --- STRING ---
template <typename Ctx, typename Ptr>
void convert_endian_and_ptr(Ctx const& c, generic_string<Ptr>* el) {
  if (*reinterpret_cast<uint8_t const*>(&el->s_.is_short_) == 0U) {
    deserialize(c, &el->h_.ptr_);
    c.convert_endian(el->h_.size_);
  }
}

template <typename Ctx, typename Ptr>
void check_state(Ctx const& c, generic_string<Ptr>* el) {
  c.check_bool(el->s_.is_short_);
  if (!el->is_short()) {
    c.check_ptr(el->h_.ptr_, el->h_.size_);
    c.check_bool(el->h_.self_allocated_);
    c.require(!el->h_.self_allocated_, "string self-allocated");
    c.require((el->h_.size_ == 0) == (el->h_.ptr_ == nullptr),
              "str size=0 <=> ptr=0");
  }
}

template <typename Ctx, typename Ptr, typename Fn>
void recurse(Ctx&, generic_string<Ptr>*, Fn&&) {}

template <typename Ctx, typename Ptr>
void convert_endian_and_ptr(Ctx const& c, basic_string<Ptr>* el) {
  convert_endian_and_ptr(c, static_cast<generic_string<Ptr>*>(el));
}

template <typename Ctx, typename Ptr>
void check_state(Ctx const& c, basic_string<Ptr>* el) {
  check_state(c, static_cast<generic_string<Ptr>*>(el));
}

template <typename Ctx, typename Ptr, typename Fn>
void recurse(Ctx&, basic_string<Ptr>*, Fn&&) {}

template <typename Ctx, typename Ptr>
void convert_endian_and_ptr(Ctx const& c, basic_string_view<Ptr>* el) {
  convert_endian_and_ptr(c, static_cast<generic_string<Ptr>*>(el));
}

template <typename Ctx, typename Ptr>
void check_state(Ctx const& c, basic_string_view<Ptr>* el) {
  check_state(c, static_cast<generic_string<Ptr>*>(el));
}

template <typename Ctx, typename Ptr, typename Fn>
void recurse(Ctx&, basic_string_view<Ptr>*, Fn&&) {}

// --- UNIQUE_PTR<T> ---
template <typename Ctx, typename T, typename Ptr>
void convert_endian_and_ptr(Ctx const& c, basic_unique_ptr<T, Ptr>* el) {
  deserialize(c, &el->el_);
}

template <typename Ctx, typename T, typename Ptr>
void check_state(Ctx const& c, basic_unique_ptr<T, Ptr>* el) {
  c.check_bool(el->self_allocated_);
  c.require(!el->self_allocated_, "unique_ptr self-allocated");
}

template <typename Ctx, typename T, typename Ptr, typename Fn>
void recurse(Ctx&, basic_unique_ptr<T, Ptr>* el, Fn&& fn) {
  if (el->el_ != nullptr) {
    fn(static_cast<T*>(el->el_));
  }
}

// --- HASH_STORAGE<T> ---
template <typename Ctx, typename T, template <typename> typename Ptr,
          typename GetKey, typename GetValue, typename Hash, typename Eq>
void convert_endian_and_ptr(
    Ctx const& c, hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq>* el) {
  deserialize(c, &el->entries_);
  deserialize(c, &el->ctrl_);
  c.convert_endian(el->size_);
  c.convert_endian(el->capacity_);
  c.convert_endian(el->growth_left_);
}

template <typename Ctx, typename T, template <typename> typename Ptr,
          typename GetKey, typename GetValue, typename Hash, typename Eq>
void check_state(Ctx const& c,
                 hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq>* el) {
  using Type = decay_t<remove_pointer_t<decltype(el)>>;
  c.require(el->ctrl_ != nullptr, "hash storage: ctrl must be set");
  c.check_ptr(
      el->entries_,
      checked_addition(
          checked_multiplication(
              el->capacity_, static_cast<typename Type::size_type>(sizeof(T))),
          checked_addition(el->capacity_, 1U, Type::WIDTH)));
  c.check_ptr(el->ctrl_, checked_addition(el->capacity_, 1U, Type::WIDTH));
  c.require(el->entries_ == nullptr ||
                reinterpret_cast<uint8_t const*>(ptr_cast(el->ctrl_)) ==
                    reinterpret_cast<uint8_t const*>(ptr_cast(el->entries_)) +
                        checked_multiplication(
                            static_cast<size_t>(el->capacity_), sizeof(T)),
            "hash storage: entries!=null -> ctrl = entries+capacity");
  c.require(
      (el->entries_ == nullptr) == (el->capacity_ == 0U && el->size_ == 0U),
      "hash storage: entries=null <=> size=capacity=0");

  c.check_bool(el->self_allocated_);
  c.require(!el->self_allocated_, "hash storage: self-allocated");

  c.require(el->ctrl_[el->capacity_] == Type::END,
            "hash storage: end ctrl byte");
  c.require(std::all_of(ptr_cast(el->ctrl_),
                        ptr_cast(el->ctrl_) + el->capacity_ + 1U + Type::WIDTH,
                        [](typename Type::ctrl_t const ctrl) {
                          return Type::is_empty(ctrl) ||
                                 Type::is_deleted(ctrl) ||
                                 Type::is_full(ctrl) ||
                                 ctrl == Type::ctrl_t::END;
                        }),
            "hash storage: ctrl bytes must be empty or deleted or full");

  using st_t = typename Type::size_type;
  auto [total_empty, total_full, total_deleted, total_growth_left] =
      std::accumulate(
          ptr_cast(el->ctrl_), ptr_cast(el->ctrl_) + el->capacity_,
          std::tuple{st_t{0U}, st_t{0U}, st_t{0U}, st_t{0}},
          [&](std::tuple<st_t, st_t, st_t, st_t> const acc,
              typename Type::ctrl_t const& ctrl) {
            auto const [empty, full, deleted, growth_left] = acc;
            return std::tuple{
                Type::is_empty(ctrl) ? empty + 1 : empty,
                Type::is_full(ctrl) ? full + 1 : full,
                Type::is_deleted(ctrl) ? deleted + 1 : deleted,
                (Type::is_empty(ctrl) && el->was_never_full(static_cast<st_t>(
                                             &ctrl - el->ctrl_))
                     ? growth_left + 1
                     : growth_left)};
          });

  c.require(el->size_ == total_full, "hash storage: size");
  c.require(total_empty + total_full + total_deleted == el->capacity_,
            "hash storage: empty + full + deleted = capacity");
  c.require(std::min(Type::capacity_to_growth(el->capacity_) - el->size_,
                     total_growth_left) <= el->growth_left_,
            "hash storage: growth left");
}

template <typename Ctx, typename T, template <typename> typename Ptr,
          typename GetKey, typename GetValue, typename Hash, typename Eq,
          typename Fn>
void recurse(Ctx&, hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq>* el,
             Fn&& fn) {
  for (auto& m : *el) {
    fn(&m);
  }
}

// --- BITSET<SIZE> ---
template <typename Ctx, size_t Size, typename Fn>
void recurse(Ctx&, bitset<Size>* el, Fn&& fn) {
  fn(&el->blocks_);
}

// --- ARRAY<T> ---
template <typename Ctx, typename T, size_t Size, typename Fn>
void recurse(Ctx&, array<T, Size>* el, Fn&& fn) {
  for (auto& m : *el) {
    fn(&m);
  }
}

// --- VARIANT<T...> ---
template <typename Ctx, typename Fn, typename... T>
void convert_endian_and_ptr(Ctx const& c, variant<T...>* el) {
  c.convert_endian(el->idx_);
}

template <typename Ctx, typename Fn, typename... T>
void recurse(Ctx&, variant<T...>* el, Fn&& fn) {
  el->apply([&](auto&& t) { fn(&t); });
}

template <typename Ctx, typename... T>
void check_state(Ctx const& c, variant<T...>* el) {
  c.require(el->index() < sizeof...(T), "variant index");
}

// --- TUPLE<T...> ---
template <typename Ctx, typename... T>
void recurse(Ctx const& c, tuple<T...>* el) {
  apply([&](auto&&... args) { (deserialize(c, &args), ...); }, *el);
}

template <typename T, mode const Mode = mode::NONE>
T* deserialize(uint8_t* from, uint8_t* to = nullptr) {
  if constexpr (is_mode_enabled(Mode, mode::CAST)) {
    CISTA_UNUSED_PARAM(to)
    return reinterpret_cast<T*>(from);
  } else {
    check<T, Mode>(from, to);
    auto const el = reinterpret_cast<T*>(from + data_start(Mode));

    deserialization_context<Mode> c{from, to};
    deserialize(c, el);

    if constexpr ((Mode & mode::DEEP_CHECK) == mode::DEEP_CHECK) {
      deep_check_context<Mode | mode::_PHASE_II> c1{from, to};
      deserialize(c1, el);
    }

    return el;
  }
}

template <typename T, mode const Mode = mode::NONE>
T const* deserialize(uint8_t const* from, uint8_t const* to = nullptr) {
  static_assert(!endian_conversion_necessary<Mode>(), "cannot be const");
  return deserialize<T, Mode | mode::_CONST>(const_cast<uint8_t*>(from),
                                             const_cast<uint8_t*>(to));
}

template <typename T, mode const Mode = mode::NONE, typename CharT>
T const* deserialize(CharT const* from, CharT const* to = nullptr) {
  static_assert(sizeof(CharT) == 1U, "byte size entries");
  return deserialize<T, Mode>(reinterpret_cast<uint8_t const*>(from),
                              reinterpret_cast<uint8_t const*>(to));
}

template <typename T, mode const Mode = mode::NONE, typename CharT>
T* deserialize(CharT* from, CharT* to = nullptr) {
  static_assert(sizeof(CharT) == 1U, "byte size entries");
  return deserialize<T, Mode>(reinterpret_cast<uint8_t*>(from),
                              reinterpret_cast<uint8_t*>(to));
}

template <typename T, mode const Mode = mode::NONE>
T const* deserialize(std::string_view c) {
  return deserialize<T const, Mode>(&c[0], &c[0] + c.size());
}

template <typename T, mode const Mode = mode::NONE, typename Container>
auto deserialize(Container& c) {
  return deserialize<T, Mode>(&c[0], &c[0] + c.size());
}

template <typename T, mode const Mode = mode::NONE>
T* unchecked_deserialize(uint8_t* from, uint8_t* to = nullptr) {
  return deserialize<T, Mode | mode::UNCHECKED>(from, to);
}

template <typename T, mode const Mode = mode::NONE, typename Container>
T* unchecked_deserialize(Container& c) {
  return unchecked_deserialize<T, Mode>(&c[0], &c[0] + c.size());
}

template <typename T, mode const Mode = mode::NONE>
T copy_from_potentially_unaligned(std::string_view buf) {
  struct aligned {
    explicit aligned(std::string_view buf)
        : mem_{static_cast<std::uint8_t*>(
              CISTA_ALIGNED_ALLOC(sizeof(max_align_t), buf.size()))} {
      verify(mem_ != nullptr, "failed to allocate aligned memory");
      std::memcpy(mem_, buf.data(), buf.size());
    }
    ~aligned() { CISTA_ALIGNED_FREE(sizeof(max_align_t), mem_); }
    std::uint8_t* mem_;
  };

  auto const is_already_aligned =
      (reinterpret_cast<std::uintptr_t>(buf.data()) % sizeof(max_align_t)) == 0;
  if (is_already_aligned) {
    return *deserialize<T, Mode>(buf);
  } else {
    auto copy = aligned{buf};
    return *deserialize<T, Mode>(copy.mem_, copy.mem_ + buf.size());
  }
}

namespace raw {
using cista::deserialize;
using cista::unchecked_deserialize;
}  // namespace raw

namespace offset {
using cista::deserialize;
using cista::unchecked_deserialize;
}  // namespace offset

}  // namespace cista

#undef cista_member_offset
