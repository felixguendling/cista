#include <cinttypes>
#include <cstring>
#include <map>

#include "cista/containers/mutable_fws_multimap.h"

using my_key = cista::strong<std::uint32_t, struct _key>;

struct my_entry {
  my_entry() : i_{static_cast<int*>(malloc(sizeof(int)))} {
    cista::verify(i_ != nullptr, "bad malloc");
  }

  my_entry(my_entry const& e) : i_{static_cast<int*>(malloc(sizeof(int)))} {
    cista::verify(i_ != nullptr, "bad malloc");
    if (e.i_ != nullptr) {
      *i_ = *e.i_;
    }
  }

  my_entry(my_entry&& e) noexcept : i_{e.i_} { e.i_ = nullptr; }

  my_entry& operator=(my_entry const& e) {
    i_ = static_cast<int*>(malloc(sizeof(int)));
    cista::verify(i_ != nullptr, "bad malloc");
    if (e.i_ != nullptr) {
      *i_ = *e.i_;
    }
    return *this;
  }

  my_entry& operator=(my_entry&& e) noexcept {
    i_ = e.i_;
    e.i_ = nullptr;
    return *this;
  }

  ~my_entry() { std::free(i_); }
  int* i_{nullptr};
};

#if defined(GENERATE_SEED)
int main() {}
#else
/**
 * Check hash_map<int, int>
 *
 *   Input bytes:
 *      0-3: key
 *      4-7: value
 *        9: operation => less or equal 128 insert,
 *                     => greater 128 delete
 */
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
  cista::offset::mutable_fws_multimap<my_key, my_entry> uut;
  std::multimap<my_key, my_entry> ref;

  my_key key;
  my_entry value;
  constexpr auto const block_size = 1 + sizeof(key.v_) + sizeof(int);
  for (auto bytes_to_process = size; bytes_to_process > block_size;
       bytes_to_process -= block_size, data += block_size) {
    auto const insert = data[8] <= 128 ? true : false;

    std::memcpy(&key.v_, data, sizeof(key.v_));
    std::memcpy(value.i_, data + sizeof(key.v_), sizeof(int));

    if (insert) {
      ref.emplace(key, value);
      uut[key].emplace_back(std::move(value));
    } else {
      ref.erase(key);
      //      uut.erase(key);
    }
  }

  //  for (auto const& [key, value] : ref) {
  //    if (uut.find(key) == end(uut)) {
  //      abort();
  //    }
  //  }
  //
  //  for (auto const& [key, value] : uut) {
  //    if (ref.find(key) == end(ref)) {
  //      abort();
  //    }
  //  }

  return 0;
}
#endif
