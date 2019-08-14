//  ~/Programs/clang+llvm-8.0.0-x86_64-apple-darwin/bin/clang++ -O0 -g -Iinclude -std=c++17 -stdlib=libc++ -fsanitize=address,fuzzer fuzz/fuzz_graph.cc && ./a.ou

#include <queue>
#include <set>

#include "cista/mmap.h"
#include "cista/serialization.h"
#include "cista/targets/file.h"

namespace data = cista::offset;

constexpr auto const CHECKSUM_INTEGRITY_AND_VERSION =
    sizeof(void*) == 4 ? 1677829801727797916ULL : 6773875736123884735ULL;
constexpr auto const CHECKSUM_BIG_ENDIAN =
    sizeof(void*) == 4 ? 13420010985482148984ULL : 15391991077970203927ULL;

namespace graphns::offset {

struct node;

using node_id_t = uint32_t;

struct edge {
  data::ptr<node> from_;
  data::ptr<node> to_;
};

struct node {
  void add_edge(edge* e) { edges_.emplace_back(e); }
  node_id_t id() const { return id_; }

  node_id_t id_{0};
  node_id_t fill_{0};
  data::vector<data::ptr<edge>> edges_;
  data::string name_;
};

struct graph {
  node* make_node(data::string name) {
    return nodes_
        .emplace_back(data::make_unique<node>(
            node{next_node_id_++, 0, data::vector<data::ptr<edge>>{0u},
                 std::move(name)}))
        .get();
  }

  edge* make_edge(node_id_t const from, node_id_t const to) {
    return edges_
        .emplace_back(
            data::make_unique<edge>(edge{nodes_[from].get(), nodes_[to].get()}))
        .get();
  }

  data::vector<data::unique_ptr<node>> nodes_;
  data::vector<data::unique_ptr<edge>> edges_;
  node_id_t next_node_id_{0};
  node_id_t fill_{0};
};

}  // namespace graphns::offset

using namespace graphns::offset;

inline std::set<node const*> bfs(node const* entry) {
  if (entry == nullptr) {
    return {};
  }

  std::queue<node const*> q;
  std::set<node const*> visited;

  // printf("emplacing %p\n", entry);
  q.emplace(entry);

  while (!q.empty()) {
    auto const next = q.front();
    q.pop();

    if (visited.find(next) != end(visited)) {
      continue;
    }

    visited.emplace(next);

    // printf("edges of %p (next->edges_.el_=%p):\n", next,
    //        next->edges_.el_.get());
    for (auto const& e : next->edges_) {
      // printf("  edge: %p\n", e.get());
      if (e != nullptr && e->to_ != nullptr) {
        // printf("  emplacing %p\n", static_cast<node const*>(e->to_));
        q.emplace(e->to_);
      }
    }
  }

  return visited;
}

void test(uint8_t const* data, size_t size) {
  try {
    auto const mutable_data = const_cast<uint8_t*>(data);
    // printf("range: %p - %p\n", data, data + size);
    auto const g = cista::deserialize<graphns::offset::graph, cista::mode::DEEP_CHECK>(
        mutable_data, mutable_data + size);
    if (g->nodes_.size() >= 2U) {
      printf("graph with %u nodes\n", g->nodes_.size());
      auto const bfs_nodes = bfs(g->nodes_[0].get()).size();
      if (bfs_nodes >= 2) {
        printf("bfs nodes = %zu\n", bfs_nodes);
      }
    }
  } catch (std::exception const&) {
  }
}

#if defined(READ_CRASH_FILE)
int main(int argc, char** argv) {
  if (argc != 2) {
    printf("usage: %s [crash file]\n", argv[0]);
    return 0;
  }

  auto const buf = cista::file{argv[1], "r"}.content();
  test(&buf[0], buf.size());
}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size) {
  test(data, size);
  return 0;
}
#endif