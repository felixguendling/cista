#include <queue>
#include <set>

#include "doctest.h"

#include "cista.h"

namespace data = cista::raw;

namespace graphns::raw {

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

}  // namespace graphns::raw

using namespace graphns::raw;

namespace cista {
template <>
struct use_standard_hash<graphns::raw::node> : public std::true_type {};
}  // namespace cista

inline std::set<node const*> bfs(node const* entry) {
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
      q.emplace(e->to_);
    }
  }

  return visited;
}

TEST_CASE("graph raw serialize file") {
  constexpr auto const FILE_CHECKSUM = 914462787224036784ULL;
  constexpr auto const FILENAME = "raw_graph.bin";

  std::remove(FILENAME);

  {
    graph g;

    CHECK(8294030070925330150ULL == cista::type_hash(g));

    auto const n1 = g.make_node(data::string{"NODE A"});
    auto const n2 = g.make_node(data::string{"NODE B"});
    auto const n3 = g.make_node(data::string{"NODE C"});

    auto const e1 = g.make_edge(n1->id(), n2->id());
    auto const e2 = g.make_edge(n2->id(), n3->id());
    auto const e3 = g.make_edge(n3->id(), n1->id());

    n1->add_edge(e1);
    n2->add_edge(e2);
    n3->add_edge(e3);

    cista::file f{FILENAME, "w+"};
    cista::serialize(f, g);

    CHECK(f.checksum() == FILE_CHECKSUM);
  }  // EOL graph

  auto b = cista::file(FILENAME, "r").content();
  CHECK(cista::hash(b) == FILE_CHECKSUM);

  auto const g = data::deserialize<graph>(b);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}

TEST_CASE("graph raw serialize buf") {
  constexpr auto const EXPECTED_BUF_CHECKSUM = 2984678943337296086ULL;
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  cista::byte_buf buf;
  {
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

    cista::buf b;
    cista::serialize<MODE>(b, g);

    CHECK(b.checksum() == EXPECTED_BUF_CHECKSUM);

    buf = std::move(b.buf_);
  }  // EOL graph

  CHECK(cista::hash(buf) == EXPECTED_BUF_CHECKSUM);

  auto const g = data::deserialize<graph, MODE>(buf);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}