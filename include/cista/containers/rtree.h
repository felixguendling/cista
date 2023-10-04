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
    NumType united_area(rect const& o) const noexcept {
      auto result = NumType{1};
      for (auto i = 0U; i != Dims; ++i) {
        result *= (std::max(max_[i], o.max_[i]) - std::min(min_[i], o.min_[i]));
      }
      return result;
    }

    NumType area() const noexcept {
      auto result = NumType{1};
      for (auto i = 0U; i != Dims; i++) {
        result *= (max_[i] - min_[i]);
      }
      return result;
    }

    bool contains(rect const& o) const noexcept {
      auto bits = 0U;
      for (auto i = 0U; i != Dims; i++) {
        bits |= (o.min_[i] < min_[i]);
        bits |= (o.max_[i] > max_[i]);
      }
      return bits == 0;
    }

    bool intersects(rect const& o) const noexcept {
      auto bits = 0;
      for (auto i = 0U; i != Dims; i++) {
        bits |= o.min_[i] > max_[i];
        bits |= o.max_[i] < min_[i];
      }
      return bits == 0;
    }

    bool onedge(rect const& o) const noexcept {
      for (auto i = 0U; i < Dims; i++) {
        if (feq(min_[i], o.min_[i]) || feq(max_[i], o.max_[i])) {
          return true;
        }
      }
      return false;
    }

    void expand(rect const& o) noexcept {
      for (auto i = 0U; i < Dims; i++) {
        min_[i] = std::min(min_[i], o.min_[i]);
        max_[i] = std::max(max_[i], o.max_[i]);
      }
    }

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
    void sort_by_axis(unsigned const axis, bool const rev, bool const max) {
      auto const by_index = max ? static_cast<unsigned>(Dims + axis) : axis;
      qsort(0U, count_, by_index, rev);
    }

    void qsort(unsigned s, unsigned e, unsigned index, bool rev) {
      struct rect4 {
        NumType all[Dims * 2U];
      };

      unsigned nrects = e - s;
      if (nrects < 2) {
        return;
      }
      unsigned left = 0U;
      unsigned right = nrects - 1;
      unsigned pivot = nrects / 2;
      swap(s + pivot, s + right);
      auto const rects = reinterpret_cast<rect4 const*>(&rects_[s]);
      if (!rev) {
        for (auto i = 0U; i != nrects; ++i) {
          if (rects[i].all[index] < rects[right].all[index]) {
            swap(s + i, s + left);
            ++left;
          }
        }
      } else {
        for (auto i = 0U; i != nrects; ++i) {
          if (rects[right].all[index] < rects[i].all[index]) {
            swap(s + i, s + left);
            ++left;
          }
        }
      }
      swap(s + left, s + right);
      qsort(s, s + left, index, rev);
      qsort(s + left + 1, e, index, rev);
    }

    unsigned choose_least_enlargement(rect const& ir) const {
      auto j = 0U;
      auto j_enlarge = kInfinity;
      for (auto i = 0U; i < count_; i++) {
        // calculate the enlarged area
        auto const uarea = rects_[i].united_area(ir);
        auto const area = rects_[i].area();
        auto const enlarge = uarea - area;
        if (enlarge < j_enlarge) {
          j = i;
          j_enlarge = enlarge;
        }
      }
      return j;
    }

    void move_rect_at_index_into(unsigned index, node& to) noexcept {
      to.rects_[to.count_] = rects_[index];
      rects_[index] = rects_[count_ - 1];
      if (kind_ == kind::kLeaf) {
        to.data_[to.count_] = data_[index];
        data_[index] = data_[count_ - 1];
      } else {
        to.children_[to.count_] = children_[index];
        children_[index] = children_[count_ - 1U];
      }
      --count_;
      ++to.count_;
    }

    void swap(unsigned i, unsigned j) noexcept {
      std::swap(rects_[i], rects_[j]);
      if (kind_ == kind::kLeaf) {
        std::swap(data_[i], data_[j]);
      } else {
        std::swap(children_[i], children_[j]);
      }
    }

    rect rect_calc() const noexcept {
      assert(count_ <= MaxItems);
      auto r = rects_[0U];
      for (auto i = 1U; i < count_; ++i) {
        r.expand(rects_[i]);
      }
      return r;
    }

    template <typename Ctx>
    friend void serialize(Ctx& c, node const* origin,
                          cista::offset_t const pos) {
      using Type = std::decay_t<decltype(*origin)>;
      c.write(pos + cista_member_offset(Type, count_),
              convert_endian<Ctx::MODE>(origin->count_));
      c.write(pos + cista_member_offset(Type, kind_),
              convert_endian<Ctx::MODE>(origin->kind_));
      c.write(pos + cista_member_offset(Type, rects_),
              convert_endian<Ctx::MODE>(origin->rects_));
      if (origin->kind_ == kind::kLeaf) {
        auto const size =
            static_cast<offset_t>(sizeof(DataType) * origin->data_.size());
        auto const data_start = pos + cista_member_offset(Type, data_);
        auto i = 0U;
        for (auto it = data_start; it != data_start + size;
             it += sizeof(DataType)) {
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

  void insert(coord_t const& min, coord_t const& max, DataType data) {
    auto const r = rect{min, max};
    while (true) {
      if (root_ == node_idx_t::invalid()) {
        root_ = node_new(kind::kLeaf);
        rect_ = r;
        height_ = 1U;
      }

      auto split = false;
      node_insert(rect_, root_, r, std::move(data), 0, split);
      if (!split) {
        rect_.expand(r);
        ++count_;
        return;
      }

      auto new_root_idx = node_new(kind::kBranch);

      auto right = node_idx_t::invalid();
      node_split(rect_, root_, right);

      auto new_root = get_node(new_root_idx);
      assert(root_ < nodes_.size());
      new_root.rects_[0] = get_node(root_).rect_calc();
      assert(right < nodes_.size());
      new_root.rects_[1] = get_node(right).rect_calc();
      new_root.children_[0] = root_;
      new_root.children_[1] = right;
      new_root.count_ = 2;
      root_ = new_root_idx;
      ++height_;
    }
  }

  void node_insert(rect const& nr, node_idx_t const n_idx, rect const& ir,
                   DataType data, unsigned depth, bool& split) {
    auto& n0 = get_node(n_idx);
    if (n0.kind_ == kind::kLeaf) {
      if (n0.count_ == MaxItems) {
        split = true;
        return;
      }

      auto const index = static_cast<unsigned>(n0.count_);
      n0.rects_[index] = ir;
      n0.data_[index] = std::move(data);
      n0.count_++;
      split = false;
      return;
    }

    // Choose a subtree for inserting the rectangle.
    auto const i = node_choose(n0, ir, depth);
    node_insert(n0.rects_[i], n0.children_[i], ir, data, depth + 1U, split);
    if (!split) {
      get_node(n_idx).rects_[i].expand(ir);
      return;
    }

    // split the child node
    if (get_node(n_idx).count_ == MaxItems) {
      split = true;
      return;
    }
    node_idx_t right;
    node_split(get_node(n_idx).rects_[i], get_node(n_idx).children_[i], right);

    auto& n1 = get_node(n_idx);
    n1.rects_[i] = get_node(n1.children_[i]).rect_calc();
    n1.rects_[n1.count_] = get_node(right).rect_calc();
    n1.children_[n1.count_] = right;
    n1.count_++;
    node_insert(nr, n_idx, ir, std::move(data), depth, split);
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

  node& get_node(node_idx_t const n) { return nodes_[n]; }

  node_idx_t node_new(kind const k) {
    auto& n = nodes_.emplace_back();
    std::memset(&n, 0U, sizeof(node));
    n.kind_ = k;
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