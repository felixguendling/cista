
#pragma once

#include <functional>
#include <tuple>

#include "cista/reflection/arity.h"

namespace cista {

namespace detail {

template <typename T, typename = void>
struct has_cista_members : std::false_type {};

template <typename T>
struct has_cista_members<
    T, std::void_t<decltype(std::declval<std::decay_t<T>>().cista_members())>>
    : std::true_type {};

template <typename T>
inline constexpr auto const has_cista_members_v = has_cista_members<T>::value;

template <typename... Ts, std::size_t... I>
constexpr auto add_const_helper(std::tuple<Ts...>&& t,
                                std::index_sequence<I...>) {
  return std::make_tuple(std::cref(std::get<I>(t))...);
}

template <typename T>
constexpr auto add_const(T&& t) {
  return add_const_helper(
      std::forward<T>(t),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(t)>>>());
}

template <typename... Ts, std::size_t... I>
auto to_ptrs_helper(std::tuple<Ts...>&& t, std::index_sequence<I...>) {
  return std::make_tuple(&std::get<I>(t)...);
}

template <typename T>
auto to_ptrs(T&& t) {
  return to_ptrs_helper(
      std::forward<T>(t),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<T>>>());
}

}  // namespace detail

template <typename T>
inline constexpr auto to_tuple_works_v =
    detail::has_cista_members_v<T> ||
    (std::is_aggregate_v<T> &&
#if !defined(_MSC_VER) || defined(NDEBUG)
     std::is_standard_layout_v<T> &&
#endif
     !std::is_polymorphic_v<T> && !std::is_union_v<T>);

template <typename T,
          std::enable_if_t<detail::has_cista_members_v<T> && std::is_const_v<T>,
                           void*> = nullptr>
constexpr auto to_tuple(T& t) {
  return detail::add_const(
      const_cast<std::add_lvalue_reference_t<std::remove_const_t<T>>>(t)
          .cista_members());
}

template <typename T, std::enable_if_t<detail::has_cista_members_v<T> &&
                                           !std::is_const_v<T>,
                                       void*> = nullptr>
constexpr auto to_tuple(T&& t) {
  return t.cista_members();
}

template <typename T,
          std::enable_if_t<!detail::has_cista_members_v<T>, void*> = nullptr>
auto to_tuple(T& t) {
  constexpr auto const a = arity<T>();
  static_assert(a <= 64U, "Max. supported members: 64");
  if constexpr (a == 0U) {
    return std::tie();
  } else if constexpr (a == 1U) {
    auto& [p1] = t;
    return std::tie(p1);
  } else if constexpr (a == 2U) {
    auto& [p1, p2] = t;
    return std::tie(p1, p2);
  } else if constexpr (a == 3U) {
    auto& [p1, p2, p3] = t;
    return std::tie(p1, p2, p3);
  } else if constexpr (a == 4U) {
    auto& [p1, p2, p3, p4] = t;
    return std::tie(p1, p2, p3, p4);
  } else if constexpr (a == 5U) {
    auto& [p1, p2, p3, p4, p5] = t;
    return std::tie(p1, p2, p3, p4, p5);
  } else if constexpr (a == 6U) {
    auto& [p1, p2, p3, p4, p5, p6] = t;
    return std::tie(p1, p2, p3, p4, p5, p6);
  } else if constexpr (a == 7U) {
    auto& [p1, p2, p3, p4, p5, p6, p7] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7);
  } else if constexpr (a == 8U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8);
  } else if constexpr (a == 9U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9);
  } else if constexpr (a == 10U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
  } else if constexpr (a == 11U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
  } else if constexpr (a == 12U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
  } else if constexpr (a == 13U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
  } else if constexpr (a == 14U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13,
                    p14);
  } else if constexpr (a == 15U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15] =
        t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15);
  } else if constexpr (a == 16U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16);
  } else if constexpr (a == 17U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17);
  } else if constexpr (a == 18U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18);
  } else if constexpr (a == 19U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19);
  } else if constexpr (a == 20U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20);
  } else if constexpr (a == 21U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21);
  } else if constexpr (a == 22U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22);
  } else if constexpr (a == 23U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23);
  } else if constexpr (a == 24U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24);
  } else if constexpr (a == 25U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25);
  } else if constexpr (a == 26U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26);
  } else if constexpr (a == 27U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27);
  } else if constexpr (a == 28U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28);
  } else if constexpr (a == 29U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28,
           p29] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29);
  } else if constexpr (a == 30U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30);
  } else if constexpr (a == 31U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31);
  } else if constexpr (a == 32U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32);
  } else if constexpr (a == 33U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33);
  } else if constexpr (a == 34U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34);
  } else if constexpr (a == 35U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35);
  } else if constexpr (a == 36U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36);
  } else if constexpr (a == 37U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37);
  } else if constexpr (a == 38U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38);
  } else if constexpr (a == 39U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39);
  } else if constexpr (a == 40U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40);
  } else if constexpr (a == 41U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41);
  } else if constexpr (a == 42U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42);
  } else if constexpr (a == 43U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42,
           p43] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43);
  } else if constexpr (a == 44U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44);
  } else if constexpr (a == 45U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45);
  } else if constexpr (a == 46U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46);
  } else if constexpr (a == 47U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47);
  } else if constexpr (a == 48U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48);
  } else if constexpr (a == 49U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49);
  } else if constexpr (a == 50U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50);
  } else if constexpr (a == 51U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51);
  } else if constexpr (a == 52U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52);
  } else if constexpr (a == 53U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53);
  } else if constexpr (a == 54U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54);
  } else if constexpr (a == 55U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55);
  } else if constexpr (a == 56U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56);
  } else if constexpr (a == 57U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56,
           p57] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57);
  } else if constexpr (a == 58U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58);
  } else if constexpr (a == 59U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58, p59] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58, p59);
  } else if constexpr (a == 60U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58, p59, p60] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58, p59, p60);
  } else if constexpr (a == 61U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58, p59, p60, p61] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61);
  } else if constexpr (a == 62U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58, p59, p60, p61, p62] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61, p62);
  } else if constexpr (a == 63U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58, p59, p60, p61, p62, p63] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61, p62,
                    p63);
  } else if constexpr (a == 64U) {
    auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
           p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29,
           p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43,
           p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
           p58, p59, p60, p61, p62, p63, p64] = t;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                    p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26,
                    p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                    p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50,
                    p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61, p62,
                    p63, p64);
  }
}

template <typename T>
auto to_ptr_tuple(T&& t) {
  return detail::to_ptrs(to_tuple(std::forward<T>(t)));
}
}  // namespace cista
