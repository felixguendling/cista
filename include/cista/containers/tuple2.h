#pragma once

#include <memory>

namespace cista {

template <typename... Ts>
struct tuple2;

template <typename... Ts>
struct max_align_of;

template <typename T, typename... Ts>
struct max_align_of<T, Ts...>
    : std::conditional_t<(alignof(T) > max_align_of<Ts...>::value),
                         std::integral_constant<std::size_t, alignof(T)>,
                         max_align_of<Ts...>> {};

template <>
struct max_align_of<> : std::integral_constant<std::size_t, 0> {};

template <typename... Ts>
constexpr static auto max_align_of_v = max_align_of<Ts...>::value;

constexpr std::size_t get_alignment_padding(std::size_t const size,
                                            std::size_t const max_align) {
  return (max_align - (size % max_align)) % max_align;
}

template <typename T>
constexpr auto get_size_in_tuple(std::size_t const max_align) {
  return sizeof(T) + get_alignment_padding(sizeof(T), max_align);
}

template <typename... Ts>
constexpr auto get_total_size() {
  constexpr auto max_align = max_align_of_v<Ts...>;
  return (0 + ... + get_size_in_tuple<Ts>(max_align));
}

template <std::size_t I, std::size_t Align, typename T, typename... Ts>
struct size_of_until
    : std::integral_constant<std::size_t,
                             get_size_in_tuple<T>(Align) +
                                 size_of_until<I - 1, Align, Ts...>::value> {};

template <std::size_t Align, typename T, typename... Ts>
struct size_of_until<0, Align, T, Ts...>
    : std::integral_constant<std::size_t, 0> {};

template <std::size_t I, std::size_t Align, typename... Ts>
constexpr auto size_of_until_v = size_of_until<I, Align, Ts...>::value;

template <typename... Ts>
struct max_size_of;

template <typename T, typename... Ts>
struct max_size_of<T, Ts...>
    : std::conditional_t<(sizeof(T) > max_size_of<Ts...>::value),
                         std::integral_constant<std::size_t, sizeof(T)>,
                         max_size_of<Ts...>> {};

template <>
struct max_size_of<> : std::integral_constant<std::size_t, 0> {};

template <typename... Ts>
constexpr auto max_size_of_v = max_size_of<Ts...>::value;

template <std::size_t I, typename T>
struct type_at_position;

template <std::size_t I, typename T, typename... Ts>
struct type_at_position<I, tuple2<T, Ts...>>
    : type_at_position<I - 1, tuple2<Ts...>> {};

template <typename T, typename... Ts>
struct type_at_position<0, tuple2<T, Ts...>> {
  using type = T;
};

template <size_t I, typename... Ts>
using type_at_position_t = typename type_at_position<I, Ts...>::type;

template <typename... Ts>
// TODO(julian) align this struct itself?
// struct alignas(max_align_of_v<Ts...>) tuple2 {
struct tuple2 {
  template <std::size_t... Is>
  using seq_t = const std::integer_sequence<std::size_t, Is...>;
  static constexpr auto Indices = std::make_index_sequence<sizeof...(Ts)>{};

  tuple2() { default_construct(Indices); }

  tuple2(tuple2 const& o) {
    default_construct(Indices);
    copy(o, Indices);
  }

  tuple2& operator=(tuple2 const& o) {
    if (&o != this) {
      default_construct(Indices);
      copy(o, Indices);
    }

    return *this;
  }

  tuple2(tuple2&& o) noexcept { move(o, Indices); }

  tuple2& operator=(tuple2&& o) noexcept {
    move(o, Indices);
    return *this;
  }

  tuple2(Ts&&... args) {  // NOLINT
    default_construct(Indices);
    set_array<std::tuple<Ts...>>(std::forward_as_tuple(args...), Indices);
  }

  ~tuple2() { destruct(Indices); }

  template <typename Tuple, std::size_t... Is>
  constexpr void set_array(Tuple&& t, seq_t<Is...>) {
    ((get<Is>(*this) = std::move(std::get<Is>(t))), ...);
  }

  template <std::size_t... Is>
  constexpr void default_construct(seq_t<Is...>) {
    ((new (&get<Is>(*this)) type_at_position_t<Is, tuple2<Ts...>>{}), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void copy(From const& from, seq_t<Is...>) {
    ((get<Is>(*this) = get<Is>(from)), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void move(From&& from, seq_t<Is...>) {
    ((get<Is>(*this) = std::move(get<Is>(from))), ...);
  }

  template <std::size_t... Is>
  constexpr void destruct(seq_t<Is...>) {
    ((get<Is>(*this).~decay_t<decltype(get<Is>(*this))>()), ...);
  }

  template <std::size_t I>
  constexpr auto get_ptr() {
    return &mem_[size_of_until_v<I, max_align_of_v<Ts...>, Ts...>];
  }

  template <std::size_t I>
  constexpr auto get_ptr() const {
    return &mem_[size_of_until_v<I, max_align_of_v<Ts...>, Ts...>];
  }

  std::aligned_storage<max_size_of_v<Ts...>, max_align_of_v<Ts...>>
      mem_[get_total_size<Ts...>()];
};

template <typename Head, typename... Tail>
tuple2(Head&& first, Tail&&... tail) -> tuple2<Head, Tail...>;

template <size_t I, typename... Ts>
auto& get(tuple2<Ts...>& t) {
  using return_t = type_at_position_t<I, tuple2<Ts...>>;
  return *std::launder(reinterpret_cast<return_t*>(t.template get_ptr<I>()));
}

template <size_t I, typename... Ts>
auto const& get(tuple2<Ts...> const& t) {
  using return_t = type_at_position_t<I, tuple2<Ts...>>;
  return *std::launder(
      reinterpret_cast<return_t const*>(t.template get_ptr<I>()));
}

template <size_t I, typename... Ts>
auto&& get(tuple2<Ts...>&& t) {
  using return_t = type_at_position_t<I, tuple2<Ts...>>;
  return *std::launder(reinterpret_cast<return_t*>(t.template get_ptr<I>()));
}

template <size_t I, typename... Ts>
auto const&& get(tuple2<Ts...> const&& t) {
  using return_t = type_at_position_t<I, tuple2<Ts...>>;
  return *std::launder(
      reinterpret_cast<return_t const*>(t.template get_ptr<I>()));
}

template <typename... Ts>
struct tuple_size<tuple2<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct tuple_element<I, tuple2<Ts...>> : type_at_position<I, tuple2<Ts...>> {};

template <typename... Ts>
struct is_tuple<tuple2<Ts...>> : std::true_type {};

}  // namespace cista

namespace std {
template <typename... Pack>
struct tuple_size<cista::tuple2<Pack...>>
    : cista::tuple_size<cista::tuple2<Pack...>> {};

template <std::size_t I, typename... Pack>
struct tuple_element<I, cista::tuple2<Pack...>>
    : cista::tuple_element<I, cista::tuple2<Pack...>> {};

}  // namespace std
