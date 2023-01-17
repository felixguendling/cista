#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>

#include "cista/containers/bitvec.h"

enum op { RESIZE, SET, NUM_OPS };
char const* op_strings[] = {"RESIZE", "SET", "NUM_OPS"};

std::ostream& operator<<(std::ostream& out, std::vector<bool> const& v) {
  for (auto i = 0U; i != v.size(); ++i) {
    auto const j = v.size() - i - 1U;
    out << (v[j] ? '1' : '0');
  }
  return out;
}

std::string to_string(std::vector<bool> const& v) {
  auto ss = std::stringstream{};
  ss << v;
  return ss.str();
}

bool any(std::vector<bool> const& v) {
  return std::any_of(begin(v), end(v), [](bool const b) { return b; });
}

template <typename T>
bool bitvec_lt(T const& x, T const& y) {
  assert(x.size() == y.size());
  for (std::size_t i = 0U; i != x.size(); ++i) {
    auto const j = static_cast<std::size_t>(x.size() - i - 1U);
    if (x[i] ^ y[i]) {
      return y[i];
    }
  }
  if (x[0] ^ y[0]) {
    return y[0];
  }
  return false;
}

struct test_set {
  template <typename Fn>
  void init(Fn&& gen) {
    for (auto i = 0; i < ref1.size(); ++i) {
      auto const val = gen();
      ref1[i] = val;
      uut1.set(i, val);
    }
    for (auto i = 0; i < ref1.size(); ++i) {
      auto const val = gen();
      uut2.set(i, val);
      ref2[i] = val;
    }

    auto const print = [&](std::string_view msg) {
      std::cerr << msg << ": INIT\n"
                << "         ref1: " << ref1 << "\n"
                << "         uut1: " << uut1 << "\n"
                << "         ref2: " << ref2 << "\n"
                << "         uut2: " << uut2 << "\n";
    };

    if (uut1.str() != to_string(ref1)) {
      print("fail on init");
      abort();
    }

    if (uut2.str() != to_string(ref2)) {
      print("fail on 2");
      abort();
    }
  }

  void apply_op(op const o, std::size_t const num, bool const value) {
    auto const ref1_before = ref1;
    auto const ref2_before = ref2;
    auto const uut1_before = uut1;
    auto const uut2_before = uut2;

    switch (o) {
      case RESIZE:
        ref1.resize(num);
        ref2.resize(num);
        uut1.resize(num);
        uut2.resize(num);
        break;

      case SET: {
        if (!ref1.empty()) {
          auto const index =
              std::min({ref1.size() - 1U, ref2.size() - 1U, num});
          ref1[index] = value;
          ref2[index] = value;
          uut1.set(index, value);
          uut2.set(index, value);
        }
        break;
      }

      default: abort();
    }

    auto const print = [&](std::string_view msg) {
      std::cerr << msg << ": OP=" << op_strings[o] << " NUM=" << num << "\n"
                << "  ref1_before: " << ref1_before << "\n"
                << "  uut1_before: " << uut1_before << "\n"
                << "  ref2_before: " << ref2_before << "\n"
                << "  uut2_before: " << uut2_before << "\n"
                << "         ref1: " << ref1 << "\n"
                << "         uut1: " << uut1 << "\n"
                << "         ref2: " << ref2 << "\n"
                << "         uut2: " << uut2 << "\n";
    };

    if (bitvec_lt(ref1, ref2) != bitvec_lt(uut1, uut2)) {
      std::cerr << "uut1 < uut2 => " << (uut1 < uut2) << "\n"
                << "ref1 < ref2 => " << (ref1 < ref2) << "\n";
      print("fail on <");
      abort();
    }

    if ((ref1 == ref2) != (uut1 == uut2)) {
      std::cerr << "uut1 == uut2 => " << (uut1 == uut2) << "\n"
                << "ref1 == ref2 => " << (ref1 == ref2) << "\n";
      print("fail on ==");
      abort();
    }

    if ((ref1 != ref2) != (uut1 != uut2)) {
      std::cerr << "uut1 != uut2 => " << (uut1 != uut2) << "\n"
                << "ref1 != ref2 => " << (ref1 != ref2) << "\n";
      print("fail on !=");
      abort();
    }

    if (uut1.any() != any(ref1)) {
      std::cerr << "uut1.any() => " << uut1.any() << "\n"
                << "ref1.any() => " << any(ref1) << "\n";
      print("fail on any 1");
      abort();
    }

    if (uut2.any() != any(ref2)) {
      std::cerr << "uut2.any() => " << uut2.any() << "\n"
                << "ref2.any() => " << any(ref2) << "\n";
      print("fail on any 2");
      abort();
    }

    if (uut1.none() != !any(ref1)) {
      std::cerr << "uut1.none() => " << uut1.none() << "\n"
                << "ref1.none() => " << !any(ref1) << "\n";
      print("fail on none 1");
      abort();
    }

    if (uut2.none() != !any(ref2)) {
      std::cerr << "uut2.none() => " << uut2.none() << "\n"
                << "ref2.none() => " << !any(ref2) << "\n";
      print("fail on none 2");
      abort();
    }

    if (uut1.str() != to_string(ref1)) {
      print("fail on 1");
      abort();
    }

    if (uut2.str() != to_string(ref2)) {
      print("fail on 2");
      abort();
    }
  }

  void resize(std::size_t const size) {
    ref1.resize(size);
    ref2.resize(size);
    uut1.resize(size);
    uut2.resize(size);
  }

  std::vector<bool> ref1, ref2;
  cista::raw::bitvec uut1, uut2;
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
  auto uut = test_set{};

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

  uut.resize(1440);
  uut.init(gen);

  for (; data != end; ++data) {
    auto const o = static_cast<op>(data[0U] % NUM_OPS);
    auto const num = (data[0U] & 0xF0) % 1440;
    auto const value = (data[0U] & 0xF7) == 0;
    uut.apply_op(o, num, value);
  }

  return 0;
}
#endif
