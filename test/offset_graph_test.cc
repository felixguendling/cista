#include <queue>
#include <set>

#include "doctest.h"

#include "cista.h"

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

namespace cista {
template <>
struct use_standard_hash<graphns::offset::node> : public std::true_type {};
}  // namespace cista

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
  {
    graph g;

    CHECK(10571624168012102829ULL == cista::type_hash(g));
    cista::type_hash(g, cista::hash());

    auto const n1 = g.make_node(data::string{"NODE A"});
    auto const n2 = g.make_node(data::string{"NODE B"});
    auto const n3 = g.make_node(data::string{"NODE C"});

    auto const e1 = g.make_edge(n1->id(), n2->id());
    auto const e2 = g.make_edge(n2->id(), n3->id());
    auto const e3 = g.make_edge(n3->id(), n1->id());

    n1->add_edge(e1);
    n2->add_edge(e2);
    n3->add_edge(e3);

    cista::sfile f{"test.bin", "w+"};
    cista::serialize(f, g);

    CHECK(f.checksum() == 3447315727130902250);
  }  // EOL graph

  auto b = cista::file("test.bin", "r").content();
  CHECK(0x2FD75363A0413EEA == cista::crc64(b));

  auto const g = data::deserialize<graph>(b);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}

TEST_CASE("graph offset serialize buf") {
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
    cista::serialize(b, g);

    CHECK(b.checksum() == 3447315727130902250);

    buf = std::move(b.buf_);
  }  // EOL graph

  CHECK(0x2FD75363A0413EEA == cista::crc64(buf));

  auto const g = data::deserialize<graph>(buf);
  auto const visited = bfs(g->nodes_[0].get());
  unsigned i = 0;
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE A"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE B"});
  CHECK((*std::next(begin(visited), i++))->name_ == data::string{"NODE C"});
}

TEST_CASE("integrity and version check") {
  constexpr auto const VERSION = 10571624168012102829ULL;

  {
    graph g;

    CHECK(VERSION == cista::type_hash(g));

    auto const n1 = g.make_node(data::string{"NODE A"});
    auto const n2 = g.make_node(data::string{"NODE B"});
    auto const n3 = g.make_node(data::string{"NODE C"});

    auto const e1 = g.make_edge(n1->id(), n2->id());
    auto const e2 = g.make_edge(n2->id(), n3->id());
    auto const e3 = g.make_edge(n3->id(), n1->id());

    n1->add_edge(e1);
    n2->add_edge(e2);
    n3->add_edge(e3);

    cista::sfile f{"test.bin", "w+"};
    cista::serialize(f, g,
                     cista::mode::WITH_INTEGRITY | cista::mode::WITH_VERSION);
  }  // EOL graph

  auto f = cista::file("test.bin", "r");
  auto const buf = f.content();

  uint64_t read_version, read_checksum;
  std::rewind(f.f_);
  std::fread(&read_version, sizeof(uint64_t), 1u, f.f_);
  std::fread(&read_checksum, sizeof(uint64_t), 1u, f.f_);
  CHECK(read_version == VERSION);
  CHECK(read_checksum ==
        cista::crc64(std::string_view{reinterpret_cast<char const*>(&buf[16]),
                                      buf.size() - 16}));
}