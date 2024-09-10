// Copyright 2023 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "rtree_c.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

////////////////////////////////

#define DATATYPE void *
#define DIMS 2
#define NUMTYPE double
#define MAXITEMS 64

////////////////////////////////

// used for splits
#define MINITEMS_PERCENTAGE 10
#define MINITEMS ((MAXITEMS) * (MINITEMS_PERCENTAGE) / 100 + 1)

#ifndef RTREE_NOPATHHINT
#define USE_PATHHINT
#endif

#ifdef RTREE_MAXITEMS
#undef MAXITEMS
#define MAXITEMS RTREE_MAXITEMS
#endif

#ifdef RTREE_NOATOMICS
typedef int rc_t;
static int rc_load(rc_t *ptr, bool relaxed) {
  (void)relaxed; // nothing to do
  return *ptr;
}
static int rc_fetch_sub(rc_t *ptr, int val) {
  int rc = *ptr;
  *ptr -= val;
  return rc;
}
static int rc_fetch_add(rc_t *ptr, int val) {
  int rc = *ptr;
  *ptr += val;
  return rc;
}
#else
#include <stdatomic.h>
typedef atomic_int rc_t;
static int rc_load(rc_t *ptr, bool relaxed) {
  if (relaxed) {
    return atomic_load_explicit(ptr, memory_order_relaxed);
  } else {
    return atomic_load(ptr);
  }
}
static int rc_fetch_sub(rc_t *ptr, int delta) {
  return atomic_fetch_sub(ptr, delta);
}
static int rc_fetch_add(rc_t *ptr, int delta) {
  return atomic_fetch_add(ptr, delta);
}
#endif

enum kind {
  LEAF = 1,
  BRANCH = 2,
};

struct rect {
  NUMTYPE min[DIMS];
  NUMTYPE max[DIMS];
};

struct item {
  const DATATYPE data;
};

struct node {
  rc_t rc;            // reference counter for copy-on-write
  enum kind kind;     // LEAF or BRANCH
  int count;          // number of rects
  struct rect rects[MAXITEMS];
  union {
    struct node *nodes[MAXITEMS];
    struct item datas[MAXITEMS];
  };
};

struct rtree {
  struct rect rect;
  struct node *root;
  size_t count;
  size_t height;
#ifdef USE_PATHHINT
  int path_hint[16];
#endif
  bool relaxed;
  void *(*malloc)(size_t);
  void (*free)(void *);
  void *udata;
  bool (*item_clone)(const DATATYPE item, DATATYPE *into, void *udata);
  void (*item_free)(const DATATYPE item, void *udata);
};

static inline NUMTYPE min0(NUMTYPE x, NUMTYPE y) {
  return x < y ? x : y;
}

static inline NUMTYPE max0(NUMTYPE x, NUMTYPE y) {
  return x > y ? x : y;
}

static bool feq(NUMTYPE a, NUMTYPE b) {
  return !(a < b || a > b);
}


void rtree_set_udata(struct rtree *tr, void *udata) {
  tr->udata = udata;
}

static struct node *node_new(struct rtree *tr, enum kind kind) {
  struct node *node = (struct node *)tr->malloc(sizeof(struct node));
  if (!node) return NULL;
  memset(node, 0, sizeof(struct node));
  node->kind = kind;
  return node;
}

static struct node *node_copy(struct rtree *tr, struct node *node) {
  struct node *node2 = (struct node *)tr->malloc(sizeof(struct node));
  if (!node2) return NULL;
  memcpy(node2, node, sizeof(struct node));
  node2->rc = 0;
  if (node2->kind == BRANCH) {
    for (int i = 0; i < node2->count; i++) {
      rc_fetch_add(&node2->nodes[i]->rc, 1);
    }
  } else {
    if (tr->item_clone) {
      int n = 0;
      bool oom = false;
      for (int i = 0; i < node2->count; i++) {
        if (!tr->item_clone(node->datas[i].data,
                            (DATATYPE*)&node2->datas[i].data, tr->udata))
        {
          oom = true;
          break;
        }
        n++;
      }
      if (oom) {
        if (tr->item_free) {
          for (int i = 0; i < n; i++) {
            tr->item_free(node2->datas[i].data, tr->udata);
          }
        }
        tr->free(node2);
        return NULL;
      }
    }
  }
  return node2;
}

static void node_free(struct rtree *tr, struct node *node) {
  if (rc_fetch_sub(&node->rc, 1) > 0) return;
  if (node->kind == BRANCH) {
    for (int i = 0; i < node->count; i++) {
      node_free(tr, node->nodes[i]);
    }
  } else {
    if (tr->item_free) {
      for (int i = 0; i < node->count; i++) {
        tr->item_free(node->datas[i].data, tr->udata);
      }
    }
  }
  tr->free(node);
}

#define cow_node_or(rnode, code) { \
    if (rc_load(&(rnode)->rc, tr->relaxed) > 0) { \
        struct node *node2 = node_copy(tr, (rnode)); \
        if (!node2) { code; } \
        node_free(tr, rnode); \
        (rnode) = node2; \
    } \
}

static void rect_expand(struct rect *rect, const struct rect *other) {
  for (int i = 0; i < DIMS; i++) {
    rect->min[i] = min0(rect->min[i], other->min[i]);
    rect->max[i] = max0(rect->max[i], other->max[i]);
  }
}

static NUMTYPE rect_area(const struct rect *rect) {
  NUMTYPE result = 1;
  for (int i = 0; i < DIMS; i++) {
    result *= (rect->max[i] - rect->min[i]);
  }
  return result;
}

// return the area of two rects expanded
static NUMTYPE rect_unioned_area(const struct rect *rect,
                                 const struct rect *other)
{
  NUMTYPE result = 1;
  for (int i = 0; i < DIMS; i++) {
    result *= (max0(rect->max[i], other->max[i]) -
               min0(rect->min[i], other->min[i]));
  }
  return result;
}

static bool rect_contains(const struct rect *rect, const struct rect *other) {
  int bits = 0;
  for (int i = 0; i < DIMS; i++) {
    bits |= other->min[i] < rect->min[i];
    bits |= other->max[i] > rect->max[i];
  }
  return bits == 0;
}

static bool rect_intersects(const struct rect *rect, const struct rect *other) {
  int bits = 0;
  for (int i = 0; i < DIMS; i++) {
    bits |= other->min[i] > rect->max[i];
    bits |= other->max[i] < rect->min[i];
  }
  return bits == 0;
}

static bool rect_onedge(const struct rect *rect, const struct rect *other) {
  for (int i = 0; i < DIMS; i++) {
    if (feq(rect->min[i], other->min[i]) ||
        feq(rect->max[i], other->max[i]))
    {
      return true;
    }
  }
  return false;
}

static bool rect_equals(const struct rect *rect, const struct rect *other) {
  for (int i = 0; i < DIMS; i++) {
    if (!feq(rect->min[i], other->min[i]) ||
        !feq(rect->max[i], other->max[i]))
    {
      return false;
    }
  }
  return true;
}

static bool rect_equals_bin(const struct rect *rect, const struct rect *other) {
  for (int i = 0; i < DIMS; i++) {
    if (rect->min[i] != other->min[i] ||
        rect->max[i] != other->max[i])
    {
      return false;
    }
  }
  return true;
}

static int rect_largest_axis(const struct rect *rect) {
  int axis = 0;
  NUMTYPE nlength = rect->max[0] - rect->min[0];
  for (int i = 1; i < DIMS; i++) {
    NUMTYPE length = rect->max[i] - rect->min[i];
    if (length > nlength) {
      nlength = length;
      axis = i;
    }
  }
  return axis;
}

// swap two rectangles
static void node_swap(struct node *node, int i, int j) {
  struct rect tmp = node->rects[i];
  node->rects[i] = node->rects[j];
  node->rects[j] = tmp;
  if (node->kind == LEAF) {
    struct item tmp = node->datas[i];
    node->datas[i] = node->datas[j];
    node->datas[j] = tmp;
  } else {
    struct node *tmp = node->nodes[i];
    node->nodes[i] = node->nodes[j];
    node->nodes[j] = tmp;
  }
}

struct rect4 {
  NUMTYPE all[DIMS*2];
};

static void node_qsort(struct node *node, int s, int e, int index) {
  int nrects = e - s;
  if (nrects < 2) {
    return;
  }
  int left = 0;
  int right = nrects-1;
  int pivot = nrects / 2;
  node_swap(node, s+pivot, s+right);
  struct rect4 *rects = (struct rect4 *)&node->rects[s];
  for (int i = 0; i < nrects; i++) {
    if (rects[right].all[index] < rects[i].all[index]) {
      node_swap(node, s+i, s+left);
      left++;
    }
  }
  node_swap(node, s+left, s+right);
  node_qsort(node, s, s+left, index);
  node_qsort(node, s+left+1, e, index);
}

// sort the node rectangles by the axis. used during splits
static void node_sort_by_axis(struct node *node, int axis, bool max) {
  int by_index = max ? DIMS+axis : axis;
  node_qsort(node, 0, node->count, by_index);
}

static void node_move_rect_at_index_into(struct node *from, int index,
                                         struct node *into)
{
  into->rects[into->count] = from->rects[index];
  from->rects[index] = from->rects[from->count-1];
  if (from->kind == LEAF) {
    into->datas[into->count] = from->datas[index];
    from->datas[index] = from->datas[from->count-1];
  } else {
    into->nodes[into->count] = from->nodes[index];
    from->nodes[index] = from->nodes[from->count-1];
  }
  from->count--;
  into->count++;
}

static bool node_split_largest_axis_edge_snap(struct rtree *tr,
                                              struct rect *rect, struct node *node, struct node **right_out)
{
  int axis = rect_largest_axis(rect);
  struct node *right = node_new(tr, node->kind);
  if (!right) {
    return false;
  }
  for (int i = 0; i < node->count; i++) {
    NUMTYPE min_dist = node->rects[i].min[axis] - rect->min[axis];
    NUMTYPE max_dist = rect->max[axis] - node->rects[i].max[axis];
    if (max_dist < min_dist) {
      // move to right
      node_move_rect_at_index_into(node, i, right);
      i--;
    }
  }
  // Make sure that both left and right nodes have at least
  // MINITEMS by moving datas into underflowed nodes.
  if (node->count < MINITEMS) {
    // reverse sort by min axis
    node_sort_by_axis(right, axis, false);
    do {
      node_move_rect_at_index_into(right, right->count-1, node);
    } while (node->count < MINITEMS);
  } else if (right->count < MINITEMS) {
    // reverse sort by max axis
    node_sort_by_axis(node, axis, true);
    do {
      node_move_rect_at_index_into(node, node->count-1, right);
    } while (right->count < MINITEMS);
  }
  if (node->kind == BRANCH) {
    node_sort_by_axis(node, 0, false);
    node_sort_by_axis(right, 0, false);
  }
  *right_out = right;
  return true;
}

static bool node_split(struct rtree *tr, struct rect *rect, struct node *node,
                       struct node **right)
{
  return node_split_largest_axis_edge_snap(tr, rect, node, right);
}

static int node_choose_least_enlargement(const struct node *node,
                                         const struct rect *ir)
{
  int j = 0;
  NUMTYPE jenlarge = INFINITY;
  for (int i = 0; i < node->count; i++) {
    // calculate the enlarged area
    NUMTYPE uarea = rect_unioned_area(&node->rects[i], ir);
    NUMTYPE area = rect_area(&node->rects[i]);
    NUMTYPE enlarge = uarea - area;
    if (enlarge < jenlarge) {
      j = i;
      jenlarge = enlarge;
    }
  }
  return j;
}

static int node_choose(struct rtree *tr, const struct node *node,
                       const struct rect *rect, int depth)
{
#ifdef USE_PATHHINT
  int h = tr->path_hint[depth];
  if (h < node->count) {
    if (rect_contains(&node->rects[h], rect)) {
      return h;
    }
  }
#endif
  // Take a quick look for the first node that contain the rect.
  for (int i = 0; i < node->count; i++) {
    if (rect_contains(&node->rects[i], rect)) {
#ifdef USE_PATHHINT
      tr->path_hint[depth] = i;
#endif
      return i;
    }
  }
  // Fallback to using che "choose least enlargment" algorithm.
  int i = node_choose_least_enlargement(node, rect);
#ifdef USE_PATHHINT
  tr->path_hint[depth] = i;
#endif
  return i;
}

static struct rect node_rect_calc(const struct node *node) {
  struct rect rect = node->rects[0];
  for (int i = 1; i < node->count; i++) {
    rect_expand(&rect, &node->rects[i]);
  }
  return rect;
}

// node_insert returns false if out of memory
static bool node_insert(struct rtree *tr, struct rect *nr, struct node *node,
                        struct rect *ir, struct item item, int depth, bool *split)
{
  if (node->kind == LEAF) {
    if (node->count == MAXITEMS) {
      *split = true;
      return true;
    }
    int index = node->count;
    node->rects[index] = *ir;
    node->datas[index] = item;
    node->count++;
    *split = false;
    return true;
  }
  // Choose a subtree for inserting the rectangle.
  int i = node_choose(tr, node, ir, depth);
  cow_node_or(node->nodes[i], return false);
  if (!node_insert(tr, &node->rects[i], node->nodes[i], ir, item, depth+1,
                   split))
  {
    return false;
  }
  if (!*split) {
    rect_expand(&node->rects[i], ir);
    *split = false;
    return true;
  }
  // split the child node
  if (node->count == MAXITEMS) {
    *split = true;
    return true;
  }
  struct node *right;
  if (!node_split(tr, &node->rects[i], node->nodes[i], &right)) {
    return false;
  }
  node->rects[i] = node_rect_calc(node->nodes[i]);
  node->rects[node->count] = node_rect_calc(right);
  node->nodes[node->count] = right;
  node->count++;
  return node_insert(tr, nr, node, ir, item, depth, split);
}

struct rtree *rtree_new_with_allocator(void *(*_malloc)(size_t),
                                       void (*_free)(void*)
) {
  _malloc = _malloc ? _malloc : malloc;
  _free = _free ? _free : free;
  struct rtree *tr = (struct rtree *)_malloc(sizeof(struct rtree));
  if (!tr) return NULL;
  memset(tr, 0, sizeof(struct rtree));
  tr->malloc = _malloc;
  tr->free = _free;
  return tr;
}

struct rtree *rtree_new(void) {
  return rtree_new_with_allocator(NULL, NULL);
}

void rtree_set_item_callbacks(struct rtree *tr,
                              bool (*clone)(const DATATYPE item, DATATYPE *into, void *udata),
                              void (*free)(const DATATYPE item, void *udata))
{
  tr->item_clone = clone;
  tr->item_free = free;
}

bool rtree_insert(struct rtree *tr, const NUMTYPE *min,
                  const NUMTYPE *max, const DATATYPE data)
{
  // copy input rect
  struct rect rect;
  memcpy(&rect.min[0], min, sizeof(NUMTYPE)*DIMS);
  memcpy(&rect.max[0], max?max:min, sizeof(NUMTYPE)*DIMS);

  // copy input data
  struct item item;
  if (tr->item_clone) {
    if (!tr->item_clone(data, (DATATYPE*)&item.data, tr->udata)) {
      return false;
    }
  } else {
    memcpy(&item.data, &data, sizeof(DATATYPE));
  }

  while (1) {
    if (!tr->root) {
      struct node *new_root = node_new(tr, LEAF);
      if (!new_root) {
        break;
      }
      tr->root = new_root;
      tr->rect = rect;
      tr->height = 1;
    }
    bool split = false;
    cow_node_or(tr->root, break);
    if (!node_insert(tr, &tr->rect, tr->root, &rect, item, 0, &split)) {
      break;
    }
    if (!split) {
      rect_expand(&tr->rect, &rect);
      tr->count++;
      return true;
    }
    struct node *new_root = node_new(tr, BRANCH);
    if (!new_root) {
      break;
    }
    struct node *right;
    if (!node_split(tr, &tr->rect, tr->root, &right)) {
      tr->free(new_root);
      break;
    }
    new_root->rects[0] = node_rect_calc(tr->root);
    new_root->rects[1] = node_rect_calc(right);
    new_root->nodes[0] = tr->root;
    new_root->nodes[1] = right;
    tr->root = new_root;
    tr->root->count = 2;
    tr->height++;
  }
  // out of memory
  if (tr->item_free) {
    tr->item_free(item.data, tr->udata);
  }
  return false;
}

void rtree_free(struct rtree *tr) {
  if (tr->root) {
    node_free(tr, tr->root);
  }
  tr->free(tr);
}

static bool node_search(struct node *node, struct rect *rect,
                        bool (*iter)(const NUMTYPE *min, const NUMTYPE *max, const DATATYPE data,
                                     void *udata),
                        void *udata)
{
  if (node->kind == LEAF) {
    for (int i = 0; i < node->count; i++) {
      if (rect_intersects(&node->rects[i], rect)) {
        if (!iter(node->rects[i].min, node->rects[i].max,
                  node->datas[i].data, udata))
        {
          return false;
        }
      }
    }
    return true;
  }
  for (int i = 0; i < node->count; i++) {
    if (rect_intersects(&node->rects[i], rect)) {
      if (!node_search(node->nodes[i], rect, iter, udata)) {
        return false;
      }
    }
  }
  return true;
}

void rtree_search(const struct rtree *tr, const NUMTYPE min[],
                  const NUMTYPE max[],
                  bool (*iter)(const NUMTYPE min[], const NUMTYPE max[], const DATATYPE data,
                               void *udata),
                  void *udata)
{
  // copy input rect
  struct rect rect;
  memcpy(&rect.min[0], min, sizeof(NUMTYPE)*DIMS);
  memcpy(&rect.max[0], max?max:min, sizeof(NUMTYPE)*DIMS);

  if (tr->root) {
    node_search(tr->root, &rect, iter, udata);
  }
}

static bool node_scan(struct node *node,
                      bool (*iter)(const NUMTYPE *min, const NUMTYPE *max, const DATATYPE data,
                                   void *udata),
                      void *udata)
{
  if (node->kind == LEAF) {
    for (int i = 0; i < node->count; i++) {
      if (!iter(node->rects[i].min, node->rects[i].max,
                node->datas[i].data, udata))
      {
        return false;
      }
    }
    return true;
  }
  for (int i = 0; i < node->count; i++) {
    if (!node_scan(node->nodes[i], iter, udata)) {
      return false;
    }
  }
  return true;
}

void rtree_scan(const struct rtree *tr,
                bool (*iter)(const NUMTYPE *min, const NUMTYPE *max, const DATATYPE data,
                             void *udata),
                void *udata)
{
  if (tr->root) {
    node_scan(tr->root, iter, udata);
  }
}

size_t rtree_count(const struct rtree *tr) {
  return tr->count;
}

static bool node_delete(struct rtree *tr, struct rect *nr, struct node *node,
                        struct rect *ir, struct item item, int depth, bool *removed, bool *shrunk,
                        int (*compare)(const DATATYPE a, const DATATYPE b, void *udata),
                        void *udata)
{
  *removed = false;
  *shrunk = false;
  if (node->kind == LEAF) {
    for (int i = 0; i < node->count; i++) {
      if (!rect_equals_bin(ir, &node->rects[i])) {
        // Must be exactly the same, binary comparison.
        continue;
      }
      int cmp = compare ?
                        compare(node->datas[i].data, item.data, udata) :
                        memcmp(&node->datas[i].data, &item.data, sizeof(DATATYPE));
      if (cmp != 0) {
        continue;
      }
      // Found the target item to delete.
      if (tr->item_free) {
        tr->item_free(node->datas[i].data, tr->udata);
      }
      node->rects[i] = node->rects[node->count-1];
      node->datas[i] = node->datas[node->count-1];
      node->count--;
      if (rect_onedge(ir, nr)) {
        // The item rect was on the edge of the node rect.
        // We need to recalculate the node rect.
        *nr = node_rect_calc(node);
        // Notify the caller that we shrunk the rect.
        *shrunk = true;
      }
      *removed = true;
      return true;
    }
    return true;
  }
  int h = 0;
#ifdef USE_PATHHINT
  h = tr->path_hint[depth];
  if (h < node->count) {
    if (rect_contains(&node->rects[h], ir)) {
      cow_node_or(node->nodes[h], return false);
      if (!node_delete(tr, &node->rects[h], node->nodes[h], ir, item,
                       depth+1,removed, shrunk, compare, udata))
      {
        return false;
      }
      if (*removed) {
        goto removed;
      }
    }
  }
  h = 0;
#endif
  for (; h < node->count; h++) {
    if (!rect_contains(&node->rects[h], ir)) {
      continue;
    }
    struct rect crect = node->rects[h];
    cow_node_or(node->nodes[h], return false);
    if (!node_delete(tr, &node->rects[h], node->nodes[h], ir, item, depth+1,
                     removed, shrunk, compare, udata))
    {
      return false;
    }
    if (!*removed) {
      continue;
    }
  removed:
    if (node->nodes[h]->count == 0) {
      // underflow
      node_free(tr, node->nodes[h]);
      node->rects[h] = node->rects[node->count-1];
      node->nodes[h] = node->nodes[node->count-1];
      node->count--;
      *nr = node_rect_calc(node);
      *shrunk = true;
      return true;
    }
#ifdef USE_PATHHINT
    tr->path_hint[depth] = h;
#endif
    if (*shrunk) {
      *shrunk = !rect_equals(&node->rects[h], &crect);
      if (*shrunk) {
        *nr = node_rect_calc(node);
      }
    }
    return true;
  }
  return true;
}

// returns false if out of memory
static bool rtree_delete0(struct rtree *tr, const NUMTYPE *min,
                          const NUMTYPE *max, const DATATYPE data,
                          int (*compare)(const DATATYPE a, const DATATYPE b, void *udata),
                          void *udata)
{
  // copy input rect
  struct rect rect;
  memcpy(&rect.min[0], min, sizeof(NUMTYPE)*DIMS);
  memcpy(&rect.max[0], max?max:min, sizeof(NUMTYPE)*DIMS);

  // copy input data
  struct item item;
  memcpy(&item.data, &data, sizeof(DATATYPE));

  if (!tr->root) {
    return true;
  }
  bool removed = false;
  bool shrunk = false;
  cow_node_or(tr->root, return false);
  if (!node_delete(tr, &tr->rect, tr->root, &rect, item, 0, &removed, &shrunk,
                   compare, udata))
  {
    return false;
  }
  if (!removed) {
    return true;
  }
  tr->count--;
  if (tr->count == 0) {
    node_free(tr, tr->root);
    tr->root = NULL;
    memset(&tr->rect, 0, sizeof(struct rect));
    tr->height = 0;
  } else {
    while (tr->root->kind == BRANCH && tr->root->count == 1) {
      struct node *prev = tr->root;
      tr->root = tr->root->nodes[0];
      prev->count = 0;
      node_free(tr, prev);
      tr->height--;
    }
    if (shrunk) {
      tr->rect = node_rect_calc(tr->root);
    }
  }
  return true;
}

bool rtree_delete(struct rtree *tr, const NUMTYPE *min, const NUMTYPE *max,
                  const DATATYPE data)
{
  return rtree_delete0(tr, min, max, data, NULL, NULL);
}

bool rtree_delete_with_comparator(struct rtree *tr, const NUMTYPE *min,
                                  const NUMTYPE *max, const DATATYPE data,
                                  int (*compare)(const DATATYPE a, const DATATYPE b, void *udata),
                                  void *udata)
{
  return rtree_delete0(tr, min, max, data, compare, udata);
}

struct rtree *rtree_clone(struct rtree *tr) {
  if (!tr) return NULL;
  struct rtree *tr2 = tr->malloc(sizeof(struct rtree));
  if (!tr2) return NULL;
  memcpy(tr2, tr, sizeof(struct rtree));
  if (tr2->root) rc_fetch_add(&tr2->root->rc, 1);
  return tr2;
}

void rtree_opt_relaxed_atomics(struct rtree *tr) {
  tr->relaxed = true;
}

#ifdef TEST_PRIVATE_FUNCTIONS
#include "tests/priv_funcs.h"
#endif
