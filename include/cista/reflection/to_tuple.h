#pragma once

#include <tuple>

#include "cista/reflection/arity.h"

namespace cista {

template <typename T>
inline auto to_tuple(T&, field_count<0>) {
  return std::tie();
}

template <typename T>
inline auto to_tuple(T& t, field_count<1>) {
  auto& [p1] = t;
  return std::tie(p1);
}

template <typename T>
inline auto to_tuple(T& t, field_count<2>) {
  auto& [p1, p2] = t;
  return std::tie(p1, p2);
}

template <typename T>
inline auto to_tuple(T& t, field_count<3>) {
  auto& [p1, p2, p3] = t;
  return std::tie(p1, p2, p3);
}

template <typename T>
inline auto to_tuple(T& t, field_count<4>) {
  auto& [p1, p2, p3, p4] = t;
  return std::tie(p1, p2, p3, p4);
}

template <typename T>
inline auto to_tuple(T& t, field_count<5>) {
  auto& [p1, p2, p3, p4, p5] = t;
  return std::tie(p1, p2, p3, p4, p5);
}

template <typename T>
inline auto to_tuple(T& t, field_count<6>) {
  auto& [p1, p2, p3, p4, p5, p6] = t;
  return std::tie(p1, p2, p3, p4, p5, p6);
}

template <typename T>
inline auto to_tuple(T& t, field_count<7>) {
  auto& [p1, p2, p3, p4, p5, p6, p7] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7);
}

template <typename T>
inline auto to_tuple(T& t, field_count<8>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8);
}

template <typename T>
inline auto to_tuple(T& t, field_count<9>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

template <typename T>
inline auto to_tuple(T& t, field_count<10>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

template <typename T>
inline auto to_tuple(T& t, field_count<11>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
}

template <typename T>
inline auto to_tuple(T& t, field_count<12>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
}

template <typename T>
inline auto to_tuple(T& t, field_count<13>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
}

template <typename T>
inline auto to_tuple(T& t, field_count<14>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
}

template <typename T>
inline auto to_tuple(T& t, field_count<15>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15);
}

template <typename T>
inline auto to_tuple(T& t, field_count<16>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
         p16] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16);
}

template <typename T>
inline auto to_tuple(T& t, field_count<17>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17);
}

template <typename T>
inline auto to_tuple(T& t, field_count<18>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17,
         p18] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17, p18);
}

template <typename T>
inline auto to_tuple(T& t, field_count<19>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18,
         p19] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17, p18, p19);
}

template <typename T>
inline auto to_tuple(T& t, field_count<20>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18, p19,
         p20] = t;
  return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
                  p15, p16, p17, p18, p19, p20);
}

template <typename T>
inline auto to_tuple(T& t) {
  return to_tuple(t, arity(t));
}

// Pointer to member

template <typename T>
inline auto to_ptr_tuple(T&, field_count<0>) {
  return std::make_tuple();
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<1>) {
  auto& [p1] = t;
  return std::make_tuple(&p1);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<2>) {
  auto& [p1, p2] = t;
  return std::make_tuple(&p1, &p2);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<3>) {
  auto& [p1, p2, p3] = t;
  return std::make_tuple(&p1, &p2, &p3);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<4>) {
  auto& [p1, p2, p3, p4] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<5>) {
  auto& [p1, p2, p3, p4, p5] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<6>) {
  auto& [p1, p2, p3, p4, p5, p6] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<7>) {
  auto& [p1, p2, p3, p4, p5, p6, p7] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<8>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<9>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<10>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<11>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<12>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<13>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<14>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<15>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<16>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,
         p16] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<17>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<18>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17,
         p18] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<19>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18,
         p19] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18, &p19);
}

template <typename T>
inline auto to_ptr_tuple(T& t, field_count<20>) {
  auto& [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16,
         p17, p18, p19,
         p20] = t;
  return std::make_tuple(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10,
                         &p11, &p12, &p13, &p14, &p15, &p16, &p17, &p18, &p19,
                         &p20);
}

template <typename T>
inline auto to_ptr_tuple(T& t) {
  return to_ptr_tuple(t, arity(t));
}

}  // namespace cista