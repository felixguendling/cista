#pragma once

#include <chrono>
#include <map>

#include "cista/containers.h"
#include "cista/decay.h"
#include "cista/hash.h"
#include "cista/indexed.h"
#include "cista/reflection/for_each_field.h"
#include "cista/type_hash/type_name.h"

namespace cista {

template <typename T>
hash_t type2str_hash() noexcept {
  return hash_combine(hash(canonical_type_str<decay_t<T>>()), sizeof(T));
}

template <typename T>
hash_t type_hash(T const&, hash_t, std::map<hash_t, unsigned>&) noexcept;

template <typename Rep, typename Period>
hash_t type_hash(std::chrono::duration<Rep, Period> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("duration"));
  h = type_hash(Rep{}, h, done);
  h = hash_combine(hash(canonical_type_str<Period>()), h);
  return h;
}

template <typename Clock, typename Duration>
hash_t type_hash(std::chrono::time_point<Clock, Duration> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("timepoint"));
  h = type_hash(Duration{}, h, done);
  h = hash_combine(hash(canonical_type_str<Clock>()), h);
  return h;
}

template <typename T, std::size_t Size>
hash_t type_hash(array<T, Size> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("array"));
  h = hash_combine(h, Size);
  return type_hash(T{}, h, done);
}

template <typename T>
hash_t type_hash(T const& el, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  using Type = decay_t<T>;

  auto const base_hash = type2str_hash<Type>();
  auto [it, inserted] =
      done.try_emplace(base_hash, static_cast<unsigned>(done.size()));
  if (!inserted) {
    return hash_combine(h, it->second);
  }

  if constexpr (is_pointer_v<Type>) {
    using PointeeType = remove_pointer_t<Type>;
    if constexpr (std::is_same_v<PointeeType, void>) {
      return hash_combine(h, hash("void*"));
    } else {
      return type_hash(remove_pointer_t<Type>{},
                       hash_combine(h, hash("pointer")), done);
    }
  } else if constexpr (std::is_integral_v<Type>) {
    return hash_combine(h, hash("i"), sizeof(Type));
  } else if constexpr (std::is_scalar_v<Type>) {
    return hash_combine(h, type2str_hash<T>());
  } else {
    static_assert(to_tuple_works_v<Type>, "Please implement custom type hash.");
    h = hash_combine(h, hash("struct"));
    for_each_field(el, [&](auto const& member) noexcept {
      h = type_hash(member, h, done);
    });
    return h;
  }
}

template <typename A, typename B>
hash_t type_hash(pair<A, B> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = type_hash(A{}, h, done);
  h = type_hash(B{}, h, done);
  return hash_combine(h, hash("pair"));
}

template <typename T, template <typename> typename Ptr, bool Indexed,
          typename TemplateSizeType>
hash_t type_hash(basic_vector<T, Ptr, Indexed, TemplateSizeType> const&,
                 hash_t h, std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("vector"));
  return type_hash(T{}, h, done);
}

template <typename T, typename Ptr>
hash_t type_hash(basic_unique_ptr<T, Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("unique_ptr"));
  return type_hash(T{}, h, done);
}

template <typename T, template <typename> typename Ptr, typename GetKey,
          typename GetValue, typename Hash, typename Eq>
hash_t type_hash(hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq> const&,
                 hash_t h, std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("hash_storage"));
  return type_hash(T{}, h, done);
}

template <typename... T>
hash_t type_hash(variant<T...> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("variant"));
  ((h = type_hash(T{}, h, done)), ...);
  return h;
}

template <typename... T>
hash_t type_hash(tuple<T...> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("tuple"));
  ((h = type_hash(T{}, h, done)), ...);
  return h;
}

template <typename Ptr>
hash_t type_hash(generic_string<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) noexcept {
  return hash_combine(h, hash("string"));
}

template <typename Ptr>
hash_t type_hash(basic_string<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) noexcept {
  return hash_combine(h, hash("string"));
}

template <typename Ptr>
hash_t type_hash(basic_string_view<Ptr> const&, hash_t h,
                 std::map<hash_t, unsigned>&) noexcept {
  return hash_combine(h, hash("string"));
}

template <typename T>
hash_t type_hash(indexed<T> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  return type_hash(T{}, h, done);
}

template <typename T, typename Tag>
hash_t type_hash(strong<T, Tag> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("strong"));
  h = type_hash(T{}, h, done);
  h = hash_combine(hash(canonical_type_str<Tag>()), h);
  return h;
}

template <typename T>
hash_t type_hash(optional<T> const&, hash_t h,
                 std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("optional"));
  h = type_hash(T{}, h, done);
  return h;
}

template <typename T, typename SizeType, template <typename> typename Vec,
          std::size_t Log2MaxEntriesPerBucket>
hash_t type_hash(
    dynamic_fws_multimap_base<T, SizeType, Vec, Log2MaxEntriesPerBucket> const&,
    hash_t h, std::map<hash_t, unsigned>& done) noexcept {
  h = hash_combine(h, hash("dynamic_fws_multimap"));
  h = type_hash(Vec<SizeType>{}, h, done);
  h = type_hash(Vec<T>{}, h, done);
  h = hash_combine(Log2MaxEntriesPerBucket, h);
  return h;
}

template <typename T>
hash_t type_hash() {
  auto done = std::map<hash_t, unsigned>{};
  return type_hash(T{}, BASE_HASH, done);
}

}  // namespace cista
