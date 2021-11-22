#include <bitset>
#include <fstream>
#include <iostream>

#include "cista/containers/bitset.h"

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
  constexpr auto const SIZE = 22;
  enum op { SHIFT_LEFT, SHIFT_RIGHT, XOR, OR, AND, NEGATE, NUM_OPS };

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

  auto const init = [&](auto& b1, auto& b2) {
    for (auto i = 0; i < SIZE; ++i) {
      auto const val = gen();
      b1.set(i, val);
      b2.set(i, val);
    }
  };

  std::bitset<SIZE> ref1, ref2;
  cista::bitset<SIZE> uut1, uut2;

  init(ref1, uut1);
  init(ref2, uut2);

  for (; data != end; ++data) {
    auto const op = data[0U] % NUM_OPS;
    auto const num = (data[0U] & 0xF0) % 100;

    switch (op) {
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

      default: abort();
    }

    if (uut1.to_string() != ref1.to_string()) {
      abort();
    }

    if (uut2.to_string() != uut2.to_string()) {
      abort();
    }
  }

  return 0;
}
#endif