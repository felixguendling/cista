#include <cinttypes>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>

#include "cista/containers/mutable_fws_multimap.h"
#include "cista/mmap.h"

using my_key = cista::strong<std::uint32_t, struct _key>;

/*
struct my_entry {
  my_entry() : i_{static_cast<int*>(malloc(sizeof(int)))} {
    cista::verify(i_ != nullptr, "bad malloc");
    *i_ = 0;
  }

  my_entry(my_entry const& e) : i_{static_cast<int*>(malloc(sizeof(int)))} {
    cista::verify(i_ != nullptr, "bad malloc");
    if (e.i_ != nullptr) {
      *i_ = *e.i_;
    }
  }

  my_entry(my_entry&& e) noexcept : i_{e.i_} { e.i_ = nullptr; }

  my_entry& operator=(my_entry const& e) {
    if (&e == this) {
      return *this;
    }
    if (i_ == nullptr) {
      i_ = static_cast<int*>(malloc(sizeof(int)));
    }
    cista::verify(i_ != nullptr, "bad malloc");
    *i_ = *e.i_;
    return *this;
  }

  my_entry& operator=(my_entry&& e) noexcept {
    std::free(i_);
    i_ = e.i_;
    e.i_ = nullptr;
    return *this;
  }

  constexpr bool operator==(my_entry const& o) const {
    return i_ == nullptr && o.i_ == nullptr ||
           (i_ != nullptr && o.i_ != nullptr && *i_ == *o.i_);
  }

  friend std::ostream& operator<<(std::ostream& out, my_entry const& e) {
    if (e.i_ == nullptr) {
      return out << "nullptr";
    } else {
      return out << e.i_;
    }
  }

  ~my_entry() { std::free(i_); }

  int* i_{nullptr};
};
*/

using my_entry = int;

int run(std::uint8_t const* data, size_t const size) {
  cista::offset::mutable_fws_multimap<my_key, my_entry> uut;
  std::multimap<my_key, my_entry> ref;

  my_key key;
  my_entry value;
  constexpr auto const block_size = 1 + sizeof(key.v_) + sizeof(int);
  for (auto bytes_to_process = size; bytes_to_process > block_size;
       bytes_to_process -= block_size, data += block_size) {
    auto const insert = data[8] <= 128 ? true : false;

    std::memcpy(&key.v_, data, sizeof(key.v_));
    std::memcpy(&value, data + sizeof(key.v_), sizeof(int));

    if (key > 1'000'000) {
      continue;
    }

    if (insert) {
      std::cerr << "insert key=" << key << ", value=" << value << "\n";
      ref.emplace(key, value);
      uut.get_or_create(key).emplace_back(value);
    } else {
      std::cerr << "erase key=" << key << "\n";
      ref.erase(key);
      uut.erase(key);
    }
  }

  auto const print_range = [](auto const i1, auto const i2, auto&& fn) {
    std::cerr << "{";
    for (auto it = i1; it != i2; ++it) {
      std::cerr << fn(*it) << ", ";
    }
    std::cerr << "}" << std::endl;
  };

  for (auto const& bucket : uut) {
    using std::begin;
    using std::end;
    auto const key = bucket.index();
    auto const ref_range = ref.equal_range(my_key{key});
    auto const equal = std::equal(
        begin(bucket), end(bucket), ref_range.first, ref_range.second,
        [](my_entry const& a, std::pair<my_key, my_entry> const& b) {
          return a == b.second;
        });

    if (!equal) {
      std::cerr << "uut:\n";
      print_range(begin(bucket), end(bucket), [](auto&& x) { return x; });

      std::cerr << "ref:\n";
      print_range(ref_range.first, ref_range.second,
                  [](auto&& x) { return x.second; });
      abort();
    }
  }

  for (auto const& [k, v] : ref) {
    auto const bucket = uut.at(k);
    auto const found = std::find(begin(bucket), end(bucket), v) != end(bucket);
    if (!found) {
      abort();
    }
  }

  return 0;
}

#if defined(GENERATE_SEED)
int main() {}
#elif defined(MAIN)
int main(int ac, char** av) {
  if (ac < 2) {
    std::cout << "usage: " << av[0] << " [crash-file]\n";
    return 0;
  }

  if (!std::filesystem::is_regular_file(av[1])) {
    std::cout << "file " << av[1] << " does not exist\n";
    return 0;
  }

  cista::mmap m{av[1], cista::mmap::protection::READ};
  return run(m.data(), m.size());
}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
  return run(data, size);
}
#endif
