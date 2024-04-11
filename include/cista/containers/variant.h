#pragma once

#include <cinttypes>
#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

#include "cista/exception.h"
#include "cista/hashing.h"

namespace cista {

template <std::size_t Size, typename Enable = void>
struct bytes_to_integer_type {};

template <std::size_t Size>
struct bytes_to_integer_type<Size, std::enable_if_t<Size == 1U>> {
  using type = std::uint8_t;
};

template <std::size_t Size>
struct bytes_to_integer_type<Size, std::enable_if_t<Size == 2U>> {
  using type = std::uint16_t;
};

template <typename... T>
constexpr std::size_t bytes() noexcept {
  return (sizeof...(T) > std::numeric_limits<std::uint8_t>::max()) ? 2U : 1U;
}

template <typename... T>
using variant_index_t = typename bytes_to_integer_type<bytes<T...>()>::type;

constexpr auto const TYPE_NOT_FOUND = std::numeric_limits<std::size_t>::max();

template <typename Arg, typename... T>
constexpr std::size_t index_of_type() noexcept {
  constexpr std::array<bool, sizeof...(T)> matches = {
      {std::is_same<std::decay_t<Arg>, std::decay_t<T>>::value...}};
  for (std::size_t i = 0U; i < sizeof...(T); ++i) {
    if (matches[i]) {
      return i;
    }
  }
  return TYPE_NOT_FOUND;
}

template <int N, typename... T>
struct type_at_index;

template <typename First, typename... Rest>
struct type_at_index<0, First, Rest...> {
  using type = First;
};

template <int N, typename First, typename... Rest>
struct type_at_index<N, First, Rest...> {
  using type = typename type_at_index<N - 1, Rest...>::type;
};

template <std::size_t Index, typename... T>
using type_at_index_t = typename type_at_index<Index, T...>::type;

template <typename... T>
struct variant {
  using index_t = variant_index_t<T...>;
  static constexpr auto NO_VALUE = std::numeric_limits<index_t>::max();

  constexpr variant() noexcept : idx_{NO_VALUE} {}

  template <typename Arg,
            typename = std::enable_if_t<
                index_of_type<std::decay_t<Arg>, T...>() != TYPE_NOT_FOUND>>
  constexpr variant(Arg&& arg)
      : idx_{static_cast<index_t>(index_of_type<Arg, T...>())} {
#if defined(CISTA_ZERO_OUT)
    std::memset(&storage_, 0, sizeof(storage_));
#endif
    using Type = std::decay_t<Arg>;
    new (&storage_) Type(std::forward<Arg>(arg));
  }

  variant(variant const& o) : idx_{o.idx_} {
    o.apply([this](auto&& el) {
      using Type = std::decay_t<decltype(el)>;
      new (&storage_) Type(std::forward<decltype(el)>(el));
    });
  }
  variant(variant&& o) : idx_{o.idx_} {
    o.apply([this](auto&& el) {
      using Type = std::decay_t<decltype(el)>;
      new (&storage_) Type(std::move(el));
    });
  }

  variant& operator=(variant const& o) {
    return o.apply([&](auto&& el) -> variant& { return *this = el; });
  }
  variant& operator=(variant&& o) {
    return o.apply(
        [&](auto&& el) -> variant& { return *this = std::move(el); });
  }

  template <typename Arg,
            typename = std::enable_if_t<
                index_of_type<std::decay_t<Arg>, T...>() != TYPE_NOT_FOUND>>
  variant& operator=(Arg&& arg) {
    if (index_of_type<Arg, T...>() == idx_) {
      apply([&](auto&& el) {
        if constexpr (std::is_same_v<std::decay_t<decltype(el)>, Arg>) {
          el = std::move(arg);
        }
      });
      return *this;
    }
    destruct();
    idx_ = static_cast<index_t>(index_of_type<Arg, T...>());
    new (&storage_) std::decay_t<Arg>{std::forward<Arg>(arg)};
    return *this;
  }
#if _MSVC_LANG >= 202002L || __cplusplus >= 202002L
  constexpr
#endif
      ~variant() {
    destruct();
  }

  constexpr bool valid() const noexcept { return index() != NO_VALUE; }

  constexpr operator bool() const noexcept { return valid(); }

  friend bool operator==(variant const& a, variant const& b) noexcept {
    return a.idx_ == b.idx_
               ? apply([](auto&& u, auto&& v) -> bool { return u == v; },
                       a.idx_, a, b)
               : false;
  }

  friend bool operator!=(variant const& a, variant const& b) noexcept {
    return a.idx_ == b.idx_
               ? apply([](auto&& u, auto&& v) -> bool { return u != v; },
                       a.idx_, a, b)
               : true;
  }

  friend bool operator<(variant const& a, variant const& b) noexcept {
    return a.idx_ == b.idx_
               ? apply([](auto&& u, auto&& v) -> bool { return u < v; }, a.idx_,
                       a, b)
               : a.idx_ < b.idx_;
  }

  friend bool operator>(variant const& a, variant const& b) noexcept {
    return a.idx_ == b.idx_
               ? apply([](auto&& u, auto&& v) -> bool { return u > v; }, a.idx_,
                       a, b)
               : a.idx_ > b.idx_;
  }

  friend bool operator<=(variant const& a, variant const& b) noexcept {
    return a.idx_ == b.idx_
               ? apply([](auto&& u, auto&& v) -> bool { return u <= v; },
                       a.idx_, a, b)
               : a.idx_ <= b.idx_;
  }

  friend bool operator>=(variant const& a, variant const& b) noexcept {
    return a.idx_ == b.idx_
               ? apply([](auto&& u, auto&& v) -> bool { return u >= v; },
                       a.idx_, a, b)
               : a.idx_ >= b.idx_;
  }

  template <typename Arg, typename... CtorArgs>
  Arg& emplace(CtorArgs&&... ctor_args) {
    static_assert(index_of_type<Arg, T...>() != TYPE_NOT_FOUND);
    destruct();
    idx_ = static_cast<index_t>(index_of_type<Arg, T...>());
    return *(new (&storage_)
                 std::decay_t<Arg>{std::forward<CtorArgs>(ctor_args)...});
  }

  template <std::size_t I, typename... CtorArgs>
  type_at_index_t<I, T...>& emplace(CtorArgs&&... ctor_args) {
    static_assert(I < sizeof...(T));
    destruct();
    idx_ = I;
    return *(new (&storage_) std::decay_t<type_at_index_t<I, T...>>{
        std::forward<CtorArgs>(ctor_args)...});
  }

  constexpr std::size_t index() const noexcept { return idx_; }

  void swap(variant& o) {
    if (idx_ == o.idx_) {
      apply(
          [](auto&& a, auto&& b) {
            using std::swap;
            swap(a, b);
          },
          idx_, *this, o);
    } else {
      variant tmp{std::move(o)};
      o = std::move(*this);
      *this = std::move(tmp);
    }
  }

  constexpr void destruct() {
    if (idx_ != NO_VALUE) {
      apply([](auto&& el) {
        using el_type = std::decay_t<decltype(el)>;
        el.~el_type();
      });
    }
  }

  template <typename F>
  auto apply(F&& f) -> decltype(f(std::declval<type_at_index_t<0U, T...>>())) {
    if (idx_ == NO_VALUE) {
      throw_exception(std::runtime_error{"variant::apply: no value"});
    }
    return apply(std::forward<F>(f), idx_, *this);
  }

  template <typename F>
  auto apply(F&& f) const
      -> decltype(f(std::declval<type_at_index_t<0U, T...>>())) {
    if (idx_ == NO_VALUE) {
      throw_exception(std::runtime_error{"variant::apply: no value"});
    }
    return apply(std::forward<F>(f), idx_, *this);
  }

#ifdef _MSC_VER
  static __declspec(noreturn) void noret() {}
#endif

  template <typename F, std::size_t B = 0U, typename... Vs>
  static auto apply(F&& f, index_t const idx, Vs&&... vs)
      -> decltype(f((vs, std::declval<type_at_index_t<0U, T...>>())...)) {
    switch (idx) {
      case B + 0U:
        if constexpr (B + 0U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 0U, T...>>()...);
        }
        [[fallthrough]];
      case B + 1U:
        if constexpr (B + 1U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 1U, T...>>()...);
        }
        [[fallthrough]];
      case B + 2U:
        if constexpr (B + 2U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 2U, T...>>()...);
        }
        [[fallthrough]];
      case B + 3U:
        if constexpr (B + 3U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 3U, T...>>()...);
        }
        [[fallthrough]];
      case B + 4U:
        if constexpr (B + 4U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 4U, T...>>()...);
        }
        [[fallthrough]];
      case B + 5U:
        if constexpr (B + 5U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 5U, T...>>()...);
        }
        [[fallthrough]];
      case B + 6U:
        if constexpr (B + 6U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 6U, T...>>()...);
        }
        [[fallthrough]];
      case B + 7U:
        if constexpr (B + 7U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 7U, T...>>()...);
        }
        [[fallthrough]];
      case B + 8U:
        if constexpr (B + 8U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 8U, T...>>()...);
        }
        [[fallthrough]];
      case B + 9U:
        if constexpr (B + 9U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 9U, T...>>()...);
        }
        [[fallthrough]];
      case B + 10U:
        if constexpr (B + 10U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 10U, T...>>()...);
        }
        [[fallthrough]];
      case B + 11U:
        if constexpr (B + 11U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 11U, T...>>()...);
        }
        [[fallthrough]];
      case B + 12U:
        if constexpr (B + 12U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 12U, T...>>()...);
        }
        [[fallthrough]];
      case B + 13U:
        if constexpr (B + 13U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 13U, T...>>()...);
        }
        [[fallthrough]];
      case B + 14U:
        if constexpr (B + 14U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 14U, T...>>()...);
        }
        [[fallthrough]];
      case B + 15U:
        if constexpr (B + 15U < sizeof...(T)) {
          return f(vs.template as<type_at_index_t<B + 15U, T...>>()...);
        }
        break;
    }

    if constexpr (B + 15U < sizeof...(T)) {
      return apply<F, B + 16U>(std::forward<F>(f), std::forward<Vs>(vs)...);
    }

#ifndef _MSC_VER
    __builtin_unreachable();
#else
    noret();
#endif
  }

  template <typename AsType>
  AsType& as() noexcept {
    return *reinterpret_cast<AsType*>(&storage_);
  }

  template <typename AsType>
  AsType const& as() const noexcept {
    return *reinterpret_cast<AsType const*>(&storage_);
  }

  hash_t hash() const noexcept {
    return apply([&](auto&& val) {
      auto const idx = index();
      auto h = hashing<decltype(idx)>{}(idx, BASE_HASH);
      h = hashing<decltype(val)>{}(val, h);
      return h;
    });
  }

  index_t idx_{NO_VALUE};
  std::aligned_union_t<0, T...> storage_{};
};

template <typename T, typename... Ts>
bool holds_alternative(variant<Ts...> const& v) noexcept {
  return v.idx_ == index_of_type<std::decay_t<T>, Ts...>();
}

template <std::size_t I, typename... Ts>
constexpr cista::type_at_index_t<I, Ts...> const& get(
    cista::variant<Ts...> const& v) noexcept {
  return v.template as<cista::type_at_index_t<I, Ts...>>();
}

template <std::size_t I, typename... Ts>
constexpr cista::type_at_index_t<I, Ts...>& get(
    cista::variant<Ts...>& v) noexcept {
  return v.template as<cista::type_at_index_t<I, Ts...>>();
}

template <class T, class... Ts>
constexpr T const& get(cista::variant<Ts...> const& v) noexcept {
  static_assert(cista::index_of_type<T, Ts...>() != cista::TYPE_NOT_FOUND);
  return v.template as<T>();
}
template <class T, class... Ts>
constexpr T& get(cista::variant<Ts...>& v) noexcept {
  static_assert(cista::index_of_type<T, Ts...>() != cista::TYPE_NOT_FOUND);
  return v.template as<T>();
}

template <class T, class... Ts>
constexpr std::add_pointer_t<T> get_if(cista::variant<Ts...>& v) noexcept {
  static_assert(cista::index_of_type<T, Ts...>() != cista::TYPE_NOT_FOUND);
  return v.idx_ == cista::index_of_type<T, Ts...>() ? &v.template as<T>()
                                                    : nullptr;
}

template <std::size_t I, typename... Ts>
constexpr std::add_pointer_t<cista::type_at_index_t<I, Ts...> const> get_if(
    cista::variant<Ts...> const& v) noexcept {
  static_assert(I < sizeof...(Ts));
  return v.idx_ == I ? &v.template as<cista::type_at_index_t<I, Ts...>>()
                     : nullptr;
}

template <std::size_t I, typename... Ts>
constexpr std::add_pointer_t<cista::type_at_index_t<I, Ts...>> get_if(
    cista::variant<Ts...>& v) noexcept {
  static_assert(I < sizeof...(Ts));
  return v.idx_ == I ? &v.template as<cista::type_at_index_t<I, Ts...>>()
                     : nullptr;
}

template <class T, class... Ts>
constexpr std::add_pointer_t<T const> get_if(
    cista::variant<Ts...> const& v) noexcept {
  static_assert(cista::index_of_type<T, Ts...>() != cista::TYPE_NOT_FOUND);
  return v.idx_ == cista::index_of_type<T, Ts...>() ? &v.template as<T>()
                                                    : nullptr;
}

template <class T>
struct variant_size;

template <class... T>
struct variant_size<variant<T...>>
    : std::integral_constant<std::size_t, sizeof...(T)> {};

template <class T>
constexpr std::size_t variant_size_v = variant_size<T>::value;

namespace raw {
using cista::variant;
}  // namespace raw

namespace offset {
using cista::variant;
}  // namespace offset

}  // namespace cista

namespace std {

template <typename Visitor, typename... T>
constexpr auto visit(Visitor&& vis, cista::variant<T...>&& v) {
  v.apply(vis);
}

using cista::get;

}  // namespace std
