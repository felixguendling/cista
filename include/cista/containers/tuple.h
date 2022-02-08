#pragma once

#include <cinttypes>
#include <type_traits>
#include <utility>

#include "cista/decay.h"

namespace cista {

template <typename... Ts>
struct tuple;

template <std::size_t I, typename... Ts>
struct tuple_element;

template <std::size_t I, typename T, typename... Ts>
struct tuple_element<I, tuple<T, Ts...>> : tuple_element<I - 1, tuple<Ts...>> {
};

template <typename T, typename... Ts>
struct tuple_element<0, tuple<T, Ts...>> {
  using type = T;
};

template <std::size_t I, typename T, typename... Ts>
struct tuple_element<I, T, Ts...> : tuple_element<I - 1, Ts...> {};

template <typename T, typename... Ts>
struct tuple_element<0, T, Ts...> {
  using type = T;
};

template <std::size_t I, typename... Ts>
using tuple_element_t = typename tuple_element<I, Ts...>::type;

template <typename T, typename... Ts>
constexpr std::size_t max_align_of() {
  if constexpr (sizeof...(Ts) == 0U) {
    return alignof(T);
  } else {
    return std::max(alignof(T), max_align_of<Ts...>());
  }
}

template <typename T, typename... Ts>
constexpr std::size_t get_offset(std::size_t const current_idx,
                                 std::size_t current_offset = 0) {
  if (auto const misalign = current_offset % alignof(T); misalign != 0) {
    current_offset += (alignof(T) - misalign) % alignof(T);
  }

  if (current_idx == 0) {
    return current_offset;
  }

  current_offset += sizeof(T);

  if constexpr (sizeof...(Ts) == 0) {
    return current_idx == 1 ? current_offset + sizeof(T) : current_offset;
  } else {
    return get_offset<Ts...>(current_idx - 1, current_offset);
  }
}

template <typename... Ts>
constexpr std::size_t get_total_size() {
  return get_offset<Ts...>(sizeof...(Ts) + 1);
}

template <std::size_t I, typename T, typename... Ts>
constexpr auto get_arg(T&& arg, Ts&&... args) {
  if constexpr (I == 0U) {
    return std::forward<T>(arg);
  } else {
    return get_arg<I - 1>(std::forward<Ts>(args)...);
  }
}

template <std::size_t I, typename... Ts>
auto& get(tuple<Ts...>& t) {
  using return_t = tuple_element_t<I, Ts...>;
  return *std::launder(reinterpret_cast<return_t*>(t.template get_ptr<I>()));
}

template <std::size_t I, typename... Ts>
auto const& get(tuple<Ts...> const& t) {
  using return_t = tuple_element_t<I, Ts...>;
  return *std::launder(
      reinterpret_cast<return_t const*>(t.template get_ptr<I>()));
}

template <std::size_t I, typename... Ts>
auto& get(tuple<Ts...>&& t) {
  using return_t = tuple_element_t<I, Ts...>;
  return *std::launder(reinterpret_cast<return_t*>(t.template get_ptr<I>()));
}

template <typename... Ts>
struct tuple {
  template <std::size_t... Is>
  using seq_t = std::index_sequence<Is...>;
  static constexpr auto Indices = std::make_index_sequence<sizeof...(Ts)>{};

  tuple() { default_construct(Indices); }

  tuple(tuple const& o) { copy_construct(o, Indices); }

  tuple& operator=(tuple const& o) {
    if (&o != this) {
      copy_assign(o, Indices);
    }

    return *this;
  }

  tuple(tuple&& o) noexcept { move_construct(o, Indices); }

  tuple& operator=(tuple&& o) noexcept {
    move_assign(o, Indices);
    return *this;
  }

  tuple(Ts&&... args) {  // NOLINT
    move_construct_from_args(Indices, std::forward<Ts>(args)...);
  }

  ~tuple() { destruct(Indices); }

  template <std::size_t... Is>
  constexpr void move_construct_from_args(seq_t<Is...>, Ts&&... args) {
    (new (get_ptr<Is>())
         tuple_element_t<Is, Ts...>(get_arg<Is>(std::forward<Ts>(args)...)),
     ...);
  }

  template <std::size_t... Is>
  constexpr void default_construct(seq_t<Is...>) {
    ((new (get_ptr<Is>()) tuple_element_t<Is, Ts...>{}), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void copy_construct(From const& from, seq_t<Is...>) {
    (new (get_ptr<Is>()) tuple_element_t<Is, Ts...>(get<Is>(from)), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void copy_assign(From const& from, seq_t<Is...>) {
    ((get<Is>(*this) = get<Is>(from)), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void move_construct(From&& from, seq_t<Is...>) {
    (new (get_ptr<Is>()) tuple_element_t<Is, Ts...>(std::move(get<Is>(from))),
     ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void move_assign(From&& from, seq_t<Is...>) {
    ((get<Is>(*this) = std::move(get<Is>(from))), ...);
  }

  template <typename T>
  constexpr void destruct(T& t) {
    static_assert(!std::is_array_v<T>);
    t.~T();
  }

  template <std::size_t... Is>
  constexpr void destruct(seq_t<Is...>) {
    (destruct(get<Is>(*this)), ...);
  }

  template <std::size_t I>
  constexpr auto get_ptr() {
    return reinterpret_cast<char*>(&mem_) + get_offset<Ts...>(I);
  }

  template <std::size_t I>
  constexpr auto get_ptr() const {
    return reinterpret_cast<char const*>(&mem_) + get_offset<Ts...>(I);
  }

  std::aligned_storage_t<get_total_size<Ts...>(), max_align_of<Ts...>()> mem_;
};

template <typename Head, typename... Tail>
tuple(Head&& first, Tail&&... tail) -> tuple<Head, Tail...>;

template <typename Tuple>
struct is_tuple : std::false_type {};

template <typename... T>
struct is_tuple<tuple<T...>> : std::true_type {};

template <typename T>
inline constexpr auto is_tuple_v = is_tuple<T>::value;

template <typename T>
struct tuple_size;

template <typename... T>
struct tuple_size<tuple<T...>>
    : public std::integral_constant<std::size_t, sizeof...(T)> {};

template <typename T>
inline constexpr std::size_t tuple_size_v = tuple_size<std::decay_t<T>>::value;

template <typename F, typename Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(std::index_sequence<I...>, F&& f,
                                    Tuple&& t) {
  return std::invoke(std::forward<F>(f), get<I>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t) {
  return apply_impl(std::make_index_sequence<tuple_size_v<Tuple>>{},
                    std::forward<F>(f), std::forward<Tuple>(t));
}

template <typename F, typename Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(std::index_sequence<I...>, F&& f, Tuple&& a,
                                    Tuple&& b) {
  return (std::invoke(std::forward<F>(f), get<I>(std::forward<Tuple>(a)),
                      get<I>(std::forward<Tuple>(b))),
          ...);
}

template <typename F, typename Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& a, Tuple&& b) {
  return apply_impl(
      std::make_index_sequence<tuple_size_v<std::remove_reference_t<Tuple>>>{},
      std::forward<F>(f), std::forward<Tuple>(a), std::forward<Tuple>(b));
}

template <typename Tuple, std::size_t... I>
constexpr decltype(auto) eq(std::index_sequence<I...>, Tuple&& a, Tuple&& b) {
  return ((get<I>(std::forward<Tuple>(a)) == get<I>(std::forward<Tuple>(b))) &&
          ...);
}

template <typename Tuple>
std::enable_if_t<is_tuple_v<decay_t<Tuple>>, bool> operator==(Tuple&& a,
                                                              Tuple&& b) {
  return eq(
      std::make_index_sequence<tuple_size_v<std::remove_reference_t<Tuple>>>{},
      std::forward<Tuple>(a), std::forward<Tuple>(b));
}

template <typename Tuple>
std::enable_if_t<is_tuple_v<decay_t<Tuple>>, bool> operator!=(Tuple&& a,
                                                              Tuple&& b) {
  return !(a == b);
}

template <typename Tuple, std::size_t Index = 0U>
bool lt(Tuple&& a, Tuple&& b) {
  if constexpr (Index == tuple_size_v<Tuple>) {
    return false;
  } else {
    if (get<Index>(std::forward<Tuple>(a)) <
        get<Index>(std::forward<Tuple>(b))) {
      return true;
    }
    if (get<Index>(std::forward<Tuple>(b)) <
        get<Index>(std::forward<Tuple>(a))) {
      return false;
    }
    return lt<Tuple, Index + 1>(std::forward<Tuple>(a), std::forward<Tuple>(b));
  }
}

template <typename Tuple>
std::enable_if_t<is_tuple_v<decay_t<Tuple>>, bool> operator<(Tuple&& a,
                                                             Tuple&& b) {
  return lt(a, b);
}

template <typename Tuple>
std::enable_if_t<is_tuple_v<decay_t<Tuple>>, bool> operator<=(Tuple&& a,
                                                              Tuple&& b) {
  return !(b < a);
}

template <typename Tuple>
std::enable_if_t<is_tuple_v<decay_t<Tuple>>, bool> operator>(Tuple&& a,
                                                             Tuple&& b) {
  return b < a;
}

template <typename Tuple>
std::enable_if_t<is_tuple_v<decay_t<Tuple>>, bool> operator>=(Tuple&& a,
                                                              Tuple&& b) {
  return !(a < b);
}

}  // namespace cista

namespace std {

template <typename... Pack>
struct tuple_size<cista::tuple<Pack...>>
    : cista::tuple_size<cista::tuple<Pack...>> {};

template <std::size_t I, typename... Pack>
struct tuple_element<I, cista::tuple<Pack...>>
    : cista::tuple_element<I, cista::tuple<Pack...>> {};

}  // namespace std