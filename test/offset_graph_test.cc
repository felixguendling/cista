#include <queue>
#include <set>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/mmap.h"
#include "cista/serialization.h"
#endif

namespace data = cista::offset;

constexpr auto const CHECKSUM_INTEGRITY_AND_VERSION =
    sizeof(void*) == 4 ? 13555853665375988614ULL : 10074751584048131577ULL;
constexpr auto const CHECKSUM_BIG_ENDIAN =
    sizeof(void*) == 4 ? 13950808542881900738ULL : 14201103843034132153ULL;

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
            node{next_node_id_++, 0, data::vector<data::ptr<edge>>(0u),
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

TEST_CASE("graph offset serialize file") {
  constexpr auto const FILENAME = "offset_graph.bin";
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  std::remove(FILENAME);

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

    cista::file f{FILENAME, "w+"};
    cista::serialize<MODE>(f, g);

    CHECK(f.checksum() == CHECKSUM_INTEGRITY_AND_VERSION);
  }  // EOL graph

  {
    cista::file f{FILENAME, "r"};
    CHECK(f.checksum() == CHECKSUM_INTEGRITY_AND_VERSION);
  }

  auto b = cista::file(FILENAME, "r").content();
  CHECK(cista::hash(b) == CHECKSUM_INTEGRITY_AND_VERSION);

  auto const g = cista::deserialize<graph, MODE>(b);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}

TEST_CASE("graph offset serialize buf") {
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

    CHECK(b.checksum() == CHECKSUM_INTEGRITY_AND_VERSION);

    buf = std::move(b.buf_);
  }  // EOL graph

  CHECK(cista::hash(buf) == CHECKSUM_INTEGRITY_AND_VERSION);

  auto const g = cista::deserialize<graph, MODE>(buf);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}

TEST_CASE("graph offset serialize mmap file") {
  constexpr auto const FILENAME = "offset_graph_mmap.bin";
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  std::remove(FILENAME);

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

    cista::buf<cista::mmap> mmap{cista::mmap{FILENAME}};
    mmap.buf_.reserve(512);
    cista::serialize<MODE>(mmap, g);
    CHECK(mmap.checksum() == CHECKSUM_INTEGRITY_AND_VERSION);
  }  // EOL graph

#if defined(CISTA_LITTLE_ENDIAN)
  auto b = cista::mmap(FILENAME, cista::mmap::protection::READ);
#else
  auto b = cista::file(FILENAME, "r").content();
#endif
  auto const g = cista::deserialize<graph, MODE>(b);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}

TEST_CASE("graph offset serialize endian test") {
  constexpr auto const FILENAME = "offset_graph_big_endian.bin";
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION |
      cista::mode::SERIALIZE_BIG_ENDIAN | cista::mode::DEEP_CHECK;

  std::remove(FILENAME);

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

    cista::buf<cista::mmap> mmap{cista::mmap{FILENAME}};
    mmap.buf_.reserve(512);
    cista::serialize<MODE>(mmap, g);
    CHECK(mmap.checksum() == CHECKSUM_BIG_ENDIAN);
  }  // EOL graph

  auto b = cista::file(FILENAME, "r").content();
  auto const g = cista::deserialize<graph, MODE>(b);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}