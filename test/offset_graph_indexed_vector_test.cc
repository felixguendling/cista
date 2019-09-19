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
    sizeof(void*) == 4 ? 12936113486144537849ULL : 1914690513476304635ULL;
constexpr auto const CHECKSUM_BIG_ENDIAN =
    sizeof(void*) == 4 ? 14829506244682160543ULL : 18413276534389206184ULL;

namespace graph_indexed_vec_ns::offset {

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
  cista::indexed<data::string> name_;
};

struct graph {
  static graph make_graph() {
    graph g;

    g.edges_.resize(3);
    g.nodes_.resize(3);
    g.node_names_.resize(3);

    g.nodes_[0].id_ = 0;
    g.nodes_[1].id_ = 1;
    g.nodes_[2].id_ = 2;
    g.nodes_[0].name_ = data::string{"NODE A"};
    g.nodes_[1].name_ = data::string{"NODE B"};
    g.nodes_[2].name_ = data::string{"NODE C"};
    g.nodes_[0].edges_ = {&g.edges_[0]};
    g.nodes_[1].edges_ = {&g.edges_[1]};
    g.nodes_[2].edges_ = {&g.edges_[2]};

    g.node_names_[0] = &g.nodes_[0].name_;
    g.node_names_[1] = &g.nodes_[1].name_;
    g.node_names_[2] = &g.nodes_[2].name_;

    g.edges_[0].from_ = &g.nodes_[0];
    g.edges_[0].to_ = &g.nodes_[1];
    g.edges_[1].from_ = &g.nodes_[1];
    g.edges_[1].to_ = &g.nodes_[2];
    g.edges_[2].from_ = &g.nodes_[2];
    g.edges_[2].to_ = &g.nodes_[0];

    return g;
  }

  data::indexed_vector<node> nodes_;
  data::indexed_vector<edge> edges_;
  data::vector<data::ptr<data::string>> node_names_;
};

}  // namespace graph_indexed_vec_ns::offset

using namespace graph_indexed_vec_ns::offset;

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

TEST_CASE("graph offset indexed vec serialize file") {
  constexpr auto const FILENAME = "offset_graph.bin";
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  std::remove(FILENAME);

  {
    graph g = graph::make_graph();

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
  auto const visited = bfs(&g->nodes_[0]);
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
  CHECK(*g->node_names_[0] == "NODE A");
  CHECK(*g->node_names_[1] == "NODE B");
  CHECK(*g->node_names_[2] == "NODE C");
}

TEST_CASE("graph offset indexed vec serialize buf") {
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION;

  cista::byte_buf buf;
  {
    graph g = graph::make_graph();

    cista::buf b;
    cista::serialize<MODE>(b, g);

    CHECK(b.checksum() == CHECKSUM_INTEGRITY_AND_VERSION);

    buf = std::move(b.buf_);
  }  // EOL graph

  CHECK(cista::hash(buf) == CHECKSUM_INTEGRITY_AND_VERSION);

  auto const g = cista::deserialize<graph, MODE>(buf);
  auto const visited = bfs(&g->nodes_[0]);
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
  CHECK(*g->node_names_[0] == "NODE A");
  CHECK(*g->node_names_[1] == "NODE B");
  CHECK(*g->node_names_[2] == "NODE C");
}

TEST_CASE("graph offset indexed vec serialize mmap file") {
  constexpr auto const FILENAME = "offset_graph_mmap.bin";
  constexpr auto const MODE = cista::mode::WITH_INTEGRITY |
                              cista::mode::WITH_VERSION |
                              cista::mode::DEEP_CHECK;

  std::remove(FILENAME);

  {
    graph g = graph::make_graph();

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
  auto const visited = bfs(&g->nodes_[0]);
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
  CHECK(*g->node_names_[0] == data::string{"NODE A"});
  CHECK(*g->node_names_[1] == data::string{"NODE B"});
  CHECK(*g->node_names_[2] == data::string{"NODE C"});
}

TEST_CASE("graph offset indexed vec serialize endian test") {
  constexpr auto const FILENAME = "offset_graph_big_endian.bin";
  constexpr auto const MODE =
      cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION |
      cista::mode::SERIALIZE_BIG_ENDIAN | cista::mode::DEEP_CHECK;

  std::remove(FILENAME);

  {
    graph g = graph::make_graph();

    cista::buf<cista::mmap> mmap{cista::mmap{FILENAME}};
    mmap.buf_.reserve(512);
    cista::serialize<MODE>(mmap, g);
    CHECK(mmap.checksum() == CHECKSUM_BIG_ENDIAN);
  }  // EOL graph

  auto b = cista::file(FILENAME, "r").content();
  auto const g = cista::deserialize<graph, MODE>(b);
  auto const visited = bfs(&g->nodes_[0]);
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
  CHECK(*g->node_names_[0] == "NODE A");
  CHECK(*g->node_names_[1] == "NODE B");
  CHECK(*g->node_names_[2] == "NODE C");
}