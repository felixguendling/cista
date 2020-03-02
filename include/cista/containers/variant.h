#include <cinttypes>
#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

namespace cista {

template <std::size_t Size, typename Enable = void>
struct bytes_to_integer_type {};

template <std::size_t Size>
struct bytes_to_integer_type<Size, std::enable_if_t<Size == 1>> {
  using type = std::uint8_t;
};

template <std::size_t Size>
struct bytes_to_integer_type<Size, std::enable_if_t<Size == 2>> {
  using type = std::uint16_t;
};

template <typename... T>
constexpr std::size_t bytes() {
  if (sizeof...(T) > std::numeric_limits<uint8_t>::max()) {
    return 2;
  } else {
    return 1;
  }
}

template <typename... T>
using variant_index_t = typename bytes_to_integer_type<bytes<T...>()>::type;

constexpr auto const TYPE_NOT_FOUND = std::numeric_limits<std::size_t>::max();

template <typename Arg, typename... T>
constexpr std::size_t index_of_type() {
  constexpr std::array<bool, sizeof...(T)> matches = {
      {std::is_same<std::decay_t<Arg>, std::decay_t<T>>::value...}};
  for (std::size_t i = 0; i < sizeof...(T); ++i) {
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
using type_index_v = typename type_at_index<Index, T...>::type;

template <typename... T>
struct variant {
  using index_t = variant_index_t<T...>;

  template <typename Arg,
            typename Enable = std::enable_if_t<
                index_of_type<std::decay_t<Arg>, T...>() != TYPE_NOT_FOUND>>
  explicit variant(Arg&& arg)
      : idx_{static_cast<index_t>(index_of_type<Arg, T...>())} {
    new (&storage_) Arg(std::forward<Arg>(arg));
  }

  variant(variant const& o) {
    o.apply([this](auto&& el) { *this = el; });
  }
  variant(variant&& o) {
    o.apply([this](auto&& el) { *this = std::move(el); });
  }

  variant& operator=(variant const& o) {
    return o.apply([&](auto&& el) -> variant& { return *this = el; });
  }
  variant& operator=(variant&& o) {
    return o.apply(
        [&](auto&& el) -> variant& { return *this = std::move(el); });
  }

  template <typename Arg,
            typename Enable = std::enable_if_t<
                index_of_type<std::decay_t<Arg>, T...>() != TYPE_NOT_FOUND>>
  variant& operator=(Arg&& arg) {
    destruct();
    idx_ = index_of_type<Arg, T...>();
    new (&storage_) std::decay_t<Arg>{std::forward<Arg>(arg)};
    return *this;
  }

  ~variant() { destruct(); }

  void destruct() {
    apply([](auto&& el) {
      using el_type = std::decay_t<decltype(el)>;
      el.~el_type();
    });
  }

  template <typename F, std::size_t B = 0U>
  auto apply(F&& f) const {
    switch (idx_) {
      case B + 0:
        if constexpr (B + 0 < sizeof...(T)) {
          return f(as<type_index_v<B + 0, T...>>());
        }
        [[fallthrough]];
      case B + 1:
        if constexpr (B + 1 < sizeof...(T)) {
          return f(as<type_index_v<B + 1, T...>>());
        }
        [[fallthrough]];
      case B + 2:
        if constexpr (B + 2 < sizeof...(T)) {
          return f(as<type_index_v<B + 2, T...>>());
        }
        [[fallthrough]];
      case B + 3:
        if constexpr (B + 3 < sizeof...(T)) {
          return f(as<type_index_v<B + 3, T...>>());
        }
        [[fallthrough]];
      case B + 4:
        if constexpr (B + 4 < sizeof...(T)) {
          return f(as<type_index_v<B + 4, T...>>());
        }
        [[fallthrough]];
      case B + 5:
        if constexpr (B + 5 < sizeof...(T)) {
          return f(as<type_index_v<B + 5, T...>>());
        }
        [[fallthrough]];
      case B + 6:
        if constexpr (B + 6 < sizeof...(T)) {
          return f(as<type_index_v<B + 6, T...>>());
        }
        [[fallthrough]];
      case B + 7:
        if constexpr (B + 7 < sizeof...(T)) {
          return f(as<type_index_v<B + 7, T...>>());
        }
        [[fallthrough]];
      case B + 8:
        if constexpr (B + 8 < sizeof...(T)) {
          return f(as<type_index_v<B + 8, T...>>());
        }
        [[fallthrough]];
      case B + 9:
        if constexpr (B + 9 < sizeof...(T)) {
          return f(as<type_index_v<B + 9, T...>>());
        }
        [[fallthrough]];
      case B + 10:
        if constexpr (B + 10 < sizeof...(T)) {
          return f(as<type_index_v<B + 10, T...>>());
        }
        [[fallthrough]];
      case B + 11:
        if constexpr (B + 11 < sizeof...(T)) {
          return f(as<type_index_v<B + 11, T...>>());
        }
        [[fallthrough]];
      case B + 12:
        if constexpr (B + 12 < sizeof...(T)) {
          return f(as<type_index_v<B + 12, T...>>());
        }
        [[fallthrough]];
      case B + 13:
        if constexpr (B + 13 < sizeof...(T)) {
          return f(as<type_index_v<B + 13, T...>>());
        }
        [[fallthrough]];
      case B + 14:
        if constexpr (B + 14 < sizeof...(T)) {
          return f(as<type_index_v<B + 14, T...>>());
        }
        [[fallthrough]];
      case B + 15:
        if constexpr (B + 15 < sizeof...(T)) {
          return f(as<type_index_v<B + 15, T...>>());
        }
        [[fallthrough]];
      default:
        if constexpr (B + 15 < sizeof...(T)) {
          return apply<B + 16U, T...>(std::forward<F>(f));
        }
    }
#ifndef _MSC_VER
    __builtin_unreachable();
#endif
  }

  template <typename F, std::size_t B = 0U>
  auto apply(F f) {
    switch (idx_) {
      case B + 0:
        if constexpr (B + 0 < sizeof...(T)) {
          return f(as<type_index_v<B + 0, T...>>());
        }
        [[fallthrough]];
      case B + 1:
        if constexpr (B + 1 < sizeof...(T)) {
          return f(as<type_index_v<B + 1, T...>>());
        }
        [[fallthrough]];
      case B + 2:
        if constexpr (B + 2 < sizeof...(T)) {
          return f(as<type_index_v<B + 2, T...>>());
        }
        [[fallthrough]];
      case B + 3:
        if constexpr (B + 3 < sizeof...(T)) {
          return f(as<type_index_v<B + 3, T...>>());
        }
        [[fallthrough]];
      case B + 4:
        if constexpr (B + 4 < sizeof...(T)) {
          return f(as<type_index_v<B + 4, T...>>());
        }
        [[fallthrough]];
      case B + 5:
        if constexpr (B + 5 < sizeof...(T)) {
          return f(as<type_index_v<B + 5, T...>>());
        }
        [[fallthrough]];
      case B + 6:
        if constexpr (B + 6 < sizeof...(T)) {
          return f(as<type_index_v<B + 6, T...>>());
        }
        [[fallthrough]];
      case B + 7:
        if constexpr (B + 7 < sizeof...(T)) {
          return f(as<type_index_v<B + 7, T...>>());
        }
        [[fallthrough]];
      case B + 8:
        if constexpr (B + 8 < sizeof...(T)) {
          return f(as<type_index_v<B + 8, T...>>());
        }
        [[fallthrough]];
      case B + 9:
        if constexpr (B + 9 < sizeof...(T)) {
          return f(as<type_index_v<B + 9, T...>>());
        }
        [[fallthrough]];
      case B + 10:
        if constexpr (B + 10 < sizeof...(T)) {
          return f(as<type_index_v<B + 10, T...>>());
        }
        [[fallthrough]];
      case B + 11:
        if constexpr (B + 11 < sizeof...(T)) {
          return f(as<type_index_v<B + 11, T...>>());
        }
        [[fallthrough]];
      case B + 12:
        if constexpr (B + 12 < sizeof...(T)) {
          return f(as<type_index_v<B + 12, T...>>());
        }
        [[fallthrough]];
      case B + 13:
        if constexpr (B + 13 < sizeof...(T)) {
          return f(as<type_index_v<B + 13, T...>>());
        }
        [[fallthrough]];
      case B + 14:
        if constexpr (B + 14 < sizeof...(T)) {
          return f(as<type_index_v<B + 14, T...>>());
        }
        [[fallthrough]];
      case B + 15:
        if constexpr (B + 15 < sizeof...(T)) {
          return f(as<type_index_v<B + 15, T...>>());
        }
        [[fallthrough]];
      default:
        if constexpr (B + 15 < sizeof...(T)) {
          return apply<B + 16U, T...>(std::forward<F>(f));
        }
    }
#ifndef _MSC_VER
    __builtin_unreachable();
#endif
  }

  template <typename AsType>
  AsType& as() {
    return *reinterpret_cast<AsType*>(&storage_);
  }

  template <typename AsType>
  AsType const& as() const {
    return *reinterpret_cast<AsType const*>(&storage_);
  }

  index_t idx_;
  std::aligned_union_t<0, T...> storage_;
};

namespace raw {
using cista::variant;
}  // namespace raw

namespace offset {
using cista::variant;
}  // namespace offset

}  // namespace cista