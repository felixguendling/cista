// $CXX -O0 -g -Iinclude -std=c++17 -stdlib=libc++ -fsanitize=address,undefined,fuzzer -o fuzz_graph fuzz/fuzz_graph.cc && ./fuzz_graph

#include <queue>
#include <set>

#include "cista/mmap.h"
#include "cista/serialization.h"
#include "cista/targets/file.h"

namespace data = cista::offset;

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

void write_file(char const* path) {
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

  cista::file f{path, "w+"};
  cista::serialize(f, g);
}

void test(uint8_t const* data, size_t size) {
  try {
    auto const mutable_data = const_cast<uint8_t*>(data);
    auto const g = cista::deserialize<graphns::offset::graph, cista::mode::DEEP_CHECK>(
        mutable_data, mutable_data + size);
    if (!g->nodes_.empty() && g->nodes_[0].get() != nullptr) {
      bfs(g->nodes_[0].get()).size();
    }
  } catch (std::exception const&) {
  }
}

#if defined(READ_CRASH_FILE)
int main(int argc, char** argv) {
  if (argc != 3) {
    printf("usage: %s [write|read] [crash file]\n", argv[0]);
    return 0;
  }

  if (std::string_view{argv[1]} == "write") {
    write_file(argv[2]);
  } else if (std::string_view{argv[1]} == "read") {
  auto const buf = cista::file{argv[2], "r"}.content();
  test(&buf[0], buf.size());
  } else {
    printf("unknown mode \"%s\"\n", argv[1]);
  }
}
#else
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size) {
  test(data, size);
  return 0;
}
#endif