#pragma once

#include <cinttypes>
#include <cstring>

#include <functional>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#include "cista/aligned_alloc.h"
#include "cista/bit_counting.h"
#include "cista/decay.h"
#include "cista/endian/conversion.h"
#include "cista/offset_t.h"

namespace cista {

// This class is a generic hash-based container.
// It can be used e.g. as hash set or hash map.
//   - hash map: `T` = `std::pair<Key, Value>`, GetKey = `return entry.first;`
//   - hash set: `T` = `T`, GetKey = `return entry;` (identity)
//
// It is based on the idea of swiss tables:
// https://abseil.io/blog/20180927-swisstables
//
// Original implementation:
// https://github.com/abseil/abseil-cpp/blob/master/absl/container/internal/raw_hash_set.h
//
// Missing features of this implemenation compared to the original:
//   - SSE to speedup the lookup in the ctrl data structure
//   - sanitizer support (Sanitizer[Un]PoisonMemoryRegion)
//   - overloads (conveniance as well to reduce copying) in the interface
//   - allocator support
template <typename T, template <typename> typename Ptr,
          typename TemplateSizeType, typename GetKey, typename GetValue,
          typename Hash, typename Eq>
struct hash_storage {
  static constexpr auto const WIDTH = 8U;

  using entry_t = T;
  using difference_type = ptrdiff_t;
  using key_t =
      decay_t<decltype(std::declval<GetKey>().operator()(std::declval<T>()))>;
  using mapped_type =
      decay_t<decltype(std::declval<GetValue>().operator()(std::declval<T>()))>;
  using group_t = uint64_t;
  using h2_t = uint8_t;

  template <typename Key>
  size_t compute_hash(Key const& k) {
    if constexpr (std::is_same_v<decay_t<Key>, key_t>) {
      return Hash{}(k);
    } else {
      return Hash::template create<Key>()(k);
    }
  }

  enum ctrl_t : int8_t {
    EMPTY = -128,  // 10000000
    DELETED = -2,  // 11111110
    END = -1  // 11111111
  };

  struct find_info {
    size_t offset_, probe_length_;
  };

  struct probe_seq {
    probe_seq(size_t hash, size_t mask) : mask_{mask}, offset_{hash & mask_} {}
    size_t offset(size_t const i) const { return (offset_ + i) & mask_; }
    void next() {
      index_ += WIDTH;
      offset_ += index_;
      offset_ &= mask_;
    }
    size_t mask_, offset_, index_{0U};
  };

  struct bit_mask {
    static constexpr auto const SHIFT = 3U;

    explicit bit_mask(group_t mask) : mask_{mask} {}

    bit_mask& operator++() {
      mask_ &= (mask_ - 1);
      return *this;
    }

    size_t operator*() const { return trailing_zeros(); }

    explicit operator bool() const { return mask_ != 0U; }

    bit_mask begin() const { return *this; }
    bit_mask end() const { return bit_mask{0}; }

    size_t trailing_zeros() const {
      return ::cista::trailing_zeros(mask_) >> SHIFT;
    }

    size_t leading_zeros() const {
      constexpr int total_significant_bits = 8 << SHIFT;
      constexpr int extra_bits = sizeof(group_t) * 8 - total_significant_bits;
      return ::cista::leading_zeros(mask_ << extra_bits) >> SHIFT;
    }

    friend bool operator!=(bit_mask const& a, bit_mask const& b) {
      return a.mask_ != b.mask_;
    }

    group_t mask_;
  };

  struct group {
    static constexpr auto MSBS = 0x8080808080808080ULL;
    static constexpr auto LSBS = 0x0101010101010101ULL;
    static constexpr auto GAPS = 0x00FEFEFEFEFEFEFEULL;

    explicit group(ctrl_t const* pos) {
      std::memcpy(&ctrl_, pos, WIDTH);
#if defined(CISTA_BIG_ENDIAN)
      ctrl_ = endian_swap(ctrl_);
#endif
    }
    bit_mask match(h2_t const hash) const {
      auto const x = ctrl_ ^ (LSBS * hash);
      return bit_mask{(x - LSBS) & ~x & MSBS};
    }
    bit_mask match_empty() const {
      return bit_mask{(ctrl_ & (~ctrl_ << 6U)) & MSBS};
    }
    bit_mask match_empty_or_deleted() const {
      return bit_mask{(ctrl_ & (~ctrl_ << 7U)) & MSBS};
    }
    size_t count_leading_empty_or_deleted() const {
      return (trailing_zeros(((~ctrl_ & (ctrl_ >> 7U)) | GAPS) + 1U) + 7U) >>
             3U;
    }
    group_t ctrl_;
  };

  struct iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = hash_storage::entry_t;
    using reference = hash_storage::entry_t&;
    using pointer = hash_storage::entry_t*;
    using difference_type = ptrdiff_t;

    iterator() = default;

    reference operator*() const { return *entry_; }
    pointer operator->() const { return &operator*(); }
    iterator& operator++() {
      ++ctrl_;
      ++entry_;
      skip_empty_or_deleted();
      return *this;
    }
    iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend bool operator==(const iterator& a, const iterator& b) {
      return a.ctrl_ == b.ctrl_;
    }
    friend bool operator!=(const iterator& a, const iterator& b) {
      return !(a == b);
    }

    iterator(ctrl_t* ctrl) : ctrl_(ctrl) {}
    iterator(ctrl_t* ctrl, T* entry) : ctrl_(ctrl), entry_(entry) {}

    void skip_empty_or_deleted() {
      while (is_empty_or_deleted(*ctrl_)) {
        auto const shift = group{ctrl_}.count_leading_empty_or_deleted();
        ctrl_ += shift;
        entry_ += shift;
      }
    }

    ctrl_t* ctrl_ = nullptr;
    T* entry_ = nullptr;
  };

  struct const_iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = hash_storage::entry_t;
    using reference = hash_storage::entry_t const&;
    using pointer = hash_storage::entry_t const*;
    using difference_type = ptrdiff_t;

    const_iterator() = default;
    const_iterator(iterator i) : inner_(std::move(i)) {}

    reference operator*() const { return *inner_; }
    pointer operator->() const { return inner_.operator->(); }

    const_iterator& operator++() {
      ++inner_;
      return *this;
    }
    const_iterator operator++(int) { return inner_++; }

    friend bool operator==(const const_iterator& a, const const_iterator& b) {
      return a.inner_ == b.inner_;
    }
    friend bool operator!=(const const_iterator& a, const const_iterator& b) {
      return !(a == b);
    }

    const_iterator(ctrl_t const* ctrl, T const* entry)
        : inner_(const_cast<ctrl_t*>(ctrl), const_cast<T*>(entry)) {}

    iterator inner_;
  };

  static inline ctrl_t* empty_group() {
    alignas(16) static constexpr ctrl_t empty_group[] = {
        END,   EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
        EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};
    return const_cast<ctrl_t*>(empty_group);
  }

  static inline bool is_empty(ctrl_t const c) { return c == EMPTY; }
  static inline bool is_full(ctrl_t const c) { return c >= 0; }
  static inline bool is_deleted(ctrl_t const c) { return c == DELETED; }
  static inline bool is_empty_or_deleted(ctrl_t const c) { return c < END; }

  static inline size_t normalize_capacity(size_t const n) {
    return n == 0U ? 1 : ~size_t{} >> leading_zeros(n);
  }

  static inline size_t h1(size_t const hash) { return (hash >> 7) ^ 16777619; }

  static inline h2_t h2(size_t const hash) { return hash & 0x7F; }

  static inline size_t capacity_to_growth(size_t const capacity) {
    return (capacity == 7) ? 6 : capacity - (capacity / 8);
  }

  hash_storage() = default;

  hash_storage(std::initializer_list<T> init) {
    insert(init.begin(), init.end());
  }

  hash_storage(hash_storage&& other)
      : entries_{other.entries_},
        ctrl_{other.ctrl_},
        size_{other.size_},
        capacity_{other.capacity_},
        growth_left_{other.growth_left_},
        self_allocated_{other.self_allocated_} {
    other.entries_ = nullptr;
    other.ctrl_ = empty_group();
    other.size_ = 0U;
    other.capacity_ = 0U;
    other.growth_left_ = 0U;
    other.self_allocated_ = false;
  }

  hash_storage(hash_storage const& other) {
    resize(other.size());
    for (const auto& v : other) {
      auto const hash = compute_hash(GetKey()(v));
      auto target = find_first_non_full(hash);
      set_ctrl(target.offset_, h2(hash));
      new (entries_ + target.offset_) T{v};
    }
    size_ = other.size();
    growth_left_ -= other.size();
  }

  hash_storage& operator=(hash_storage&& other) {
    entries_ = other.entries_;
    ctrl_ = other.ctrl_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    growth_left_ = other.growth_left_;
    self_allocated_ = other.self_allocated_;
    other.entries_ = nullptr;
    other.ctrl_ = empty_group();
    other.size_ = 0U;
    other.capacity_ = 0U;
    other.growth_left_ = 0U;
    other.self_allocated_ = false;
    return *this;
  }

  hash_storage& operator=(hash_storage const& other) {
    resize(other.size());
    for (const auto& v : other) {
      auto const hash = compute_hash(GetKey()(v));
      auto target = find_first_non_full(hash);
      set_ctrl(target.offset, H2(hash));
      new (entries_ + target.offset_) T{v};
    }
    size_ = other.size();
    growth_left_ -= other.size();
    return *this;
  }

  ~hash_storage() { clear(); }

  void set_empty_key(key_t const&) {}
  void set_deleted_key(key_t const&) {}

  // --- operator[]
  template <typename Key>
  mapped_type& bracket_operator_impl(Key&& key) {
    auto const res = find_or_prepare_insert(key);
    if (res.second) {
      new (entries_ + res.first) T{static_cast<key_t>(key), mapped_type{}};
    }
    return GetValue{}(entries_[res.first]);
  }

  template <typename Key>
  mapped_type& operator[](Key&& key) {
    return bracket_operator_impl(std::forward<Key>(key));
  }

  mapped_type& operator[](key_t const& key) {
    return bracket_operator_impl(key);
  }

  // --- at()
  template <typename Key>
  mapped_type& at_impl(Key&& key) {
    if (auto it = find(key); it != end()) {
      return GetValue{}(*it);
    } else {
      throw std::out_of_range{"hash_storage::at() key not found"};
    }
  }

  mapped_type& at(key_t const& key) { return at_impl(key); }

  mapped_type const& at(key_t const& key) const {
    return const_cast<hash_storage*>(this)->at(key);
  }

  template <typename Key>
  mapped_type& at(Key&& key) {
    return at_impl(std::forward<Key>(key));
  }

  template <typename Key>
  mapped_type const& at(Key&& key) const {
    return const_cast<hash_storage*>(this)->at(std::forward<Key>(key));
  }

  // --- find()
  template <typename Key>
  iterator find_impl(Key&& key) {
    auto const hash = compute_hash(key);
    for (auto seq = probe_seq{h1(hash), capacity_}; true; seq.next()) {
      group g{ctrl_ + seq.offset_};
      for (auto const i : g.match(h2(hash))) {
        if (Eq{}(GetKey()(entries_[seq.offset(i)]), key)) {
          return iterator_at(seq.offset(i));
        }
      }
      if (g.match_empty()) {
        return end();
      }
    }
  }

  template <typename Key>
  const_iterator find(Key&& key) const {
    return const_cast<hash_storage*>(this)->find_impl(std::forward<Key>(key));
  }

  template <typename Key>
  iterator find(Key&& key) {
    return find_impl(std::forward<Key>(key));
  }

  const_iterator find(key_t const& key) const {
    return const_cast<hash_storage*>(this)->find_impl(key);
  }

  iterator find(key_t const& key) { return find_impl(key); }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      emplace(*first);
    }
  }

  // --- erase()
  template <typename Key>
  size_t erase_impl(Key&& key) {
    auto it = find(std::forward<Key>(key));
    if (it == end()) {
      return 0;
    }
    erase(it);
    return 1;
  }

  size_t erase(key_t const& k) { return erase_impl(k); }

  template <typename Key>
  size_t erase(Key&& key) {
    return erase_impl(std::forward<Key>(key));
  }

  void erase(iterator const it) {
    it.entry_->~T();
    erase_meta_only(it);
  }

  std::pair<iterator, bool> insert(T const& entry) { return emplace(entry); }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    auto entry = T{std::forward<Args>(args)...};
    auto res = find_or_prepare_insert(GetKey()(entry));
    if (res.second) {
      new (entries_ + res.first) T{std::move(entry)};
    }
    return {iterator_at(res.first), res.second};
  }

  iterator begin() {
    auto it = iterator_at(0);
    if (ctrl_ != nullptr) {
      it.skip_empty_or_deleted();
    }
    return it;
  }
  iterator end() { return {ctrl_ + capacity_}; }

  const_iterator begin() const {
    return const_cast<hash_storage*>(this)->begin();
  }
  const_iterator end() const { return const_cast<hash_storage*>(this)->end(); }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

  friend iterator begin(hash_storage& h) { return h.begin(); }
  friend const_iterator begin(hash_storage const& h) { return h.begin(); }
  friend const_iterator cbegin(hash_storage const& h) { return h.begin(); }
  friend iterator end(hash_storage& h) { return h.end(); }
  friend const_iterator end(hash_storage const& h) { return h.end(); }
  friend const_iterator cend(hash_storage const& h) { return h.begin(); }

  bool empty() const { return size() == 0U; }
  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }
  size_t max_size() const { return std::numeric_limits<size_t>::max(); }

  bool is_free(int index) {
    auto const index_before = (index - WIDTH) & capacity_;

    auto const empty_after = group{ctrl_ + index}.match_empty();
    auto const empty_before = group{ctrl_ + index_before}.match_empty();

    return empty_before && empty_after &&
           (empty_after.trailing_zeros() + empty_before.leading_zeros()) <
               WIDTH;
  }

  inline bool was_never_full(size_t const index) {
    auto const index_before = (index - WIDTH) & capacity_;

    auto const empty_after = group{ctrl_ + index}.match_empty();
    auto const empty_before = group{ctrl_ + index_before}.match_empty();

    return empty_before && empty_after &&
           (empty_after.trailing_zeros() + empty_before.leading_zeros()) <
               WIDTH;
  }

  void erase_meta_only(const_iterator it) {
    --size_;
    auto const index = static_cast<size_t>(it.inner_.ctrl_ - ctrl_);
    auto const wnf = was_never_full(index);
    set_ctrl(index, static_cast<h2_t>(wnf ? EMPTY : DELETED));
    growth_left_ += wnf;
  }

  void clear() {
    if (capacity_ == 0U) {
      return;
    }

    for (auto i = size_t{0U}; i != capacity_; ++i) {
      if (is_full(ctrl_[i])) {
        entries_[i].~T();
      }
    }

    if (self_allocated_) {
      CISTA_ALIGNED_FREE(entries_);
    }
    entries_ = nullptr;
    ctrl_ = empty_group();
    size_ = 0U;
    capacity_ = 0U;
    growth_left_ = 0U;
  }

  template <typename Key>
  std::pair<size_t, bool> find_or_prepare_insert(Key&& key) {
    auto const hash = compute_hash(key);
    for (auto seq = probe_seq{h1(hash), capacity_}; true; seq.next()) {
      group g{ctrl_ + seq.offset_};
      for (auto const i : g.match(h2(hash))) {
        if (Eq{}(GetKey()(entries_[seq.offset(i)]), key)) {
          return {seq.offset(i), false};
        }
      }
      if (g.match_empty()) {
        break;
      }
    }
    return {prepare_insert(hash), true};
  }

  find_info find_first_non_full(size_t const hash) const {
    for (auto seq = probe_seq{h1(hash), capacity_}; true; seq.next()) {
      if (auto const mask = group{ctrl_ + seq.offset_}.match_empty_or_deleted();
          mask) {
        return {seq.offset(*mask), seq.index_};
      }
    }
  }

  size_t prepare_insert(size_t const hash) {
    auto target = find_first_non_full(hash);
    if (growth_left_ == 0U && !is_deleted(ctrl_[target.offset_])) {
      rehash_and_grow_if_necessary();
      target = find_first_non_full(hash);
    }
    ++size_;
    growth_left_ -= (is_empty(ctrl_[target.offset_]) ? 1 : 0);
    set_ctrl(target.offset_, h2(hash));
    return target.offset_;
  }

  void set_ctrl(size_t const i, h2_t const c) {
    ctrl_[i] = static_cast<ctrl_t>(c);
    ctrl_[((i - WIDTH) & capacity_) + 1 + ((WIDTH - 1) & capacity_)] =
        static_cast<ctrl_t>(c);
  }

  void rehash_and_grow_if_necessary() {
    if (capacity_ == 0) {
      resize(1);
    } else {
      resize(capacity_ * 2 + 1);
    }
  }

  void reset_growth_left() {
    growth_left_ = capacity_to_growth(capacity_) - size_;
  }

  void reset_ctrl() {
    std::memset(ctrl_, EMPTY, capacity_ + WIDTH + 1U);
    ctrl_[capacity_] = END;
  }

  void initialize_entries() {
    self_allocated_ = true;
    auto const size =
        capacity_ * sizeof(T) + (capacity_ + 1 + WIDTH) * sizeof(ctrl_t);
    entries_ = reinterpret_cast<T*>(CISTA_ALIGNED_ALLOC(sizeof(T), size));
    ctrl_ = reinterpret_cast<ctrl_t*>(reinterpret_cast<uint8_t*>(entries_) +
                                      capacity_ * sizeof(T));
    reset_ctrl();
    reset_growth_left();
  }

  void resize(size_t const new_capacity) {
    auto const old_ctrl = ctrl_;
    auto const old_entries = entries_;
    auto const old_capacity = capacity_;

    capacity_ = new_capacity;
    initialize_entries();

    for (auto i = size_t{0U}; i != old_capacity; ++i) {
      if (is_full(old_ctrl[i])) {
        auto const hash = compute_hash(GetKey()(old_entries[i]));
        auto const target = find_first_non_full(hash);
        auto const new_index = target.offset_;
        set_ctrl(new_index, h2(hash));
        new (entries_ + new_index) T{std::move(old_entries[i])};
      }
    }

    if (old_capacity != 0U && self_allocated_) {
      CISTA_ALIGNED_FREE(old_entries);
    }
  }

  iterator iterator_at(size_t const i) { return {ctrl_ + i, entries_ + i}; }
  const_iterator iterator_at(size_t i) const {
    return {ctrl_ + i, entries_ + i};
  }

  Ptr<T> entries_{nullptr};
  Ptr<ctrl_t> ctrl_{empty_group()};
  size_t size_{0U}, capacity_{0U}, growth_left_{0U};
  bool self_allocated_{false};
};

}  // namespace cista
