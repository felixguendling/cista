<p align="center"><img src="logo.svg"></p>

<p align="center">
    <a href="https://isocpp.org/">
        <img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg">
    </a>
    <a href="https://travis-ci.org/felixguendling/cista">
        <img src="https://travis-ci.org/felixguendling/cista.svg?branch=master">
    </a>
    <a href="https://ci.appveyor.com/project/felixguendling/cista/branch/master">
        <img src="https://ci.appveyor.com/api/projects/status/cwcwp808uhaa5d3w/branch/master?svg=true">
    </a>
    <a href="https://coveralls.io/github/felixguendling/cista?branch=master">
        <img src="https://coveralls.io/repos/github/felixguendling/cista/badge.svg?branch=master">
    </a>
    <a href="https://opensource.org/licenses/MIT" >
        <img src="https://img.shields.io/apm/l/vim-mode.svg">
    </a>
</p>

# Simple C++ Serialization & Reflection.

Cista++ is a simple, open source (MIT license) C++17 compatible way of (de-)serializing C++ data structures.

*Single header. No macros. No source code generation.*

  - Raw performance - use your native structs. Supports modification/resizing of deserialized data!
  - Supports complex and cyclic data structures including cyclic references, recursive data structures, etc.
  - Save 50% memory: serialize directly to the filesystem if needed, no intermediate buffer required.
  - Compatible with Clang, GCC, and MSVC

The underlying reflection mechanism can be used in [other ways](https://cista.rocks/#reflection), too!

**Example:**

Download the [latest release](https://github.com/felixguendling/cista/releases/download/v0.4/cista.h) or try it [online](https://onlinegdb.com/ByuE4jo-4).

```cpp
#include <cassert>
#include "cista.h"

int main() {
  namespace data = cista::offset;

  struct my_struct {  // Define your struct.
    int a_{0};
    struct inner {
      int b_{0};
      data::string d_;
    } j;
  };

  std::vector<unsigned char> buf;
  {  // Serialize an object of class my_struct.
    my_struct obj{1, {2, data::string{"test"}}};
    buf = cista::serialize(obj);
  }  // End of life for the "obj" value

  // Deserialize and read.
  auto deserialized = data::deserialize<my_struct>(buf);
  assert(deserialized->j.d_ == data::string{"test"});
}
```

# Benchmarks

Have a look at the [benchmark repository](https://github.com/felixguendling/cpp-serialization-benchmark) for more details.

| Library                                               | Serialize      | Deserialize     | Fast Deserialize |   Traverse | Deserialize & Traverse |      Size  |
| :---                                                  |           ---: |            ---: |             ---: |       ---: |                   ---: |       ---: |
| [Cap’n Proto](https://capnproto.org/capnp-tool.html)  |       105 ms   |    **0.002 ms** |       **0.0 ms** |   356 ms   |               353 ms   |    50.5M   |
| [cereal](https://uscilab.github.io/cereal/index.html) |       239 ms   |    197.000 ms   |                - |   125 ms   |               322 ms   |    37.8M   |
| [Cista++](https://cista.rocks/) `offset`              |      **72 ms** |      0.053 ms   |       **0.0 ms** |   132 ms   |             **132 ms** |  **25.3M** |
| [Cista++](https://cista.rocks/) `raw`                 |      3555 ms   |     68.900 ms   |        21.5 ms   | **112 ms** |             **133 ms** |   176.4M   |
| [Flatbuffers](https://google.github.io/flatbuffers/)  |      2349 ms   |     15.400 ms   |       **0.0 ms** |   136 ms   |             **133 ms** |   378.0M   |


# Use Cases

Reader and writer should run on the same architecture (e.g. 64 bit big endian).
Examples:

  - Asset loading for all kinds of applications (i.e. game assets, GIS data, large graphs, etc.)
  - Transferring data over network
  - shared memory applications

Currently, only C++17 software can read/write data.
But it should be possible to generate accessors
for other programming languages, too.

# Alternatives

If you need to be compatible with other programming languages
or require protocol evolution (downward compatibility)
you should look for another solution:

  - [Protocol Buffers](https://developers.google.com/protocol-buffers/)
  - [Cap’n Proto](https://capnproto.org/)
  - [Flatbuffers](https://google.github.io/flatbuffers/)
  - [cereal](https://uscilab.github.io/cereal/)
  - [Boost Serialization](https://www.boost.org/doc/libs/1_68_0/libs/serialization/doc/index.html)
  - [MessagePack](https://msgpack.org/)
  - [... many more](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats)

# Usage

  - Declare the data structures you want to serialize as regular C++ structs
    (using scalar types, `cista::raw/offset::string`, `cista::raw/offset::unique_ptr<T&>`,
    and `cista::raw/offset::vector<T&>` - more types such as `map`/`set`/etc. may follow).
  - Do *NOT* declare any constructors (reflection will not work otherwise).
  - Always use data types with known sizes such as `int32_t`, `uint8_t` for compatibility across platforms (with the same architecture).
  - To use pointers: store the object you want to reference as `cista::raw/offset::unique_ptr<T&>` and use a raw pointer `T*` to reference it.
  - Optional: if you need deterministic buffer contents, you need to fill spare bytes in your structs (see the advanced example below).

Cista++ supports two serialization formats:

*Offset Based Data Structures*

  - `+` can be read without any deserialization step  
  - `+` suitable for shared memory applications  
  - `-` slower at runtime (pointers need to be resolved using one more add)

*Raw Data Structures*

  - `-` deserialize step takes time (but still very fast also for GBs of data)  
  - `-` the buffer containing the serialized data needs to be modified  
  - `+` fast runtime access (raw access)  


# API Documentation

Both namespaces `cista::offset` and `cista::raw` have the same structure. They provide the same data structures and functions. They have the same behavior.

### Data Structures

The following data structures exist in `cista::offset` and `cista::raw`:

  - **`vector<T>`**: serializable version of `std::vector<T>`
  - **`string`**: serializable version of `std::string`
  - **`unique_ptr<T>`**: serializable version of `std::unique_ptr<T>`
  - **`ptr<T>`**: serializable pointer: `cista::raw::ptr<T>` is just a `T*`, `cista::offset::ptr<T>` is a specialized data structure that behaves mostly like a `T*` (overloaded `->`, `*`, etc. operators).

Currently, `vector`, `string`, and `unique_ptr` do not provide exactly the same interface as their `std::` equivalents. Standard compliance was not a goal. This can change in future releases. It is possible to add more data structures to Cista++.

### Serialization and Deserialization Functions

#### Serialization

The following methods can be used to serialize either to a `std::vector<uint8_t>` (default) or to an arbitrary serialization target.

  - **`std::vector<uint8_t> cista::serialize<T>(T const&)`** serializes an object of type `T`and returns a buffer containing the serialized object.
  - **`void cista::serialize<Target, T>(Target&, T const&)`** serializes an object of type `T` to the specified target. Targets are either `cista::buf` or `cista::file`. Custom target sturcts should provide `write` functions as described [here](#serialization).

#### Deserialization

The following functions exist in `cista::offset` and `cista::raw`:

  - **`T* deserialize<T, Container>(Container&)`** deserializes an object from a `std::vector<uint8_t>` or similar data structure. This function throws a `std::runtimer_error` if the data is not well-formed.
  - **`T* deserialize<T>(uint8_t* from, uint8_t* to)`** deserializes an object from a pointer range. This function throws a `std::runtimer_error` if the data is not well-formed.
  - **`T* unchecked_deserialize<T, Container>(Container&)`** deserializes an object from a `std::vector<uint8_t>` or similar data structure. No checking is performed!
  - **`T* unchecked_deserialize<T>(uint8_t* from, uint8_t* to)`** deserializes an object from a pointer range. No checking is performed!

`cista::offset::unchecked_deserialize` performs just a pointer cast!

# Advanced Example

The following example shows serialization and deserialization
of more complex data structures.

## Data structure definition

Try it [online](https://onlinegdb.com/rJ-DDD9ZN)!

```cpp
// Use the raw serialization format.
namespace data = cista::raw;

// Forward declare `node` to be able to use it in `edge`.
struct node;

// Always use types that have a fixed size on every
// platform: (u)int16_t, (u)int32_t, ...
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

  // Use unique_ptr to enable pointers to these objects.
  data::vector<data::unique_ptr<node>> nodes_;
  data::vector<data::unique_ptr<edge>> edges_;
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

  auto const n1 = g.make_node(data::string{"NODE A"});
  auto const n2 = g.make_node(data::string{"NODE B"});
  auto const n3 = g.make_node(data::string{"NODE C"});

  auto const e1 = g.make_edge(n1->id(), n2->id());
  auto const e2 = g.make_edge(n2->id(), n3->id());
  auto const e3 = g.make_edge(n3->id(), n1->id());

  n1->add_edge(e1);
  n2->add_edge(e2);
  n3->add_edge(e3);

  cista::file f{"test.bin", "wb"};
  cista::serialize(f, g);
}  // EOL graph

auto b = cista::file("test.bin", open_mode::READ).content();
auto const g = data::deserialize<graph>(b);

use(g);
```

# How Does It Work?

Basically, the only thing the <code>cista::serialize()</code>
call does, is to copy everything into one coherent target
(e.g. file or memory buffer) byte-by-byte recursively.
Additionally, each pointer gets converted to an offset
at serialization and back to a real pointer at deserialization.
Every data structure can be (de-)serialized using a custom
(de-)serialization function (see below for more details).
All this is done recursively.

# Security

Generally, all serialized data is checked so that every pointer `T*` is either a `nullptr` or points to a valid position within the buffer with enough bytes for `T`. Scalar values are checked to fit into the buffer at their position, too. Code can be found [here](https://github.com/felixguendling/cista/blob/master/include/cista/serialization.h#L215).

Note that modifying serialized data may corrupt it in unexpected ways. Therefore, it is not safe to access modified deserialized data coming from untrusted sources. However, deserializing and reading data from untrusted sources (without modifying the data) is safe. 

# Custom (De-)Serialization Functions

If you have 3rd-party structs, structs with constructors
or structs that manage memory, etc. you need to override
the serialize and deserialize functions.

*Note that usually you should not need to do this! Every combination of the cista::offset/raw data structures should work without custom (de-)serialzation functions.*

## Serialization

By default, every value gets copied raw byte-by-byte.
For each type you would like serialize with a custom function,
you need to override the following function for your type.

This function will be called with
  - The serialization context (described below).
    It provides functions to write to the buffer
    and to translate pointers to offsets.
  - A pointer to the original value (i.e. not the serialized!)
    of your struct.
  - The offset value where the value of <code>YourType</code> has been
    copied to. You can use this information to adjust certain
    members of <code>YourType</code>. For example:
    <code>ctx.write(pos + offsetof(cista::string, h_.ptr_), start)</code>.
    This overrides the pointer contained in <code>cista::string</code>
    with the offset, the bytes have been copied to by calling
    <code>start = c.write(orig->data(), orig->size(), 1)</code>.

```cpp
template <typename Ctx>;
void serialize(Ctx&, YourType const*, cista::offset_t const);
```


The `Ctx` parameter is templated to support different
serialization targets (e.g. file and buffer).
`Ctx` provides the following members:

```cpp
struct serialization_context {
  struct pending_offset {
    void* origin_ptr_;  // pointer to the original
    offset_t pos_;      // offset where it is serialized
  };

  /**
   * Writes the values at [ptr, ptr + size[ to
   * the end of the serialization target buffer.
   * Adjusts for alignment if needed and returns
   * the new (aligned) offset the value was written to.
   *
   * Appends to the buffer (resize).
   *
   * \param ptr         points to the data to write
   * \param size        number of bytes to write
   * \param alignment   the alignment to consider
   * \return the alignment adjusted offset
   */
  offset_t write(void const* ptr, offset_t const size,
                 offset_t alignment = 0);

  /**
   * Overrides the value at `pos` with value `val`.
   *
   * Note: does not append to the buffer.
   * The position `pos` needs to exist already
   * and provide enough space to write `val`.
   *
   * \param pos  the position to write to
   * \param val  the value to copy to position `pos`
   */
  template <typename T>;
  void write(offset_t const pos, T const& val);

  /**
   * Lookup table from original pointer
   * to the offset the data was written to.
   */
  std::map<void*, offset_t>; offsets_;

  /**
   * Pending pointers that could not yet get
   * resolved (i.e. the value they point to has not
   * yet been written yet but will be later).
   */
  std::vector<pending_offset>; pending_;
};
```

## Deserialization

To enable a custom deserialization, you need to create a specialized
function for your type with the following signature:

```cpp
void deserialize(cista::deserialization_context const&, YourType*);
void unchecked_deserialize(cista::deserialization_context const&, YourType*);
```

With this function you should:
  - for the non-"unchecked" version: check that
    everything fits into the buffer and that
    pointers point to memory addresses
    that are located inside the buffer
    using `deserialization_context::check()`
  - convert offsets back to pointers using the
    `deserialization_context::deserialize()`
    member function.

Both functions (`deserialize()` and `check()`)
are provided by the `deserialization_context`:
```cpp
struct deserialization_context {
  /**
   * Converts a stored offset back to the original pointer
   * by adding the base address.
   *
   * \param ptr  offset (given as a pointer)
   * \return offset converted to pointer
   */
  template <typename T, typename Ptr>;
  T deserialize(Ptr* ptr) const;

  /**
   * Checks whether the pointer points to
   * a valid memory address within the buffer
   * where at least `size` bytes are available.
   *
   * \param el    the memory address to check
   * \param size  the size to check for
   * \throws if there are bytes outside the buffer
   */
  template <typename T>;
  void check(T* el, size_t size) const;
};
```

# Contribute

Feel free to contribute (bug reports, pull requests, etc.)!
