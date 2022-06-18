#include <cinttypes>
#include <cstring>
#include <iostream>

#include <unordered_map>

#include "cista/containers/hash_map.h"

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
  std::unordered_map<int, int> ref;
  cista::raw::hash_map<int, int> uut;

  for (auto bytes_to_process = size; bytes_to_process > 8;
       bytes_to_process -= 9, data += 9) {
    auto key = 0;
    auto value = 0;
    auto const insert = data[8] <= 128 ? true : false;

    std::memcpy(&key, data, sizeof(key));
    std::memcpy(&value, data + sizeof(key), sizeof(value));

    if (insert) {
      printf("insert STD\n");
      ref.emplace(key, value);

      printf("insert CISTA\n");
      uut.emplace(key, value);
    } else {
      printf("erase STD\n");
      ref.erase(key);

      printf("erase CISTA\n");
      uut.erase(key);
    }
  }

  for (auto const& [key, value] : ref) {
    if (uut.find(key) == end(uut)) {
      abort();
    }
  }

  for (auto const& [key, value] : uut) {
    if (ref.find(key) == end(ref)) {
      abort();
    }
  }

  return 0;
}
#endif
