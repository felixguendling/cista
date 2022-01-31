#pragma once

#include <type_traits>
#include <utility>

#include "cista/decay.h"

namespace cista {

template <typename Head, typename... Tail>
struct tuple : public tuple<Tail...> {
  tuple() = default;
  tuple(Head&& first, Tail&&... tail)
      : tuple<Tail...>{std::forward<Tail>(tail)...},
        head_{std::forward<Head>(first)} {}
  Head head_;
};

template <typename Head>
struct tuple<Head> {
  tuple() = default;
  explicit tuple(Head&& first) : head_{std::forward<Head>(first)} {}
  Head head_;
};

template <typename Head, typename... Tail>
tuple(Head&& first, Tail&&... tail) -> tuple<Head, Tail...>;

template <typename Tuple>
struct is_tuple : std::false_type {};

template <typename... T>
struct is_tuple<tuple<T...>> : std::true_type {};

template <typename T>
inline constexpr auto is_tuple_v = is_tuple<T>::value;

template <size_t I, typename Head, typename... Tail,
          std::enable_if_t<I == 0U, int> = 0>
Head& get(tuple<Head, Tail...>& t) {
  return t.head_;
}

template <size_t I, typename Head, typename... Tail,
          std::enable_if_t<I != 0U, int> = 0>
auto& get(tuple<Head, Tail...>& t) {
  return get<I - 1U, Tail...>(static_cast<tuple<Tail...>&>(t));
}

template <size_t I, typename Head, typename... Tail,
          std::enable_if_t<I == 0U, int> = 0>
Head const& get(tuple<Head, Tail...> const& t) {
  return t.head_;
}

template <size_t I, typename Head, typename... Tail,
          std::enable_if_t<I != 0U, int> = 0>
auto const& get(tuple<Head, Tail...> const& t) {
  return get<I - 1U, Tail...>(static_cast<tuple<Tail...> const&>(t));
}

template <size_t I, typename Head, typename... Tail,
          std::enable_if_t<I == 0U, int> = 0>
Head&& get(tuple<Head, Tail...>&& t) {
  return t.head_;
}

template <size_t I, typename Head, typename... Tail,
          std::enable_if_t<I != 0U, int> = 0>
auto&& get(tuple<Head, Tail...>&& t) {
  return get<I - 1U, Tail...>(static_cast<tuple<Tail...>&&>(t));
}

template <typename T>
struct tuple_size;

template <typename... T>
struct tuple_size<tuple<T...>>
    : public std::integral_constant<std::size_t, sizeof...(T)> {};

template <typename T>
inline constexpr std::size_t tuple_size_v = tuple_size<std::decay_t<T>>::value;

template <std::size_t I, typename T>
struct tuple_element;

template <std::size_t I, typename Head, typename... Tail>
struct tuple_element<I, tuple<Head, Tail...>>
    : tuple_element<I - 1, tuple<Tail...>> {};

template <typename Head, typename... Tail>
struct tuple_element<0, tuple<Head, Tail...>> {
  using type = Head;
};

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
struct tuple_size<cista::tuple<Pack...>> : cista::tuple_size<cista::tuple<Pack...>> {};

template <std::size_t I, typename... Pack>
struct tuple_element<I, cista::tuple<Pack...>> : cista::tuple_element<I, cista::tuple<Pack...>> {};

} // namespace std