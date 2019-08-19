#include <queue>
#include <set>

#include "cista/serialization.h"
#include "cista/targets/file.h"

namespace data = cista::raw;

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

inline std::set<node const*> bfs(node const* entry) {
  if (entry == nullptr) {
    return {};
  }

  std::queue<node const*> q;
  std::set<node const*> visited;

  q.emplace(entry);

  while (!q.empty()) {
    auto const next = q.front();
    q.pop();

    if (visited.find(next) != end(visited)) {
      continue;
    }

    visited.emplace(next);

    for (auto const& e : next->edges_) {
      if (e != nullptr && e->to_ != nullptr) {
        q.emplace(e->to_);
      }
    }
  }

  return visited;
}

#if defined(GENERATE_SEED)
int main(int argc, char** argv) {
  if (argc != 2) {
    printf("usage: %s [seed file]\n", argv[0]);
    return 0;
  }

  graph g;

  auto const n1 = g.make_node(data::string{"NODE A"});
  auto const n2 = g.make_node(data::string{"NODE B"});
  auto const n3 = g.make_node(data::string{"NODE C"});

  auto const e1 = g.make_edge(n1->id(), n2->id());
  auto const e2 = g.make_edge(n2->id(), n3->id());
  auto const e3 = g.make_edge(n3->id(), n1->id());

  n1->add_edge(e1);
  n2->add_edge(e2);
  n3->add_edge(e3);

  cista::file f{argv[1], "w+"};
  cista::serialize(f, g);
}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) {
  try {
    cista::buffer b{reinterpret_cast<char const*>(data), size};
    auto const g = cista::deserialize<graph, cista::mode::DEEP_CHECK>(b);
    if (!g->nodes_.empty() && g->nodes_[0].get() != nullptr) {
      bfs(g->nodes_[0].get()).size();
    }
  } catch (std::exception const&) {
  }
  return 0;
}
#endif