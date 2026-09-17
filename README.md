# CASTLE

**Compliant Automotive Safety Template Library for Embedded**

*Prototype version 2.0*

CASTLE is a lightweight, header-only CPP17-based template library built for safety-critical
embedded systems where every byte of RAM, every millisecond of jitter, and every
allocation counts.

It is inspired by the [ETL (Embedded Template Library)](https://www.etlcpp.com/docs/)
and is intended as a drop-in replacement for parts of the C++ standard library and
common enterprise frameworks that are otherwise unsuitable for automotive ECUs,
MCUs, and other resource-constrained targets.

The main idea is simple: make common C++ jobs easier without bringing the
runtime costs that are often unwanted on small targets.

CASTLE 2.0 focuses on:

- deterministic, fixed-capacity behavior where practical
- no mandatory heap allocation
- no mandatory RTTI
- no mandatory exceptions
- no virtual functions in the core data structures
- no dependency on the C++ STL
- C++17 and lower language levels
- GCC, Clang, and ARM compiler families

CASTLE is a good fit for firmware, device drivers, control software, protocol
stacks, communication buffers, sensor pipelines, schedulers, and other code
where memory and timing are important.

---

## Design principles

### No hidden heap

`vector`, `array`, `ring_buffer`, `stack`, `optional`, `variant`, and the
storage helpers keep their data in objects with known bounds.

That means the application can choose the memory budget at compile time.

### Explicit failure

A fixed-size container can become full or empty. CASTLE reports these cases
through return values such as `castle::status::ok`, `castle::status::full`, and
`castle::status::empty`, or through `bool` for simple operations.

There is no need to throw an exception for a normal capacity condition.

### Friendly to freestanding-style firmware

The public headers do not require the C++ STL. CASTLE uses compiler features
and small low-level building blocks instead.

### Compile-time first

Many utilities can be evaluated at compile time.

This is useful for register definitions, buffer sizes, time periods, and other firmware configuration.

---

## Module map

- [`algorithm/`](docs/algorithm/algorithm.md) — Range algorithms and iterator-based utilities.
- [`atomic/`](docs/atomic/atomic.md) — Atomic values and memory ordering.
- [`bit/`](docs/bit/bit.md) — Bit operations, masks, rotations, and flags.
- [`callbacks/`](docs/callbacks/delegate.md) — Fixed-storage delegates, functions, registries, and execution policies.
- [`chrono/`](docs/chrono/chrono.md) — Durations, time points, clocks, and literals.
- [`container/`](docs/container/containers.md) — Fixed-capacity sequences, associative containers, buffers, and strings.
- [`core/`](docs/core/compiler.md) — Compiler abstraction, configuration, traits, type ranges, and error handling.
- [`design_patterns/`](docs/design_patterns/observer.md) — Observer, singleton, and visitor helpers.
- [`error/`](docs/error/status.md) — Common status/error values.
- [`events/`](docs/events/delegate_dispatcher.md) — Event dispatch, signal/slot, IPC, timers, and thread-pool helpers.
- [`filter/`](docs/filter/debounce_sampling.md) — Deterministic signal sampling helpers.
- [`iterator/`](docs/iterator/iterator.md) — Iterator tags, traits, operations, and bounded iterators.
- [`math/`](docs/math/geometry/geometry.md) — Integer math, geometry, linear algebra, ratios, roots, random helpers, and more.
- [`memory/`](docs/memory/memory.md) — Alignment, storage, object lifetime, construction, and destruction helpers.
- [`serialization/`](docs/serialization/ini.md) — Fixed-capacity INI, JSON, and XML support.
- [`sync/`](docs/sync/mutex.md) — Mutexes, semaphores, shared locking, and wait policies.
- [`utility/`](docs/utility/utility.md) — Bit cast, bitset, compare, hash, optional, pair, tuple, variant, and basic move/forward utilities.

See the module guides in [`docs/`](docs/README.md).

---

## Requirements

- **C++17** background and toolchain.
- A conforming compiler: **GCC 7+**, **Clang 5+**.
- **CMake 3.10+** if you want to build the samples/tests through the provided
  project files (optional — CASTLE itself is header-only).

---

## Installation

Castle is header-only. Add `include/` to the compiler include path and include the required `.hpp` file:

```cpp
#include "castle/core/error_handler.hpp"
#include "castle/container/vector.hpp"
#include "castle/utility/optional.hpp"

castle::container::vector<int, 4U> values;
CASTLE_ASSERT((values.push_back(42) == castle::status::ok), "Push failed");
castle::optional<int> reading(42);
CASTLE_ASSERT(reading.has_value(), "Has no value");
```

A simple GCC/Clang build command is:

```text
c++ -std=c++17 -Iinclude -fno-exceptions -fno-rtti your_file.cpp
```

### CMake (`add_subdirectory`)

```cmake
add_subdirectory(third_party/castle)
target_link_libraries(my_app PRIVATE castle)
```

The `castle` target is an `INTERFACE` library that propagates the include
directory and enforces `cxx_std_17`.

### CMake (`FetchContent`)

```cmake
include(FetchContent)
FetchContent_Declare(
    castle
    GIT_REPOSITORY https://github.com/nguyenchiemminhvu/castle.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(castle)

target_link_libraries(my_app PRIVATE castle)
```

### Build the samples and tests (optional)

```bash
# Samples
./build_samples_linux.sh          # or build_samples_win.bat on Windows

# Tests
./build_test_linux.sh             # or build_test_win.bat on Windows
```

Equivalent CMake options:

```bash
cmake -S . -B build \
      -DCASTLE_BUILD_SAMPLES=ON \
      -DCASTLE_BUILD_TESTING=ON \
      -DCASTLE_FETCH_GTEST=ON
cmake --build build -j
```

---

## License

MIT License © 2026 nguyenchiemminhvu — see [`LICENSE`](LICENSE).
