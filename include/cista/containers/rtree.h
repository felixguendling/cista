#pragma once

#include <cinttypes>

#include "cista/cista_member_offset.h"
#include "cista/containers/array.h"
#include "cista/containers/variant.h"
#include "cista/containers/vector.h"
#include "cista/endian/conversion.h"

namespace cista {

template <typename Ctx, typename T>
void serialize(Ctx& c, T const* origin, offset_t const pos);

template <typename NumType>
inline bool feq(NumType a, NumType b) {
  return !(a < b || a > b);
}

template <typename DataType, unsigned Dims = 2U, typename NumType = float,
          unsigned MaxItems = 64U, typename SizeType = std::uint32_t>
struct rtree {
  static constexpr auto const kInfinity = std::numeric_limits<NumType>::max();

  // used for splits
  static constexpr auto const kMinItemsPercentage = 10U;
  static constexpr auto const kMinItems =
      MaxItems * (kMinItemsPercentage / 100 + 1);

  enum class kind : std::uint8_t { kLeaf, kBranch };

  using node_idx_t = strong<SizeType, struct node_idx_>;
  using coord_t = array<NumType, Dims>;

  struct rect {
    /**
     * The area of the bounding rectangle of this rect and other_rect
     * @param other_rect The second rectangle to calculate with
     * @return The bounding area of this rect and other_rect
     */
    NumType united_area(rect const& other_rect) const noexcept {
      auto result = NumType{1};
      for (auto i = 0U; i != Dims; ++i) {
        result *= (std::max(max_[i], other_rect.max_[i]) - std::min(min_[i], other_rect.min_[i]));
      }
      return result;
    }

    /**
     * Calculates the area of this rect
     * @return The area of this rectangle
     */
    NumType area() const noexcept {
      auto result = NumType{1};
      for (auto i = 0U; i != Dims; i++) {
        result *= (max_[i] - min_[i]);
      }
      return result;
    }

    /**
     * Checks if other_rect is contained in this rect
     * @param other_rect The rectangle to check
     * @return True if this rect contains other_rect
     */
    bool contains(rect const& other_rect) const noexcept {
      auto bits = 0U;
      for (auto i = 0U; i != Dims; i++) {
        bits |= (other_rect.min_[i] < min_[i]);
        bits |= (other_rect.max_[i] > max_[i]);
      }
      return bits == 0;
    }

    /**
     * Checks if this rectangle intersects with other_rect
     * @param other_rect The rectangle to check for intersection
     * @return True if the rectangles intersect.
     */
    bool intersects(rect const& other_rect) const noexcept {
      auto bits = 0;
      for (auto i = 0U; i != Dims; i++) {
        bits |= other_rect.min_[i] > max_[i];
        bits |= other_rect.max_[i] < min_[i];
      }
      return bits == 0;
    }

    /**
     * Checks if at least one edge of rect is touching other_rect
     * @param other_rect The rectangle to check with
     * @return True if the at least one of the edges of the rectangles are touching
     */
    bool onedge(rect const& other_rect) const noexcept {
      for (auto i = 0U; i < Dims; i++) {
        if (feq(min_[i], other_rect.min_[i]) || feq(max_[i], other_rect.max_[i])) {
          return true;
        }
      }
      return false;
    }

    /**
     * Expands the current rectangle by the coordinates of new_rect
     * @param new_rect The rectangle to expand by
     */
    void expand(rect const& new_rect) noexcept {
      for (auto i = 0U; i < Dims; i++) {
        min_[i] = std::min(min_[i], new_rect.min_[i]);
        max_[i] = std::max(max_[i], new_rect.max_[i]);
      }
    }

    /**
     * Calculates the longest side of this rectangle and returns it's axis
     * @return The axis of the longest side of the rectangle
     */
    unsigned largest_axis() const noexcept {
      auto axis = 0U;
      auto nlength = max_[0] - min_[0];
      for (unsigned i = 1; i != Dims; ++i) {
        auto const length = max_[i] - min_[i];
        if (length > nlength) {
          nlength = length;
          axis = i;
        }
      }
      return axis;
    }

    coord_t min_, max_;
  };

  struct node {
    /**
     * Sorts rectangles in a node along one axis
     * @param axis The axis to sort for
     * @param rev Decides if sorted ascending or descending
     * @param max If true sort with max coordinates else min coordinates
     */
    void sort_by_axis(unsigned const axis, bool const rev, bool const max) {
      auto const by_index = max ? static_cast<unsigned>(Dims + axis) : axis;
      qsort(0U, count_, by_index, rev);
    }

    /**
     * Quicksort implementation for sorting rectangles (and its attached data) inside of a node
     * @param start Start index of this partition
     * @param end End index + 1 of this partition
     * @param index Index for choosing the sorting axis of the rectangles
     * @param rev Decides if sorted ascending or descending
     */
    void qsort(unsigned start, unsigned end, unsigned index, bool rev) {
      // Simplification for the struct rect
      struct rect4 {
        NumType all[Dims * 2U];
      };

      unsigned nrects = end - start;
      if (nrects < 2) {
        return;
      }
      unsigned left = 0U;
      unsigned right = nrects - 1;
      unsigned pivot = nrects / 2;
      swap(start + pivot, start + right);
      auto const rects = reinterpret_cast<rect4 const*>(&rects_[start]);
      if (!rev) {
        for (auto i = 0U; i != nrects; ++i) {
          if (rects[i].all[index] < rects[right].all[index]) {
            swap(start + i, start + left);
            ++left;
          }
        }
      } else {
        for (auto i = 0U; i != nrects; ++i) {
          if (rects[right].all[index] < rects[i].all[index]) {
            swap(start + i, start + left);
            ++left;
          }
        }
      }
      swap(start + left, start + right);
      qsort(start, start + left, index, rev);
      qsort(start + left + 1, end, index, rev);
    }

    /**
     * Finds the index of the rect in this node with the least bounding area with insert_rect
     * @param insert_rect The rectangle to check the bounding area with
     * @return The index of th rect of this node with the least bounding area
     */
    unsigned choose_least_enlargement(rect const& insert_rect) const {
      auto j = 0U;
      auto j_enlarge = kInfinity;
      for (auto i = 0U; i < count_; i++) {
        // calculate the enlarged area
        auto const uarea = rects_[i].united_area(insert_rect);
        auto const area = rects_[i].area();
        auto const enlarge = uarea - area;
        if (enlarge < j_enlarge) {
          j = i;
          j_enlarge = enlarge;
        }
      }
      return j;
    }

    /**
     * Moves a rectangle and it's data from this node to into
     * @param index The index of the rect to move
     * @param into The node to move to
     */
    void move_rect_at_index_into(unsigned index, node& into) noexcept {
      into.rects_[into.count_] = rects_[index];
      rects_[index] = rects_[count_ - 1];
      if (kind_ == kind::kLeaf) {
        into.data_[into.count_] = data_[index];
        data_[index] = data_[count_ - 1];
      } else {
        into.children_[into.count_] = children_[index];
        children_[index] = children_[count_ - 1U];
      }
      --count_;
      ++into.count_;
    }

    /**
     * Swaps the rectangles at index i and j
     * @param i First index
     * @param j Second index
     */
    void swap(unsigned i, unsigned j) noexcept {
      std::swap(rects_[i], rects_[j]);
      if (kind_ == kind::kLeaf) {
        std::swap(data_[i], data_[j]);
      } else {
        std::swap(children_[i], children_[j]);
      }
    }

    /**
     * Calculates the bounding box of the rectangles in this node
     * @return The bounding rectangle of this node
     */
    rect rect_calc() const noexcept {
      assert(count_ <= MaxItems);
      auto temp_rect = rects_[0U];
      for (auto i = 1U; i < count_; ++i) {
        temp_rect.expand(rects_[i]);
      }
      return temp_rect;
    }

    template <typename Ctx>
    friend void serialize(Ctx& c, node const* origin, cista::offset_t const pos) {
      using Type = std::decay_t<decltype(*origin)>;
      c.write(pos + cista_member_offset(Type, count_), convert_endian<Ctx::MODE>(origin->count_));
      c.write(pos + cista_member_offset(Type, kind_), convert_endian<Ctx::MODE>(origin->kind_));
      c.write(pos + cista_member_offset(Type, rects_), convert_endian<Ctx::MODE>(origin->rects_));
      if (origin->kind_ == kind::kLeaf) {
        auto const size = static_cast<offset_t>(sizeof(DataType) * origin->data_.size());
        auto const data_start = pos + cista_member_offset(Type, data_);
        auto i = 0U;
        for (auto it = data_start; it != data_start + size; it += sizeof(DataType)) {
          serialize(c, &(origin->data_[i++]), it);
        }
      }
    }

    template <typename Ctx>
    friend void deserialize(Ctx const& ctx, node* el) {
      if (el->kind_ == kind::kLeaf) {
        deserialize(ctx, &el->data_);
      }
    }

    using node_array_t = array<node_idx_t, MaxItems>;
    using data_array_t = array<DataType, MaxItems>;

    unsigned count_{0U};
    kind kind_;
    array<rect, MaxItems> rects_;
    union {
      node_array_t children_;
      data_array_t data_;
    };
  };

  /**
   * Inserts a new rectangle with it's data into the data structure
   * @param min Min coordinates of the new rectangle
   * @param max Max coordinates of the new rectangle
   * @param data The data to store
   */
  void insert(coord_t const& min, coord_t const& max, DataType data) {
    auto const insert_rect = rect{min, max};
    while (true) {
      if (root_ == node_idx_t::invalid()) {
        root_ = node_new(kind::kLeaf);
        rect_ = insert_rect;
        height_ = 1U;
      }

      auto split = false;
      node_insert(rect_, root_, insert_rect, std::move(data), 0, split);
      if (!split) {
        rect_.expand(insert_rect);
        ++count_;
        return;
      }

      // Something goes wrong right here :(
      auto new_root_idx = node_new(kind::kBranch);

      auto right = node_idx_t::invalid();
      node_split(rect_, root_, right);

      auto new_root = get_node(new_root_idx);
      //assert(root_ < nodes_.size());
      new_root.rects_[0] = get_node(root_).rect_calc();
      //assert(right < nodes_.size());
      new_root.rects_[1] = get_node(right).rect_calc();
      new_root.children_[0] = root_;
      new_root.children_[1] = right;
      root_ = new_root_idx;
      new_root.count_ = 2;
      ++height_;
    }
  }

  /**
   * Inserts a new rectangle into a specified node or a node of its subtree
   * @param nr NOT USED
   * @param n_idx The id of the current node
   * @param insert_rect The rectangle to insert
   * @param data The data to insert
   * @param depth The depth of the current node
   * @param split True if this or a subsequent node had to be split
   */
  void node_insert(rect const& nr, node_idx_t const n_idx, rect const& insert_rect, DataType data, unsigned depth, bool& split) {
    auto& n0 = get_node(n_idx);
    if (n0.kind_ == kind::kLeaf) {
      if (n0.count_ == MaxItems) {
        split = true;
        return;
      }

      auto const index = static_cast<unsigned>(n0.count_);
      n0.rects_[index] = insert_rect;
      n0.data_[index] = std::move(data);
      n0.count_++;
      split = false;
      return;
    }

    // Choose a subtree for inserting the rectangle.
    auto const i = node_choose(n0, insert_rect, depth);
    node_insert(n0.rects_[i], n0.children_[i], insert_rect, data, depth + 1U, split);
    if (!split) {
      get_node(n_idx).rects_[i].expand(insert_rect);
      return;
    }

    // split the child node
    if (get_node(n_idx).count_ == MaxItems) {
      split = true;
      return;
    }
    node_idx_t right;
    node_split(get_node(n_idx).rects_[i], get_node(n_idx).children_[i], right);

    // n1 should be replaceable with n0
    auto& n1 = get_node(n_idx);
    n1.rects_[i] = get_node(n1.children_[i]).rect_calc();
    n1.rects_[n1.count_] = get_node(right).rect_calc();
    n1.children_[n1.count_] = right;
    n1.count_++;
    node_insert(nr, n_idx, insert_rect, std::move(data), depth, split);
  }

  void node_split(rect r, node_idx_t const n_idx, node_idx_t& right_out) {
    right_out = node_new(get_node(n_idx).kind_);
    auto& n = get_node(n_idx);
    auto& right = get_node(right_out);
    auto const axis = r.largest_axis();
    for (auto i = 0U; i != n.count_; ++i) {
      auto const min_dist = n.rects_[i].min_[axis] - r.min_[axis];
      auto const max_dist = r.max_[axis] - n.rects_[i].max_[axis];
      if (max_dist < min_dist) {
        // move to right
        assert(nodes_.contains(&n));
        assert(nodes_.contains(&right));
        n.move_rect_at_index_into(i, right);
        --i;
      }
    }
    // Make sure that both left and right nodes have at least
    // MINITEMS by moving datas into underflowed nodes.
    if (n.count_ < kMinItems) {
      // reverse sort by min axis
      right.sort_by_axis(axis, true, false);
      do {
        assert(nodes_.contains(&n));
        assert(nodes_.contains(&right));
        right.move_rect_at_index_into(right.count_ - 1, n);
      } while (n.count_ < kMinItems);
    } else if (right.count_ < kMinItems) {
      // reverse sort by max axis
      n.sort_by_axis(axis, true, true);
      do {
        assert(nodes_.contains(&n));
        assert(nodes_.contains(&right));
        n.move_rect_at_index_into(n.count_ - 1, right);
      } while (right.count_ < kMinItems);
    }
  }

  unsigned node_choose(node const& n, rect const& r, unsigned const depth) {
    auto const h = path_hint_[depth];
    if (h < n.count_) {
      if (n.rects_[h].contains(r)) {
        return h;
      }
    }

    // Take a quick look for the first node that contain the rect.
    for (auto i = 0U; i != n.count_; ++i) {
      if (n.rects_[i].contains(r)) {
        path_hint_[depth] = i;
        return i;
      }
    }

    // Fallback to using che "choose least enlargment" algorithm.
    auto const i = n.choose_least_enlargement(r);
    path_hint_[depth] = i;
    return i;
  }

  /**
   * Gets the node by id
   * @param node_id Node id
   * @return The corresponding node
   */
  node& get_node(node_idx_t const node_id) { return nodes_[node_id]; }

  /**
   * Creates a new node for the r_tree
   * @param node_kind Specifies if leaf or branch node
   * @return The id of the created node
   */
  node_idx_t node_new(kind const node_kind) {
    auto& new_node = nodes_.emplace_back();
    std::memset(&new_node, 0U, sizeof(node));
    new_node.kind_ = node_kind;
    return node_idx_t{nodes_.size() - 1U};
  }

  template <typename Fn>
  bool node_search(node const& n, rect const& r, Fn&& fn) {
    if (n.kind_ == kind::kLeaf) {
      for (auto i = 0U; i != n.count_; ++i) {
        if (n.rects_[i].intersects(r)) {
          if (!fn(n.rects_[i].min_, n.rects_[i].max_, n.data_[i])) {
            return false;
          }
        }
      }
      return true;
    }
    for (auto i = 0U; i != n.count_; ++i) {
      if (n.rects_[i].intersects(r)) {
        if (!node_search(get_node(n.children_[i]), r, fn)) {
          return false;
        }
      }
    }
    return true;
  }

  template <typename Fn>
  void search(coord_t const& min, coord_t const& max, Fn&& fn) {
    auto const r = rect{min, max};
    if (root_ != node_idx_t::invalid()) {
      node_search(get_node(root_), r, std::forward<Fn>(fn));
    }
  }

  rect rect_;
  node_idx_t root_{node_idx_t::invalid()};
  SizeType count_{0U};
  SizeType height_{0U};
  array<unsigned, 16U> path_hint_{};
  raw::vector_map<node_idx_t, node> nodes_;
};

}  // namespace cista