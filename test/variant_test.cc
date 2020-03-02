#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/variant.h"
#endif

namespace data = cista::offset;

template <typename... T>
struct op {
  using index_t = cista::variant_index_t<T...>;
  static constexpr std::size_t storage_size = std::max({sizeof(T)...});
  static constexpr std::size_t storage_alignment = std::max({alignof(T)...});
  template <std::size_t B = 0U, typename Fn>
  auto operator()(Fn&& f) const
      -> std::invoke_result_t<Fn, cista::type_index_v<2U, T...>> {
    switch (index_) {
      case B + 0:
        if constexpr (B + 0 < sizeof...(T)) return f(3);
    }
  }
  int index_;
};

TEST_CASE("complex union") {
  auto const print = [](int i) { printf("%d\n", i); };
  op<char, short, int>()(print);
  data::variant<short, char> v{short{1}}, o{char{'b'}};
  v.apply([](auto&& e) { printf("%d\n", e); });
  //  v = char{'a'};
  // v = o;
  //  std::cout << sizeof(v) << " " << v << "\n";
}