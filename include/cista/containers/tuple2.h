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

template <std::size_t I, typename... Ts>
struct type_at_position;

template <std::size_t I, typename T, typename... Ts>
struct type_at_position<I, tuple2<T, Ts...>>
    : type_at_position<I - 1, tuple2<Ts...>> {};

template <typename T, typename... Ts>
struct type_at_position<0, tuple2<T, Ts...>> {
  using type = T;
};

template <std::size_t I, typename T, typename... Ts>
struct type_at_position<I, T, Ts...> : type_at_position<I - 1, Ts...> {};

template <typename T, typename... Ts>
struct type_at_position<0, T, Ts...> {
  using type = T;
};

template <size_t I, typename... Ts>
using type_at_position_t = typename type_at_position<I, Ts...>::type;

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
constexpr std::size_t get_size() {
  return get_offset<Ts...>(sizeof...(Ts) + 1);
}

template <std::size_t I, typename T, typename... Ts>
constexpr std::enable_if_t<I == 0, T>&& get_arg(T&& arg, Ts&&...) {
  return std::forward<T>(arg);
}

template <std::size_t I, typename T, typename... Ts>
constexpr std::enable_if_t<I != 0, type_at_position_t<I, T, Ts...>>&& get_arg(
    T&&, Ts&&... args) {
  return get_arg<I - 1>(std::forward<Ts>(args)...);
}

template <typename... Ts>
struct alignas(max_align_of_v<Ts...>) tuple2 {
  template <std::size_t... Is>
  using seq_t = const std::integer_sequence<std::size_t, Is...>;
  static constexpr auto Indices = std::make_index_sequence<sizeof...(Ts)>{};

  tuple2() { default_construct(Indices); }

  tuple2(tuple2 const& o) { copy_construct(o, Indices); }

  tuple2& operator=(tuple2 const& o) {
    if (&o != this) {
      copy_assign(o, Indices);
    }

    return *this;
  }

  tuple2(tuple2&& o) noexcept { move_construct(o, Indices); }

  tuple2& operator=(tuple2&& o) noexcept {
    move_assign(o, Indices);
    return *this;
  }

  tuple2(Ts&&... args) {  // NOLINT
    move_construct_from_args(Indices, std::forward<Ts>(args)...);
  }

  ~tuple2() { destruct(Indices); }

  template <std::size_t... Is>
  constexpr void move_construct_from_args(seq_t<Is...>, Ts&&... args) {
    (new (&get<Is>(*this)) type_at_position_t<Is, Ts...>(
         std::move(get_arg<Is>(std::forward<Ts>(args)...))),
     ...);
  }

  template <typename Tuple, std::size_t... Is>
  constexpr void set_array(Tuple&& t, seq_t<Is...>) {
    ((get<Is>(*this) = std::move(std::get<Is>(t))), ...);
  }

  template <std::size_t... Is>
  constexpr void default_construct(seq_t<Is...>) {
    ((new (&get<Is>(*this)) type_at_position_t<Is, tuple2<Ts...>>{}), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void copy_construct(From const& from, seq_t<Is...>) {
    (new (&get<Is>(*this)) type_at_position_t<Is, tuple2<Ts...>>(get<Is>(from)),
     ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void copy_assign(From const& from, seq_t<Is...>) {
    ((get<Is>(*this) = get<Is>(from)), ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void move_construct(From&& from, seq_t<Is...>) {
    (new (&get<Is>(*this))
         type_at_position_t<Is, tuple2<Ts...>>(std::move(get<Is>(from))),
     ...);
  }

  template <typename From, std::size_t... Is>
  constexpr void move_assign(From&& from, seq_t<Is...>) {
    ((get<Is>(*this) = std::move(get<Is>(from))), ...);
  }

  template <std::size_t... Is>
  constexpr void destruct(seq_t<Is...>) {
    ((get<Is>(*this).~decay_t<decltype(get<Is>(*this))>()), ...);
  }

  template <std::size_t I>
  constexpr auto get_ptr() {
    return &mem_[get_offset<Ts...>(I)];
  }

  template <std::size_t I>
  constexpr auto get_ptr() const {
    return &mem_[get_offset<Ts...>(I)];
  }

  std::aligned_storage<max_size_of_v<Ts...>, max_align_of_v<Ts...>>
      mem_[get_size<Ts...>()];
};

template <typename Head, typename... Tail>
tuple2(Head&& first, Tail&&... tail) -> tuple2<Head, Tail...>;

template <size_t I, typename... Ts>
auto& get(tuple2<Ts...>& t) {
  using return_t = type_at_position_t<I, Ts...>;
  return *std::launder(reinterpret_cast<return_t*>(t.template get_ptr<I>()));
}

template <size_t I, typename... Ts>
auto const& get(tuple2<Ts...> const& t) {
  using return_t = type_at_position_t<I, Ts...>;
  return *std::launder(
      reinterpret_cast<return_t const*>(t.template get_ptr<I>()));
}

template <size_t I, typename... Ts>
auto&& get(tuple2<Ts...>&& t) {
  using return_t = type_at_position_t<I, Ts...>;
  return *std::launder(reinterpret_cast<return_t*>(t.template get_ptr<I>()));
}

template <size_t I, typename... Ts>
auto const&& get(tuple2<Ts...> const&& t) {
  using return_t = type_at_position_t<I, Ts...>;
  return *std::launder(
      reinterpret_cast<return_t const*>(t.template get_ptr<I>()));
}

template <typename... Ts>
struct tuple_size<tuple2<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct tuple_element<I, tuple2<Ts...>> : type_at_position<I, Ts...> {};

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
