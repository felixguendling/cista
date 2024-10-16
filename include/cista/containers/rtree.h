#pragma once

#include <cinttypes>
#include <fstream>

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

template <typename DataType, uint32_t Dims = 2U, typename NumType = float, uint32_t MaxItems = 64U, typename SizeType = std::uint32_t, template <typename, typename...> typename VectorType = offset::vector_map>
struct rtree {
  static constexpr auto const kInfinity = std::numeric_limits<NumType>::max();

  // used for splits
  static constexpr auto const kMinItemsPercentage = 10U;
  static constexpr auto const kMinItems = ((MaxItems * kMinItemsPercentage) / 100) + 1;

  enum class kind : std::uint8_t { kLeaf, kBranch, kEndFreeList };

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
    uint32_t largest_axis() const noexcept {
      auto axis = 0U;
      auto nlength = max_[0] - min_[0];
      for (uint32_t i = 1; i != Dims; ++i) {
        auto const length = max_[i] - min_[i];
        if (length > nlength) {
          nlength = length;
          axis = i;
        }
      }
      return axis;
    }

    /**
     * Checks if this and another rectangle are the same
     * @param other_rect The rectangle to compare to
     * @return True if they are equal
     */
    bool equals(rect const& other_rect) {
       if (!coord_t_equal(min_, other_rect.min_) ||
          !coord_t_equal(max_, other_rect.max_)) {
         return false;
       }
       return true;
    }

    bool equals_bin(rect const& other_rect) {
      for (size_t i = 0; i < Dims; ++i) {
        if (min_[i] != other_rect.min_[i] ||
            max_[i] != other_rect.max_[i]) {
          return false;
        }
      }
      return true;
    }

    /**
     * Checks if two coordinates are the same
     * @param coord_1 First coordinate
     * @param coord_2 Second coordinate
     * @return True if they are the same
     */
    static bool coord_t_equal(coord_t const& coord_1, coord_t const& coord_2) {
      for (size_t i = 0; i < Dims; ++i) {
        if (!feq(coord_1[i], coord_2[i])){
          return false;
        }
      }
      return true;
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
    void sort_by_axis(uint32_t const axis, bool const rev, bool const max) {
      auto const by_index = max ? static_cast<uint32_t>(Dims + axis) : axis;
      qsort(0U, count_, by_index, rev);
    }

    /**
     * Quicksort implementation for sorting rectangles (and its attached data) inside of a node
     * @param start Start index of this partition
     * @param end End index + 1 of this partition
     * @param index Index for choosing the sorting axis of the rectangles
     * @param rev Decides if sorted ascending or descending
     */
    void qsort(uint32_t start, uint32_t end, uint32_t index, bool rev) {
      // Simplification for the struct rect
      struct rect4 {
        NumType all[Dims * 2U];
      };

      uint32_t nrects = end - start;
      if (nrects < 2) {
        return;
      }
      uint32_t left = 0U;
      uint32_t right = nrects - 1;
      uint32_t pivot = nrects / 2;
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
    uint32_t choose_least_enlargement(rect const& insert_rect) const {
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
    void move_rect_at_index_into(uint32_t index, node& into) noexcept {
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
    void swap(uint32_t i, uint32_t j) noexcept {
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

    using node_vector_t = array<node_idx_t, MaxItems>;
    using data_vector_t = array<DataType, MaxItems>;

    uint32_t count_{0U};
    kind kind_;
    array<rect, MaxItems> rects_;

    union {
      node_vector_t children_;
      data_vector_t data_;
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

      auto new_root_idx = node_new(kind::kBranch);

      auto right = node_idx_t::invalid();

      node_split(rect_, root_, right);

      auto& new_root = get_node(new_root_idx);
      new_root.rects_[0] = get_node(root_).rect_calc();
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
  void node_insert(rect const& nr, node_idx_t const n_idx, rect const& insert_rect, DataType data, uint32_t depth, bool& split) {
    auto& current_node = get_node(n_idx);
    if (current_node.kind_ == kind::kLeaf) {
      if (current_node.count_ == MaxItems) {
        split = true;
        return;
      }

      auto const index = static_cast<uint32_t>(current_node.count_);
      current_node.rects_[index] = insert_rect;
      current_node.data_[index] = std::move(data);
      current_node.count_++;
      split = false;
      return;
    }

    // Choose a subtree for inserting the rectangle.
    auto const i = node_choose(current_node, insert_rect, depth);
    node_insert(current_node.rects_[i], current_node.children_[i], insert_rect, data, depth + 1U, split);
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

    // n1 should be replaceable with current_node
    auto& n1 = get_node(n_idx);
    n1.rects_[i] = get_node(n1.children_[i]).rect_calc();
    n1.rects_[n1.count_] = get_node(right).rect_calc();
    n1.children_[n1.count_] = right;
    n1.count_++;
    node_insert(nr, n_idx, insert_rect, std::move(data), depth, split);
  }

  /**
   * Function for splitting an overflowing node into multiple nodes.
   * @param node_rect The bounding rectangle of the overflowing node
   * @param n_idx The node id of the overflowing node
   * @param right_out The newly added node
   */
  void node_split(rect node_rect, node_idx_t const n_idx, node_idx_t& right_out) {
    auto const axis = node_rect.largest_axis();
    right_out = node_new(get_node(n_idx).kind_);
    auto& old_node = get_node(n_idx);
    auto& right = get_node(right_out);
    for (auto i = 0U; i < old_node.count_; ++i) {
      auto const min_dist = old_node.rects_[i].min_[axis] - node_rect.min_[axis];
      auto const max_dist = node_rect.max_[axis] - old_node.rects_[i].max_[axis];
      if (max_dist < min_dist) {
        // move to right
        old_node.move_rect_at_index_into(i, right);
        --i;
      }
    }

    // MINITEMS by moving datas into underflowed nodes.
    if (old_node.count_ < kMinItems) {
      // reverse sort by min axis
      right.sort_by_axis(axis, true, false);
      do {
        right.move_rect_at_index_into(right.count_ - 1, old_node);
      } while (old_node.count_ < kMinItems);
    } else if (right.count_ < kMinItems) {
      // reverse sort by max axis
      old_node.sort_by_axis(axis, true, true);
      do {
        old_node.move_rect_at_index_into(old_node.count_ - 1, right);
      } while (right.count_ < kMinItems);
    }
    if (old_node.kind_ == kind::kBranch) {
      old_node.sort_by_axis(0, true, false);
      right.sort_by_axis(0,true, false);
    }
  }

  /**
   * Chooses the node to insert the rectangle and its data into
   * @param search_node The node to search in
   * @param search_rect The rectangle to search for
   * @param depth The current node depth of the search
   * @return A fitting node to place the rectangle into
   */
  uint32_t node_choose(node const& search_node, rect const& search_rect, uint32_t const depth) {
    auto const h = path_hint_[depth];
    if (h < search_node.count_) {
      if (search_node.rects_[h].contains(search_rect)) {
        return h;
      }
    }

    // Take a quick look for the first node that contain the rect.
    for (auto i = 0U; i != search_node.count_; ++i) {
      if (search_node.rects_[i].contains(search_rect)) {
        path_hint_[depth] = i;
        return i;
      }
    }

    // Fallback to using che "choose least enlargment" algorithm.
    auto const i = search_node.choose_least_enlargement(search_rect);
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
    if (free_list_ == node_idx_t::invalid()) {
      auto& new_node = nodes_.emplace_back();
      std::memset(&new_node, 0U, sizeof(node));
      new_node.kind_ = node_kind;
      return node_idx_t{nodes_.size() - 1U};
    } else {
      node_idx_t recycled_node_id = free_list_;
      if (get_node(recycled_node_id).kind_ == kind::kEndFreeList) {
        free_list_ = node_idx_t::invalid();
      } else {
        free_list_ = node_idx_t(get_node(recycled_node_id).count_);
        //free_list = reinterpret_cast<node_idx_t>(get_node(recycled_node_id).count_);
      }
      get_node(recycled_node_id).kind_ = node_kind;
      get_node(recycled_node_id).count_ = 0U;
      return recycled_node_id;
    }
  }

  /**
   * Traverses the rtree and executes a function for each intersection
   * @tparam Fn The function type of the handed function
   * @param current_node The current node to search in
   * @param search_rect The rectangle to search for intersects
   * @param fn The function to execute
   * @return True if the function fn returns true for every found intersection hit
   */
  template <typename Fn>
  bool node_search(node const& current_node, rect const& search_rect, Fn&& fn) {
    if (current_node.kind_ == kind::kLeaf) {
      for (auto i = 0U; i != current_node.count_; ++i) {
        if (current_node.rects_[i].intersects(search_rect)) {
          if (!fn(current_node.rects_[i].min_, current_node.rects_[i].max_,
                  current_node.data_[i])) {
            return false;
          }
        }
      }
      return true;
    }
    for (auto i = 0U; i != current_node.count_; ++i) {
      if (current_node.rects_[i].intersects(search_rect)) {
        if (!node_search(get_node(current_node.children_[i]), search_rect, fn)) {
          return false;
        }
      }
    }
    return true;
  }

  /**
   * Searches the rtree for rectangles intersecting (min, max) and executes a function for each hit
   * @tparam Fn The function type of the handed function
   * @param min The lower left vertex of the rectangle
   * @param max The upper right vertex of the rectangle
   * @param fn The function to execute
   */
  template <typename Fn>
  void search(coord_t const& min, coord_t const& max, Fn&& fn) {
    auto const r = rect{min, max};
    if (root_ != node_idx_t::invalid()) {
      node_search(get_node(root_), r, std::forward<Fn>(fn));
    }
  }

  /**
   * Deletes a node and joins underflowing nodes to preserve rtree rules
   * @param node_rect The bounding rectangle of the current node
   * @param delete_node_id The current node id to check for deletion
   * @param input_rect The rectangle to search for
   * @param item The data to compare to the found deletion entry
   * @param depth The current tree depth
   * @param removed Bool reference to indicate if a node was deleted
   * @param shrunk Bool reference to indicate if the tree has shrunk
   * @param compare A comparator to compare data
   * @param udata User defined data used in comparator
   */
  template <typename Fn>
  void node_delete(rect& node_rect, node_idx_t delete_node_id, rect& input_rect,
                   uint32_t const depth, bool& removed, bool& shrunk,
                   Fn&& fn) {
    removed = false;
    shrunk = false;
    auto& delete_node = get_node(delete_node_id);
    if (delete_node.kind_ == kind::kLeaf) {
      for (size_t i = 0; i < delete_node.count_; ++i) {
        // Skip to next loop iteration if function evaluate to false
        if (!fn(delete_node.rects_[i].min_, delete_node.rects_[i].max_, delete_node.data_[i])) {
          continue;
        }

        // Found the target item to delete.
        if (true) {
          delete_node.data_[i].~DataType();
        }
        delete_node.rects_[i] = delete_node.rects_[delete_node.count_ - 1];
        delete_node.data_[i] = delete_node.data_[delete_node.count_ - 1];
        delete_node.count_--;
        if (input_rect.onedge(node_rect)) {
          // The item rect was on the edge of the node rect.
          // We need to recalculate the node rect.
          node_rect = delete_node.rect_calc();
          // Notify the caller that we shrunk the rect.
          shrunk = true;
        }
        removed = true;
        return;
      }
      return;
    }

    int h = 0;
    h = path_hint_[depth];
    rect crect;
    if (h < delete_node.count_) {
      if (delete_node.rects_[h].contains(input_rect)) {
        node_delete(delete_node.rects_[h], delete_node.children_[h], input_rect, depth + 1, removed, shrunk, fn);
        if (removed) {
          goto removed;
        }

      }
    }
    h = 0;
    for (; h < delete_node.count_; h++) {
      if (!delete_node.rects_[h].contains(input_rect)) {
        continue;
      }
      crect = delete_node.rects_[h];
      node_delete(delete_node.rects_[h], delete_node.children_[h], input_rect, depth + 1, removed, shrunk, fn);
      if (!removed) {
        continue;
      }
    removed:
      if (get_node(delete_node.children_[h]).count_ == 0) {
        // underflow
        // free the node, planned with a free_list: delete_node.children_[h]
        add_to_free_list(delete_node.children_[h]);
        delete_node.rects_[h] = delete_node.rects_[delete_node.count_ - 1];
        delete_node.children_[h] = delete_node.children_[delete_node.count_ - 1];
        delete_node.count_--;
        node_rect = delete_node.rect_calc();
        shrunk = true;
        return;
      }
      path_hint_[depth] = h;
      if (shrunk) {
        shrunk = !delete_node.rects_[h].equals(crect);
        if (shrunk) {
          node_rect = delete_node.rect_calc();
        }
      }
      return;
    }
  }

  /**
   * Deletes an element from the rtree
   * @param min The lower left vertex of the rectangle
   * @param max The upper right vertex of the rectangle
   * @param item The data to insert
   * @param compare A comparator for comparing data items
   * @param udata User defined data used in comparator
   */
  template <typename Fn>
  void delete_0(coord_t const& min, coord_t const& max, Fn&& fn) {
    rect input_rect = {min,max};

    if (root_ == node_idx_t::invalid()) {
      return;
    }
    auto removed = false;
    auto shrunk = false;
    node_delete(rect_, root_, input_rect, 0, removed, shrunk, std::forward<Fn>(fn));
    if (!removed) {
      return;
    }
    count_--;
    if (count_ == 0) {
      // free the root node, planned with a free_list:
      add_to_free_list(root_);
      root_ = node_idx_t::invalid();
      height_ = 0;
    } else {
      while (get_node(root_).kind_ == kind::kBranch && count_ == 1) {
        auto& prev = get_node(root_);
        auto prev_id = root_;
        root_ = get_node(root_).children_[0];
        prev.count_ = 0;
        // free the prev node, planned with a free_list:
        add_to_free_list(prev_id);
        height_--;
      }
      if (shrunk) {
        rect_ = get_node(root_).rect_calc();
      }
    }
  }

  /**
   * Deletes an element from the rtree. Proxy function without function parameter for delete_0
   * @param min The lower left vertex of the rectangle
   * @param max The upper right vertex of the rectangle
   * @param data The data to delete
   */
  void delete_element(coord_t const& min, coord_t const& max, DataType data) {
    using r_tree_instance = cista::rtree<DataType, Dims, NumType, MaxItems, SizeType, VectorType>;
    return delete_0(min, max, [min, max, &data](r_tree_instance::coord_t const& min_temp, r_tree_instance::coord_t const& max_temp, DataType search_data){
      if (r_tree_instance::rect::coord_t_equal(min, min_temp) && r_tree_instance::rect::coord_t_equal(max, max_temp) && data == search_data) {
        return true;
      } else {
        return false;
      }
    });
  }

  /**
   * Deletes an element from the rtree. Proxy function with function parameter for delete_0
   * Note: Function has to return true for the entry to delete!
   * @param min The lower left vertex of the rectangle
   * @param max The upper right vertex of the rectangle
   * @param data The data to insert
   * @param fn Function for selecting data entries
   */
  template <typename Fn>
  void delete_element_with_function(coord_t const& min, coord_t const& max, Fn&& fn) {
    return delete_0(min, max, std::forward<Fn>(fn));
  }

  /**
   * Adds a no longer used node to a list of nodes. The allocated space by the
   * nodes in this list will be used up first when ever a new node is created.
   * @param node_id The node id added to the list
   */
  void add_to_free_list(node_idx_t node_id) {
    if (free_list_ == node_idx_t::invalid()) {
      get_node(node_id).kind_ = kind::kEndFreeList;
    } else {
      get_node(node_id).count_ = to_idx(free_list_);
    }
    free_list_ = node_id;
  }

  /**
   * Writes all rtree parameters except nodes_ into the given output stream.
   * @param f The output stream to write to
   */
  void write_header_bin(std::ostream &f) {
    f.write((char*)&rect_, sizeof rect_);
    f.write((char*)&root_, sizeof root_);
    f.write((char*)&free_list_, sizeof free_list_);
    f.write((char*)&count_, sizeof count_);
    f.write((char*)&height_, sizeof height_);
    for (int i = 0; i < path_hint_.size(); ++i) {
      f.write((char*)&path_hint_[i], sizeof path_hint_[i]);
    }
  }

  /**
   * Reads all rtree parameters except nodes_ from a given input stream
   * @param f The input stream to read from
   */
  void read_header_bin(std::istream &f) {
    f.read((char*)&rect_, sizeof rect_);
    f.read((char*)&root_, sizeof root_);
    f.read((char*)&free_list_, sizeof free_list_);
    f.read((char*)&count_, sizeof count_);
    f.read((char*)&height_, sizeof height_);
    for (int i = 0; i < path_hint_.size(); ++i) {
      f.read((char*)&path_hint_[i], sizeof path_hint_[i]);
    }
  }

  rect rect_;
  node_idx_t root_{node_idx_t::invalid()};
  node_idx_t free_list_ = node_idx_t::invalid();
  SizeType count_{0U};
  SizeType height_{0U};
  array<uint32_t, 16U> path_hint_{};
  VectorType<node_idx_t, node> nodes_;
};

}  // namespace cista