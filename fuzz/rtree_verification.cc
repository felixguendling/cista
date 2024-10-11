#include <cinttypes>
#include <cstring>

#include "cista/containers/rtree.h"
extern "C" {
#include <stdatomic.h>
#include "dependencies/rtree_c/rtree_c.h"
}

struct iter_ref_ctx {
  size_t count;
  uint32_t expected_value;
};

bool iter_ref(const double *min, const double *max, const void *data,
              void *udata)
{
  (void)min; (void)max; (void)data;
  auto *ctx = static_cast<iter_ref_ctx*>(udata);
  ctx->count++;
  return true;
}


int test_bench(uint8_t const* data, size_t size) {

  /**
   * Test data usage:
   * b0 - b1: operator [insert, delete, search, no_op]
   * b8 - 135: 4x 32bit float for rects [min_0, max_0, min_1, max_1]
   * b136 - b168: int data
   */

  enum operator_ {
    SEARCH = 0,
    INSERT = 1,
    DELETE = 2
  };

  auto ref_tree = rtree_new_with_allocator(malloc, free);
  cista::rtree<uint32_t> uut_tree;

  for (size_t current_position = 0; current_position + 21 < size;) {
    uint8_t op = data[current_position] >> 6;
    uint8_t min_0[4] = {data[current_position + 1], data[current_position + 2], data[current_position + 3], data[current_position + 4]};
    uint8_t min_1[4] = {data[current_position + 5], data[current_position + 6], data[current_position + 7], data[current_position + 8]};
    uint8_t max_0[4] = {data[current_position + 9], data[current_position + 10], data[current_position + 11], data[current_position + 12]};
    uint8_t max_1[4] = {data[current_position + 13], data[current_position + 14], data[current_position + 15], data[current_position + 16]};
    int input_data = data[current_position + 17] + (data[current_position + 18] << 8) + (data[current_position + 19] << 16) + (data[current_position + 20] << 24);
    float min_0f, max_0f, min_1f, max_1f;

    memcpy(&min_0f, &min_0, sizeof(min_0f));
    memcpy(&max_0f, &max_0, sizeof(max_0f));
    memcpy(&min_1f, &min_1, sizeof(min_1f));
    memcpy(&max_1f, &max_1, sizeof(max_1f));

    double rect_min[] = {min_0f, min_1f};
    double rect_max[] = {max_0f, max_1f};

    cista::rtree<int>::coord_t coord_min = {min_0f, min_1f};
    cista::rtree<int>::coord_t coord_max = {max_0f, max_1f};

    if (op == SEARCH) {
      iter_ref_ctx ctx{};
      ctx.count = 0;
      ctx.expected_value = 0;
      uut_tree.search(coord_min, coord_max, [coord_min, coord_max, &ctx](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, uint32_t data){
        if (cista::rtree<int>::rect::coord_t_equal(coord_min, min_temp) && cista::rtree<int>::rect::coord_t_equal(coord_max, max_temp)) {
          ctx.expected_value++;
        }
        return true;
      });
      rtree_search(ref_tree, rect_min, rect_max, iter_ref, &ctx);

      if (ctx.count != ctx.expected_value) {
        rtree_free(ref_tree);
        return 1;
      }
    }
    if (op == INSERT) {
      uut_tree.insert(coord_min, coord_max, input_data);
      rtree_insert(ref_tree, rect_min, rect_max, (void *)(uintptr_t)input_data);
    }
    if (op == DELETE) {
      uut_tree.delete_element(coord_min, coord_max, input_data);
      rtree_delete(ref_tree, rect_min, rect_max, (void *)(uintptr_t)input_data);
    }

    current_position = current_position + 21;
  }

  rtree_free(ref_tree);
  return 0;
}



#if defined(GENERATE_SEED)
int main() {}
#elif defined(MAIN)
int main() {}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
  return test_bench(data, size);
}
#endif