<p align="center"><img src="logo.svg"></p>

<p align="center">
    <a href="https://isocpp.org/">
        <img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg">
    </a>
    <a href="https://github.com/felixguendling/cista/actions?query=workflow%3A%22Linux+Build%22">
        <img src="https://github.com/felixguendling/cista/workflows/Linux%20Build/badge.svg">
    </a>
    <a href="https://github.com/felixguendling/cista/actions?query=workflow%3A%22Windows+Build%22">
        <img src="https://github.com/felixguendling/cista/workflows/Windows%20Build/badge.svg">
    </a>
    <a href="https://opensource.org/licenses/MIT" >
        <img src="https://img.shields.io/github/license/felixguendling/cista">
    </a>
</p>

# Simple C++ Serialization & Reflection.

Cista++ is a simple, open source (MIT license) C++17 compatible way of (de-)serializing C++ data structures.

*Single header - no dependencies. No macros. No source code generation.*

  - Raw performance - use your native structs. Supports modification/resizing of deserialized data!
  - Supports complex and cyclic data structures including cyclic references, recursive data structures, etc.
  - Save 50% memory: serialize directly to the filesystem if needed, no intermediate buffer required.
  - Fuzzing-checked though continuous fuzzing using LLVMs LibFuzzer.
  - Comes with a serializable high-performance hash map and hash set implementation based on [Google's Swiss Table](https://abseil.io/blog/20180927-swisstables).
  - Reduce boilerplate code: automatic derivation of hash and equality functions.
  - Built-in optional automatic data structure versioning through recursive type hashing.
  - Optional check sum to prevent deserialization of corrupt data.
  - Compatible with Clang, GCC, and MSVC

The underlying reflection mechanism can be used in [other ways](https://cista.rocks/#reflection), too!

**Examples:**

Download the [latest release](https://github.com/felixguendling/cista/releases/latest/download/cista.h) and try it out.

Simple example writing to a buffer:

```cpp
namespace data = cista::raw;
struct my_struct {  // Define your struct.
  int a_{0};
  struct inner {
      data::string b_;
  } j;
};

std::vector<unsigned char> buf;
{  // Serialize.
  my_struct obj{1, {data::string{"test"}}};
  buf = cista::serialize(obj);
}

// Deserialize.
auto deserialized = cista::deserialize<my_struct>(buf);
assert(deserialized->j.b_ == data::string{"test"});
```

Advanced example writing a hash map to a memory mapped file:

```cpp
namespace data = cista::offset;
constexpr auto const MODE =  // opt. versioning + check sum
    cista::mode::WITH_VERSION | cista::mode::WITH_INTEGRITY;

struct pos { int x, y; };
using pos_map =  // Automatic deduction of hash & equality
    data::hash_map<data::vector<pos>,
                   data::hash_set<data::string>>;

{  // Serialize.
  auto positions =
      pos_map{{{{1, 2}, {3, 4}}, {"hello", "cista"}},
              {{{5, 6}, {7, 8}}, {"hello", "world"}}};
  cista::buf mmap{cista::mmap{"data"}};
  cista::serialize<MODE>(mmap, positions);
}

// Deserialize.
auto b = cista::mmap("data", cista::mmap::protection::READ);
auto positions = cista::deserialize<pos_map, MODE>(b);
```

Advanced example showing support for non-aggregate types like derived classes or classes with custom constructors:

```cpp
namespace data = cista::offset;
constexpr auto MODE = cista::mode::WITH_VERSION;

struct parent {
  parent() = default;
  explicit parent(int a) : x_{a}, y_{a} {}
  auto cista_members() { return std::tie(x_, y_); }
  int x_, y_;
};
struct child : parent {
  child() = default;
  explicit child(int a) : parent{a}, z_{a} {}
  auto cista_members() {
    return std::tie(*static_cast<parent*>(this), z_);
  }
  int z_;
};

/*
 * Automatically defaulted for you:
 *   - de/serialization
 *   - hashing (use child in hash containers)
 *   - equality comparison
 *   - data structure version ("type hash")
 */
using t = data::hash_map<child, int>;

// ... usage, serialization as in the previous examples
```

# Benchmarks

Have a look at the [benchmark repository](https://github.com/felixguendling/cpp-serialization-benchmark) for more details.

| Library                                               | Serialize      | Deserialize     | Fast Deserialize |   Traverse | Deserialize & Traverse |      Size  |
| :---                                                  |           ---: |            ---: |             ---: |       ---: |                   ---: |       ---: |
| [Cap’n Proto](https://capnproto.org/capnp-tool.html)  |       105 ms   |    **0.002 ms** |       **0.0 ms** |   356 ms   |               353 ms   |    50.5M   |
| [cereal](https://uscilab.github.io/cereal/index.html) |       239 ms   |    197.000 ms   |                - |   125 ms   |               322 ms   |    37.8M   |
| [Cista++](https://cista.rocks/) `offset`              |      **72 ms** |      0.053 ms   |       **0.0 ms** |   132 ms   |             **132 ms** |  **25.3M** |
| [Cista++](https://cista.rocks/) `raw`                 |      3555 ms   |     68.900 ms   |        21.5 ms   | **112 ms** |             **133 ms** |   176.4M   |
| [Flatbuffers](https://google.github.io/flatbuffers/)  |      2349 ms   |     15.400 ms   |       **0.0 ms** |   136 ms   |             **133 ms** |    63.0M   |


# Use Cases

Reader and writer should have the same pointer width. Loading data on systems with a different byte order (endianess) is supported.
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
  - [tser](https://github.com/KonanM/tser)
  - [... many more](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats)

# Documentation

* [Installation and Usage](https://github.com/felixguendling/cista/wiki/Installation-and-Usage)
* [Serialization Reference](https://github.com/felixguendling/cista/wiki/Serialization-Reference)
* [Custom (De-)Serialization Functions](https://github.com/felixguendling/cista/wiki/Custom-(De-)Serialization-Functions)
* [Data Structure Versioning](https://github.com/felixguendling/cista/wiki/Data-Structure-Versioning)
* [Hash Containers](https://github.com/felixguendling/cista/wiki/Hash-Containers)
  * [Hashing Framework](https://github.com/felixguendling/cista/wiki/Hashing-Framework)
  * [Equality Framework](https://github.com/felixguendling/cista/wiki/Equality-Framework)
  * [Benchmark](https://github.com/felixguendling/cista/wiki/Hash-Map-Benchmark)
* [Security](https://github.com/felixguendling/cista/wiki/Security)

# Contribute

Feel free to contribute (bug reports, pull requests, etc.)!
