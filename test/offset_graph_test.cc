#include <queue>
#include <set>

#include "cista.h"

#include "doctest.h"

struct o_node;

using node_id_t = uint32_t;

struct o_edge {
  cista::offset_ptr<o_node> from_;
  cista::offset_ptr<o_node> to_;
};

struct o_node {
  void add_edge(o_edge* e) { edges_.emplace_back(e); }
  node_id_t id() const { return id_; }

  node_id_t id_{0};
  node_id_t fill_{0};
  cista::o_vector<cista::offset_ptr<o_edge>> edges_;
  cista::o_string name_;
};

struct o_graph {
  o_node* make_node(cista::o_string name) {
    return nodes_
        .emplace_back(cista::make_o_unique<o_node>(o_node{
            next_node_id_++, 0, cista::o_vector<cista::offset_ptr<o_edge>>{0u},
            std::move(name)}))
        .get();
  }

  o_edge* make_edge(node_id_t const from, node_id_t const to) {
    return edges_
        .emplace_back(cista::make_o_unique<o_edge>(
            o_edge{nodes_[from].get(), nodes_[to].get()}))
        .get();
  }

  cista::o_vector<cista::o_unique_ptr<o_node>> nodes_;
  cista::o_vector<cista::o_unique_ptr<o_edge>> edges_;
  node_id_t next_node_id_{0};
  node_id_t fill_{0};
};

inline std::set<o_node const*> o_bfs(o_node const* entry) {
  std::queue<o_node const*> q;
  std::set<o_node const*> visited;

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

TEST_CASE("graph serialization file") {
  {
    o_graph g;

    auto const n1 = g.make_node(cista::o_string{"NODE A"});
    auto const n2 = g.make_node(cista::o_string{"NODE B"});
    auto const n3 = g.make_node(cista::o_string{"NODE C"});

    auto const e1 = g.make_edge(n1->id(), n2->id());
    auto const e2 = g.make_edge(n2->id(), n3->id());
    auto const e3 = g.make_edge(n3->id(), n1->id());

    n1->add_edge(e1);
    n2->add_edge(e2);
    n3->add_edge(e3);

    cista::sfile f{"test.bin", "wb"};
    cista::serialize(f, g);
  }  // EOL graph

  auto b = cista::file("test.bin", "r").content();
  auto const g = reinterpret_cast<o_graph*>(b.begin());
  auto const visited = o_bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == cista::o_string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == cista::o_string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == cista::o_string{"NODE C"});
}

TEST_CASE("graph serialization buf") {
  cista::byte_buf buf;
  {
    o_graph g;

    auto const n1 = g.make_node(cista::o_string{"NODE A"});
    auto const n2 = g.make_node(cista::o_string{"NODE B"});
    auto const n3 = g.make_node(cista::o_string{"NODE C"});

    auto const e1 = g.make_edge(n1->id(), n2->id());
    auto const e2 = g.make_edge(n2->id(), n3->id());
    auto const e3 = g.make_edge(n3->id(), n1->id());

    n1->add_edge(e1);
    n2->add_edge(e2);
    n3->add_edge(e3);

    buf = cista::serialize(g);
  }  // EOL graph

  auto const g = reinterpret_cast<o_graph*>(&buf[0]);
  auto const visited = o_bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == cista::o_string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == cista::o_string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == cista::o_string{"NODE C"});
}
