#include <bitset>
#include <fstream>
#include <iostream>
#include <tuple>

#include "cista/containers/bitset.h"

enum op { SHIFT_LEFT, SHIFT_RIGHT, XOR, OR, AND, NEGATE, NUM_OPS };
char const* op_strings[] = {"SHIFT_LEFT", "SHIFT_RIGHT", "XOR",    "OR",
                            "AND",        "NEGATE",      "NUM_OPS"};

template <std::size_t Size>
struct test_set {
  template <typename Fn>
  void init(Fn&& gen) {
    for (auto i = 0; i < ref1.size(); ++i) {
      auto const val = gen();
      ref1.set(i, val);
      uut1.set(i, val);
    }
    for (auto i = 0; i < ref1.size(); ++i) {
      auto const val = gen();
      uut2.set(i, val);
      ref2.set(i, val);
    }
  }

  void apply_op(op const o, size_t const num) {
    auto const ref1_before = ref1;
    auto const ref2_before = ref2;
    auto const uut1_before = uut1;
    auto const uut2_before = uut2;

    switch (o) {
      case SHIFT_LEFT:
        ref1 <<= num;
        uut1 <<= num;
        break;

      case SHIFT_RIGHT:
        ref1 >>= num;
        uut1 >>= num;
        break;

      case XOR:
        ref1 = ref1 ^ ref2;
        uut1 = uut1 ^ uut2;
        break;

      case OR:
        ref1 |= ref2;
        uut1 |= uut2;
        break;

      case AND:
        ref1 &= ref2;
        uut1 &= uut2;
        break;

      case NEGATE:
        ref1 = ~ref1;
        ref2 = ~ref2;
        uut1 = ~uut1;
        uut2 = ~uut2;
        break;
    }

    if (uut1.to_string() != ref1.to_string()) {
      std::cerr << "fail on 1: OP=" << op_strings[o] << " NUM=" << num << "\n"
                << "  ref1_before: " << ref1_before << "\n"
                << "  uut1_before: " << uut1_before << "\n"
                << "  ref2_before: " << ref2_before << "\n"
                << "  uut2_before: " << uut2_before << "\n"
                << "         ref1: " << ref1 << "\n"
                << "         uut1: " << uut1 << "\n"
                << "         ref2: " << ref2 << "\n"
                << "         uut2: " << uut2 << "\n";
      abort();
    }

    if (uut2.to_string() != ref2.to_string()) {
      std::cerr << "fail on 2: OP=" << op_strings[o] << " NUM=" << num << "\n"
                << "  ref1_before: " << ref1_before << "\n"
                << "  uut1_before: " << uut1_before << "\n"
                << "  ref2_before: " << ref2_before << "\n"
                << "  uut2_before: " << uut2_before << "\n"
                << "         ref1: " << ref1 << "\n"
                << "         uut1: " << uut1 << "\n"
                << "         ref2: " << ref2 << "\n"
                << "         uut2: " << uut2 << "\n";
      abort();
    }
  }

  std::bitset<Size> ref1, ref2;
  cista::bitset<Size> uut1, uut2;
};

#if defined(GENERATE_SEED)
int main() {}
#else
#if defined(MAIN)
int main(int argc, char const** argv) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " INPUT\n";
    return 1;
  }

  auto in = std::ifstream{};
  in.exceptions(std::ios::failbit | std::ios::badbit);
  in.open(argv[1], std::ios_base::binary);
  auto str = std::string{};

  in.seekg(0, std::ios::end);
  str.reserve(in.tellg());
  in.seekg(0, std::ios::beg);

  str.assign((std::istreambuf_iterator<char>(in)),
             std::istreambuf_iterator<char>());
  auto data = reinterpret_cast<std::uint8_t const*>(str.data());
  auto const size = str.size();
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
#endif
  auto test_sets =
      std::make_tuple(test_set<22>{}, test_set<77>{}, test_set<222>{},
                      test_set<64>{}, test_set<128>{}, test_set<192>{});

  auto const end = data + size;

  auto gen = [&, bit = 0U]() mutable {
    if (data >= end) {
      return false;
    }
    auto const boolean = ((data[0] >> bit) & 1) != 0U;
    ++bit;
    if (bit == 8U) {
      ++data;
      bit = 0U;
    }
    return boolean;
  };

  std::apply([&](auto&... t) { (t.init(gen), ...); }, test_sets);

  for (; data != end; ++data) {
    auto const o = static_cast<op>(data[0U] % NUM_OPS);
    auto const num = (data[0U] & 0xF0) % 100;
    std::apply([&](auto&... t) { (t.apply_op(o, num), ...); }, test_sets);
  }

  return 0;
}
#endif