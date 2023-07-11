#pragma once

#include "cista/containers.h"
#include "cista/decay.h"
#include "cista/hash.h"
#include "cista/indexed.h"
#include "cista/reflection/to_tuple.h"
#include "cista/type_hash/type_name.h"

namespace cista {

template <typename T>
constexpr hash_t static_type2str_hash() noexcept {
  return hash_combine(hash(type_str<decay_t<T>>()), sizeof(T));
}

template <typename T>
constexpr auto to_member_type_list() {
  auto t = T{};
  return cista::to_tuple(t);
}

template <typename T>
using tuple_representation_t = decltype(to_member_type_list<T>());

template <typename K, typename V, std::size_t Size>
struct count_map {
  using key_type = K;
  using mapped_type = V;
  struct value_type {
    key_type first;
    mapped_type second;
  };
  using const_iterator = typename std::array<value_type, Size>::const_iterator;

  constexpr count_map() = default;

  constexpr std::pair<mapped_type, bool> add(key_type k) noexcept {
    auto const it = find(k);
    if (it == end()) {
      arr_[size_] = value_type{k, static_cast<mapped_type>(size_)};
      auto const index = size_;
      ++size_;
      return {static_cast<mapped_type>(index), true};
    } else {
      return {it->second, false};
    }
  }

  constexpr const_iterator find(key_type k) const noexcept {
    for (auto it = begin(); it != end(); ++it) {
      if (it->first == k) {
        return it;
      }
    }
    return end();
  }

  constexpr const_iterator begin() const noexcept { return std::begin(arr_); }
  constexpr const_iterator end() const noexcept {
    return std::next(
        begin(),
        static_cast<
            typename std::iterator_traits<const_iterator>::difference_type>(
            size_));
  }

  std::size_t size_{0U};
  std::array<value_type, Size> arr_{};
};

template <std::size_t NMaxTypes>
struct hash_data {
  constexpr hash_data combine(hash_t const h) const noexcept {
    hash_data r;
    r.done_ = done_;
    r.h_ = hash_combine(h_, h);
    return r;
  }
  count_map<hash_t, unsigned, NMaxTypes> done_;
  hash_t h_{BASE_HASH};
};

template <typename T>
constexpr T* null() noexcept {
  return static_cast<T*>(nullptr);
}

template <typename T, std::size_t NMaxTypes>
constexpr hash_data<NMaxTypes> static_type_hash(T const*,
                                                hash_data<NMaxTypes>) noexcept;

template <typename Tuple, std::size_t NMaxTypes, std::size_t I>
constexpr auto hash_tuple_element(hash_data<NMaxTypes> const h) noexcept {
  using element_type = std::decay_t<std::tuple_element_t<I, Tuple>>;
  return static_type_hash(null<element_type>(), h);
}

template <typename Tuple, std::size_t NMaxTypes, std::size_t... I>
constexpr auto hash_tuple(Tuple const*, hash_data<NMaxTypes> h,
                          std::index_sequence<I...>) noexcept {
  ((h = hash_tuple_element<Tuple, NMaxTypes, I>(h)), ...);
  return h;
}

template <typename T, std::size_t NMaxTypes>
constexpr hash_data<NMaxTypes> static_type_hash(
    T const*, hash_data<NMaxTypes> h) noexcept {
  using Type = decay_t<T>;

  auto const base_hash = static_type2str_hash<Type>();
  auto const [ordering, inserted] = h.done_.add(base_hash);
  if (!inserted) {
    return h.combine(ordering);
  }

  if constexpr (is_pointer_v<Type>) {
    using PointeeType = remove_pointer_t<Type>;
    if constexpr (std::is_same_v<std::remove_const_t<PointeeType>, void>) {
      return h.combine(hash("void*"));
    } else {
      h = h.combine(hash("pointer"));
      return static_type_hash(static_cast<remove_pointer_t<Type>*>(nullptr), h);
    }
  } else if constexpr (std::is_integral_v<Type>) {
    return h.combine(hash("i")).combine(sizeof(Type));
  } else if constexpr (std::is_scalar_v<Type>) {
    return h.combine(static_type2str_hash<T>());
  } else {
    static_assert(to_tuple_works_v<Type>, "Please implement custom type hash.");
    using tuple_t = tuple_representation_t<T>;
    return hash_tuple<tuple_t>(
        null<tuple_t>(), h.combine(hash("struct")),
        std::make_index_sequence<std::tuple_size_v<tuple_t>>());
  }
}

template <typename Rep, typename Period, std::size_t NMaxTypes>
constexpr auto static_type_hash(std::chrono::duration<Rep, Period> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("duration"));
  h = static_type_hash(null<Rep>(), h);
  h = static_type_hash(null<Period>(), h);
  return h;
}

template <typename A, typename B, std::size_t NMaxTypes>
constexpr auto static_type_hash(std::pair<A, B> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("pair"));
  h = static_type_hash(null<A>(), h);
  h = static_type_hash(null<B>(), h);
  return h;
}

template <typename Clock, typename Duration, std::size_t NMaxTypes>
constexpr auto static_type_hash(std::chrono::time_point<Clock, Duration> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("timepoint"));
  h = static_type_hash(null<Duration>(), h);
  h = static_type_hash(null<Clock>(), h);
  return h;
}

template <typename T, std::size_t Size, std::size_t NMaxTypes>
constexpr auto static_type_hash(array<T, Size> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = static_type_hash(null<T>(), h);
  return h.combine(hash("array")).combine(Size);
}

template <typename T, template <typename> typename Ptr, bool Indexed,
          typename TemplateSizeType, std::size_t NMaxTypes>
constexpr auto static_type_hash(
    basic_vector<T, Ptr, Indexed, TemplateSizeType> const*,
    hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("vector"));
  return static_type_hash(null<T>(), h);
}

template <typename T, typename Ptr, std::size_t NMaxTypes>
constexpr auto static_type_hash(basic_unique_ptr<T, Ptr> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("unique_ptr"));
  return static_type_hash(null<T>(), h);
}

template <typename T, template <typename> typename Ptr, typename GetKey,
          typename GetValue, typename Hash, typename Eq, std::size_t NMaxTypes>
constexpr auto static_type_hash(
    hash_storage<T, Ptr, GetKey, GetValue, Hash, Eq> const*,
    hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("hash_storage"));
  return static_type_hash(null<T>(), h);
}

template <std::size_t NMaxTypes, typename... T>
constexpr auto static_type_hash(variant<T...> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("variant"));
  ((h = static_type_hash(null<T>(), h)), ...);
  return h;
}

template <std::size_t NMaxTypes, typename... T>
constexpr auto static_type_hash(tuple<T...> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("tuple"));
  ((h = static_type_hash(null<T>(), h)), ...);
  return h;
}

template <typename Ptr, std::size_t NMaxTypes>
constexpr auto static_type_hash(generic_string<Ptr> const*,
                                hash_data<NMaxTypes> h) noexcept {
  return h.combine(hash("string"));
}

template <typename Ptr, std::size_t NMaxTypes>
constexpr auto static_type_hash(basic_string<Ptr> const*,
                                hash_data<NMaxTypes> h) noexcept {
  return h.combine(hash("string"));
}

template <typename Ptr, std::size_t NMaxTypes>
constexpr auto static_type_hash(basic_string_view<Ptr> const*,
                                hash_data<NMaxTypes> h) noexcept {
  return h.combine(hash("string"));
}

template <typename T, std::size_t NMaxTypes>
constexpr auto static_type_hash(indexed<T> const*,
                                hash_data<NMaxTypes> h) noexcept {
  return static_type_hash(null<T>(), h);
}

template <typename T, typename Tag, std::size_t NMaxTypes>
constexpr auto static_type_hash(strong<T, Tag> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = static_type_hash(null<T>(), h);
  return h;
}

template <typename T, std::size_t NMaxTypes>
constexpr auto static_type_hash(optional<T> const*,
                                hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("optional"));
  h = static_type_hash(null<T>(), h);
  return h;
}

template <typename T, typename SizeType, template <typename> typename Vec,
          std::size_t Log2MaxEntriesPerBucket, std::size_t NMaxTypes>
constexpr auto static_type_hash(
    dynamic_fws_multimap_base<T, SizeType, Vec, Log2MaxEntriesPerBucket> const*,
    hash_data<NMaxTypes> h) noexcept {
  h = h.combine(hash("dynamic_fws_multimap"));
  h = static_type_hash(null<Vec<SizeType>>(), h);
  h = static_type_hash(null<Vec<T>>(), h);
  h = h.combine(Log2MaxEntriesPerBucket);
  return h;
}

template <typename T, std::size_t NMaxTypes = 128U>
constexpr hash_t static_type_hash() noexcept {
  return static_type_hash(null<T>(), hash_data<NMaxTypes>{}).h_;
}

}  // namespace cista
