<p align="center"><img src="logo.svg"></p>

[![Build Status](https://travis-ci.org/felixguendling/cista.svg?branch=master)](https://travis-ci.org/felixguendling/cista) [![Build status](https://ci.appveyor.com/api/projects/status/cwcwp808uhaa5d3w?svg=true)](https://ci.appveyor.com/project/felixguendling/cista)

# Simple C++ Serialization.

Cista++ is a simple, open source (MIT license) C++17 compatible way of (de-)serializing C++ data structures.

*Single header. No macros. No source code generation.*

  - Raw performance
  - No macro magic, no additional definition files, no generated source code
  - Supports complex and cyclic datastructures including cyclic references, recursive data structures, etc.
  - Compatible with Clang, GCC, and MSVC


# Use Cases

You can use this library to load data from *trusted* sources *ONLY*.

Reader and writer should run on the same architecture (e.g. 64 bit big endian).
Examples:

  - Asset loading for all kinds of applications (i.e. game assets, GIS data, large graphs, etc.)
  - Transferring data over network to/from trusted parties

Currently, only C++17 software can read/write data.
But it should be possible to generate accessors
for other programming languages, too.

# Alternatives

If you need to be compatible with other programming languages,
need to process data from untrusted sources,
require protocol evolution (downward compatibility)
you should look for another solution.
The following libraries provide some/all of those features.

Alternative libraries:

  - [Protocol Buffers](https://developers.google.com/protocol-buffers/)
  - [Cap’n Proto](https://capnproto.org/)
  - [Flatbuffers](https://google.github.io/flatbuffers/)
  - [cereal](https://uscilab.github.io/cereal/)
  - [Boost Serialization](https://www.boost.org/doc/libs/1_68_0/libs/serialization/doc/index.html)
  - [MessagePack](https://msgpack.org/)
  - [... many more](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats)

# Usage

  - Declare the data structures you want to serialize as regular C++ structs
    (using scalar types, `utl::string`, `utl::unique_ptr<T&>`,
    and `utl::vector<T&>` - more types such as `map`/`set`/etc. may follow).
  - Do *NOT* declare any constructors (reflection will not work otherwise).
  - Always use data types with known sizes such as `int32_t`, `uint8_t`.
  - To use pointers: store the object you want to reference as `utl::unique_ptr<T&>` and use a raw pointer `T*` to reference it.
  - Optional: if you need deterministic buffer contents, you need to fill spare bytes in your structs.


# Advanced Example


The following example shows serialization and deserialization
of more complex data structures.

## Data structure definition

```cpp
// Forward declare `node` to be able to use it in `edge`.
struct node;

// Always use types that have a fixed size on every
// platform: (u)int16_t, (u)int32_t, ...
using node_id_t = uint32_t;

struct edge {
  node* from_;
  node* to_;
};

struct node {
  void add_edge(edge* e) { edges_.emplace_back(e); }
  node_id_t id() const { return id_; }

  node_id_t id_{0};
  node_id_t fill_{0};  // optional: zero out spare bytes for
                       // deterministic buffer contents
  utl::vector<edge*&> edges_;
  utl::string name_;
};

struct graph {
  node* make_node(utl::string name) {
    return nodes_
        .emplace_back(utl::make_unique<node&>(
            node{next_node_id_++, 0, utl::vector<edge*&>{0u},
                 std::move(name)}))
        .get();
  }

  edge* make_edge(node_id_t const from,
                  node_id_t const to) {
    return edges_
        .emplace_back(utl::make_unique<edge&>(
            edge{nodes_[from].get(), nodes_[to].get()}))
        .get();
  }

  // Use unique_ptr to enable pointers to these objects.
  utl::vector<utl::unique_ptr<node&>&> nodes_;
  utl::vector<utl::unique_ptr<edge&>&> edges_;
  node_id_t next_node_id_{0};
  node_id_t fill_{0};  // optional: zero out spare bytes for
                       // deterministic buffer contents
};
```

## Creating the Graph and De-/Serialization

```cpp
// Create cyclic graph with nodes and edges.
{
  graph g;

  auto const n1 = g.make_node(utl::string{"NODE A"});
  auto const n2 = g.make_node(utl::string{"NODE B"});
  auto const n3 = g.make_node(utl::string{"NODE C"});

  auto const e1 = g.make_edge(n1->id(), n2->id());
  auto const e2 = g.make_edge(n2->id(), n3->id());
  auto const e3 = g.make_edge(n3->id(), n1->id());

  n1->add_edge(e1);
  n2->add_edge(e2);
  n3->add_edge(e3);

  // Serialize graph data structure to file.
  utl::sfile f{"graph.bin", "wb"};
  utl::serialize(f, g);
}  // End of life for `g`.

// Deserialize
auto b = utl::file("test.bin", "r").content();
auto const g =
    utl::deserialize<graph>(b.begin(), b.end());

// Read graph.
use(g);
```

# Custom (De-)Serialization Functions

If you have 3rd-party structs, structs with constructors
or structs that manage memory, etc. you need to override
the serialize and deserialize functions.

## Serialization

```cpp
void serialize(Ctx&, YourType const*, utl::offset_t const) {}`
```

## Deserialization

```cpp
void deserialize(utl::deserialization_context const&, YourType*) {}
```
