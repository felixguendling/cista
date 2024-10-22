#include <cista/containers/mmap_vec.h>
#include <cista/mmap.h>
#include <cinttypes>
#include <cstring>
#include <filesystem>
#include <iostream>

#include "cista/containers/rtree.h"
extern "C" {
#include <stdatomic.h>
#include "dependencies/rtree_c/rtree_c.h"
}



using cista_rtree = cista::rtree<uint32_t, 2, double>;
using mm_rtree = cista::rtree<uint32_t, 2, double, 64, uint32_t, cista::mmap_vec_map>;


struct ctx_entry {
  cista_rtree::coord_t min, max;
  uint32_t data;
  bool operator==(ctx_entry other) const{
    return cista_rtree::rect::coord_t_equal(this->min, other.min) && cista_rtree::rect::coord_t_equal(this->max, other.max) && this->data == other.data;
  }
};

struct iter_ref_ctx {
  size_t count;
  uint32_t expected_value;
  uint32_t expected_value_mm;
  std::vector<ctx_entry> found_entries;
  std::vector<ctx_entry> found_entries_mm;
};

bool iter_ref(const double *min, const double *max, const void *data,
              void *udata)
{
  auto *ctx = static_cast<iter_ref_ctx*>(udata);
  auto input_data = static_cast<uint32_t>(reinterpret_cast<uint64_t>(data));
  ctx_entry search_entry{.min = {static_cast<float>(min[0]), static_cast<float>(min[1])}, .max = {static_cast<float>(max[0]), static_cast<float>(max[1])}, .data = input_data};

  for (auto & found_entrie : ctx->found_entries) {
    if (found_entrie == search_entry) {
      for (auto & found_entrie_mm : ctx->found_entries_mm) {
        if (found_entrie_mm == search_entry) {
          ctx->count++;
          return true;
        }
      }
    }
  }
  return true;
}


int test_bench(uint8_t const* data, size_t size) {

  /**
   * Test data usage:
   * b0 - b1: operator [insert, delete, search, save/load]
   * b8 - 135: 4x 32bit float for rects [min_0, max_0, min_1, max_1]
   * b136 - b168: int data
   */
   // Add memory dump fuzzing

  enum operator_ {
    SEARCH = 0,
    INSERT = 1,
    DELETE = 2,
    SAVE_LOAD = 3,
  };

  auto f = cista::mmap{"./mmap_rtree_nodes.bin", cista::mmap::protection::WRITE};


  auto ref_tree = rtree_new_with_allocator(malloc, free);
  cista_rtree uut_tree;
  mm_rtree cista_mm_rtree{mm_rtree::rect(), mm_rtree::node_idx_t::invalid(), mm_rtree::node_idx_t::invalid(), 0, 0, {}, cista::mmap_vec_map<mm_rtree::node_idx_t, mm_rtree::node>{std::move(f)}};


  for (size_t current_position = 0; current_position + 21 < size;) {
    uint8_t op = data[current_position] >> 6;
    uint8_t min_0[4] = {data[current_position + 1], data[current_position + 2], data[current_position + 3], data[current_position + 4]};
    uint8_t min_1[4] = {data[current_position + 5], data[current_position + 6], data[current_position + 7], data[current_position + 8]};
    uint8_t max_0[4] = {data[current_position + 9], data[current_position + 10], data[current_position + 11], data[current_position + 12]};
    uint8_t max_1[4] = {data[current_position + 13], data[current_position + 14], data[current_position + 15], data[current_position + 16]};
    uint32_t input_data = data[current_position + 17] + (data[current_position + 18] << 8) + (data[current_position + 19] << 16) + (data[current_position + 20] << 24);
    uint32_t min_0u, max_0u, min_1u, max_1u;

    memcpy(&min_0u, &min_0, sizeof(float));
    memcpy(&max_0u, &max_0, sizeof(float));
    memcpy(&min_1u, &min_1, sizeof(float));
    memcpy(&max_1u, &max_1, sizeof(float));

    float min_0f = static_cast<float>(static_cast<double>(min_0u) / 65536);
    float max_0f = static_cast<float>(static_cast<double >(max_0u) / 65536);
    float min_1f = static_cast<float>(static_cast<double>(min_1u) / 65536);
    float max_1f = static_cast<float>(static_cast<double>(max_1u) / 65536);

    float min_0_in = std::min(min_0f, max_0f);
    float max_0_in = std::max(min_0f, max_0f);
    float min_1_in = std::min(min_1f, max_1f);
    float max_1_in = std::max(min_1f, max_1f);

    double rect_min[] = {min_0_in, min_1_in};
    double rect_max[] = {max_0_in, max_1_in};

    cista_rtree::coord_t coord_min = {min_0_in, min_1_in};
    cista_rtree::coord_t coord_max = {max_0_in, max_1_in};

    if (op == SEARCH) {
      iter_ref_ctx ctx{};
      ctx.count = 0;
      ctx.expected_value = 0;
      uut_tree.search(coord_min, coord_max, [coord_min, coord_max, &ctx](cista_rtree::coord_t const& min_temp, cista_rtree::coord_t const& max_temp, uint32_t data){
        if (cista_rtree::rect::coord_t_equal(coord_min, min_temp) && cista_rtree::rect::coord_t_equal(coord_max, max_temp)) {
          ctx.expected_value++;
          ctx_entry input_entry{};
          input_entry.min = min_temp;
          input_entry.max = max_temp;
          input_entry.data = data;
          ctx.found_entries.emplace_back(input_entry);
        }
        return true;
      });

      ctx.expected_value_mm = 0;
      cista_mm_rtree.search(coord_min, coord_max, [coord_min, coord_max, &ctx](cista_rtree::coord_t const& min_temp, cista_rtree::coord_t const& max_temp, uint32_t data){
        if (cista_rtree::rect::coord_t_equal(coord_min, min_temp) && cista_rtree::rect::coord_t_equal(coord_max, max_temp)) {
          ctx.expected_value_mm++;
          ctx_entry input_entry{};
          input_entry.min = min_temp;
          input_entry.max = max_temp;
          input_entry.data = data;
          ctx.found_entries_mm.emplace_back(input_entry);
        }
        return true;
      });

      rtree_search(ref_tree, rect_min, rect_max, iter_ref, &ctx);

      if (ctx.count != ctx.expected_value || ctx.count != ctx.expected_value_mm || ctx.expected_value != ctx.expected_value_mm) {
        rtree_free(ref_tree);
        abort();
      }
    }
    if (op == INSERT) {
      uut_tree.insert(coord_min, coord_max, input_data);
      cista_mm_rtree.insert(coord_min, coord_max, input_data);
      rtree_insert(ref_tree, rect_min, rect_max,reinterpret_cast<uintptr_t*>(static_cast<uint64_t>(input_data)));
    }
    if (op == DELETE) {
      uut_tree.delete_element(coord_min, coord_max, input_data);
      cista_mm_rtree.delete_element(coord_min, coord_max, input_data);
      rtree_delete(ref_tree, rect_min, rect_max, reinterpret_cast<uintptr_t*>(static_cast<uint64_t>(input_data)));
    }

    if (op == SAVE_LOAD) {
      std::fstream file;
      file.open("mmap_rtree_header.bin", std::ios::binary | std::ios::out);
      cista_mm_rtree.write_header_bin(file);
      file.close();

      auto save_load_file = cista::mmap{"./mmap_rtree_nodes.bin", cista::mmap::protection::MODIFY};
      cista_mm_rtree.nodes_ = cista::mmap_vec_map<mm_rtree::node_idx_t, mm_rtree::node>{std::move(save_load_file)};

      file.open("mmap_rtree_header.bin", std::ios::binary | std::ios::in);
      cista_mm_rtree.read_header_bin(file);
      file.close();
    }

    current_position = current_position + 21;
  }

  rtree_free(ref_tree);
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
  return test_bench(m.data(), m.size());
}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
  return test_bench(data, size);
}
#endif