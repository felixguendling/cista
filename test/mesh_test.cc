#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/mmap.h"
#include "cista/serialization.h"
#endif

using namespace cista;
namespace data = cista::offset;

struct vec3 {
  float x, y, z;
};

struct mat4 {
  data::array<float, 16> d;
};

struct Vertex {
  vec3 position;
  vec3 normal;
};

struct Mesh {
  data::vector<mat4> transforms;
  data::vector<Vertex> vertices;
};
using Meshes = data::vector<Mesh>;

struct Staging {
  int stepIndex;
  data::vector<float> distances;
  data::vector<vec3> translations;
  data::vector<vec3> rotations;
};
using Stagings = data::vector<Staging>;

struct Group {
  Meshes meshes;
  Stagings stagings;
};

TEST_CASE("mesh test") {
  constexpr auto const FILENAME = "group.bin";

  std::remove(FILENAME);

  {
    Group g;
    Mesh m;
    m.transforms.emplace_back(mat4{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0.0,
                                   1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0});
    g.meshes.emplace_back(m);

    cista::buf<cista::mmap> mmap{cista::mmap{FILENAME}};
    cista::serialize(mmap, g);
  }  // EOL graph

  auto b = cista::file(FILENAME, "r").content();
  auto const g = cista::deserialize<Group>(b);
  CHECK(g->meshes[0].transforms[0].d[0] == 0.0);
  CHECK(g->meshes[0].transforms[0].d[1] == 1.0);
  CHECK(g->meshes[0].transforms[0].d[2] == 2.0);
  CHECK(g->meshes[0].transforms[0].d[3] == 3.0);
  CHECK(g->meshes[0].transforms[0].d[4] == 4.0);
  CHECK(g->meshes[0].transforms[0].d[5] == 5.0);
  CHECK(g->meshes[0].transforms[0].d[6] == 6.0);
  CHECK(g->meshes[0].transforms[0].d[7] == 7.0);
  CHECK(g->meshes[0].transforms[0].d[8] == 0.0);
  CHECK(g->meshes[0].transforms[0].d[9] == 1.0);
  CHECK(g->meshes[0].transforms[0].d[10] == 2.0);
  CHECK(g->meshes[0].transforms[0].d[11] == 3.0);
  CHECK(g->meshes[0].transforms[0].d[12] == 4.0);
  CHECK(g->meshes[0].transforms[0].d[13] == 5.0);
  CHECK(g->meshes[0].transforms[0].d[14] == 6.0);
  CHECK(g->meshes[0].transforms[0].d[15] == 7.0);
}