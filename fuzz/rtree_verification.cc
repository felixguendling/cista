//
// Created by Alexander on 28.06.2024.
//

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

atomic_int total_allocs = 0;
atomic_int total_mem = 0;

static bool rand_alloc_fail = false;
static int rand_alloc_fail_odds = 3;


static void* xmalloc(size_t size) {
  if (false && rand()%(rand_alloc_fail_odds* 1) == 0) {
    return NULL;
  }
  void *mem = malloc(sizeof(uint64_t)+size);
  assert(mem);
  *(uint64_t*)mem = size;
  atomic_fetch_add(&total_allocs, 1);
  atomic_fetch_add(&total_mem, (int)size);
  return (char*)mem+sizeof(uint64_t);
}



static void xfree(void *ptr) {
  if (ptr) {
    atomic_fetch_sub(&total_mem,
                     (int)(*(uint64_t*)((char*)ptr-sizeof(uint64_t))));
    atomic_fetch_sub(&total_allocs, 1);
    free((char*)ptr-sizeof(uint64_t));
  }
}


int test_bench(uint8_t const* data, size_t size) {

  /**
   * Test data usage:
   * b0 - b1: operator [insert, delete, search, no_op]
   * b2 - b7: array length [max 64 byte]
   * b8 - 135: 4x 32bit float for rects [min_0, max_0, min_1, max_1]
   * b135 - b[135 ... 647]: Data
   */

  enum operator_ {
    SEARCH = 0,
    INSERT = 1,
    DELETE = 2
  };

  struct bench_data_struct {
    uint8_t *data;
    size_t size;

    bool operator==(bench_data_struct other) {
      if (size == other.size) {
        for (size_t i = 0; i < size; ++i) {
          if (data[i] != other.data[i]) {
            return false;
          }
        }
        return true;
      }
      return false;
    }
  };

  //auto ref_tree = rtree_new_with_allocator(xmalloc, xfree);
  auto ref_tree = rtree_new();
  cista::rtree<bench_data_struct> uut_tree;

  for (size_t current_position = 0; current_position + 17 < size;) {
    uint8_t op = data[current_position] >> 6;
    uint8_t data_length = data[current_position] & 0x3F;
    uint8_t min_0[4] = {data[current_position + 1], data[current_position + 2], data[current_position + 3], data[current_position + 4]};
    uint8_t min_1[4] = {data[current_position + 5], data[current_position + 6], data[current_position + 7], data[current_position + 8]};
    uint8_t max_0[4] = {data[current_position + 9], data[current_position + 10], data[current_position + 11], data[current_position + 12]};
    uint8_t max_1[4] = {data[current_position + 13], data[current_position + 14], data[current_position + 15], data[current_position + 16]};
    float min_0f, max_0f, min_1f, max_1f;

    if (current_position + 17 + data_length >= size) {
      break;
    }

    memcpy(&min_0f, &min_0, sizeof(min_0f));
    memcpy(&max_0f, &max_0, sizeof(max_0f));
    memcpy(&min_1f, &min_1, sizeof(min_1f));
    memcpy(&max_1f, &max_1, sizeof(max_1f));

    double rect_min[] = {min_0f, min_1f};
    double rect_max[] = {max_0f, max_1f};

    cista::rtree<bench_data_struct>::coord_t coord_min = {min_0f, min_1f};
    cista::rtree<bench_data_struct>::coord_t coord_max = {max_0f, max_1f};

    auto *input_struct = new bench_data_struct();
    input_struct->size = data_length;
    input_struct->data = new uint8_t[data_length];

    for (size_t i = 0; i < data_length; i++) {
      input_struct->data[i] = data[current_position + 17 + i];
    }

    if (op == SEARCH) {
      iter_ref_ctx ctx{};
      ctx.count = 0;
      ctx.expected_value = 0;
      uut_tree.search(coord_min, coord_max, [&ctx](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, bench_data_struct data){
        ctx.expected_value++;
        return true;
      });
      rtree_search(ref_tree, rect_min, rect_max, iter_ref, &ctx);

      if (ctx.count != ctx.expected_value) {
        return 1;
      }
    }
    if (op == INSERT) {
      uut_tree.insert(coord_min, coord_max, *input_struct);
      rtree_insert(ref_tree, rect_min,
                   rect_max, (void*)input_struct);
    }
    if (op == DELETE) {
      uut_tree.delete_element(coord_min, coord_max, *input_struct);
      rtree_delete(ref_tree, rect_min, rect_max, (void*)input_struct);
    }

    current_position = current_position + 17 + data_length;
  }

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