# CASTLE

**Compliant Automotive Safety Template Library for Embedded**

*Prototype version 2.0*

CASTLE is a lightweight, header-only CPP17-based template library built for safety-critical
embedded systems where every byte of RAM, every millisecond of jitter, and every
allocation counts. It is inspired by the [ETL (Embedded Template Library)](https://www.etlcpp.com/docs/)
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

## Why CASTLE?

Most of the C++ standard library (`std::function`, `std::string`,
`std::vector`, `std::unordered_map`, most logging frameworks, ...) relies on
dynamic memory allocation, exceptions, and virtual dispatch. That combination
is often unacceptable in automotive and safety-critical environments where:

- Heap fragmentation must be avoided over long runtimes.
- Worst-case execution time (WCET) must be provable.
- Stack usage must be bounded and known at compile time.
- Exceptions and RTTI are either disabled or forbidden by the coding standard
  (MISRA C++, AUTOSAR C++14 guidelines, ISO 26262 constraints).

CASTLE addresses these constraints by providing **stack-allocated,
fixed-capacity, `constexpr`/`noexcept`-friendly** alternatives whose behaviour
is predictable, testable, and traceable.

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

| Module | Main purpose |
|---|---|
| `atomic` | Atomic values and memory ordering |
| `bit` | Low-level bit operations and bit math |
| `callbacks` | Fixed-storage callbacks and callback policies |
| `chrono` | Durations, time points, clocks, and literals |
| `container` | `array`, fixed-capacity `vector`, `ring_buffer`, `stack` |
| `core` | Types, compiler abstraction, traits, configuration, assertions |
| `design_patterns` | Small embedded-oriented observer, singleton, and visitor helpers |
| `error` | Status/error values |
| `events` | Event dispatching, signals/slots, IPC events, tick timers |
| `iterator` | Iterator tags, traits, bounded/circular/reverse iterators |
| `math` | Integer math, ratios, square root, mean, logarithms, etc. |
| `memory` | Placement construction, destruction, storage, addresses, lifetimes |
| `mutex` | Mutex and scoped locking |
| `utility` | `move`, `forward`, `swap`, `optional`, `variant`, `pair`, `tuple`, `bitset`, etc. |

See the module guides in [`docs/`](docs/README.md).

---

## Requirements

- **C++17** background and toolchain.
- A conforming compiler: **GCC 7+**, **Clang 5+**.
- **CMake 3.10+** if you want to build the samples/tests through the provided
  project files (optional — CASTLE itself is header-only).

A typical project can use a compiler mode such as:

```text
-std=c++17
-fno-exceptions
-fno-rtti
-nostdinc++
```

---

## Installation

CASTLE is **header-only**. Any of the following is enough:

### 1. Copy the headers

Copy `include/castle/` into your project and add `include/` to your
compiler's include path. Then:

```cpp
#include "castle/container/array.h"
#include "castle/events/tick_timer.h"
...
```

### 2. CMake (`add_subdirectory`)

```cmake
add_subdirectory(third_party/castle)
target_link_libraries(my_app PRIVATE castle)
```

The `castle` target is an `INTERFACE` library that propagates the include
directory and enforces `cxx_std_17`.

### 3. CMake (`FetchContent`)

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

## Roadmap

Prototype 2.0 provides a small set of building blocks that are useful in
embedded firmware:

- fixed-size containers
- iterators
- optional values and variants
- tuples and pairs
- bit manipulation tools
- atomics and mutex helpers
- deterministic memory/lifetime helpers
- simple callback and event helpers
- compile-time and small runtime math
- chrono-style time types
- selected design patterns

Planned for upcoming releases:

- Finite state machine framework.
- Input/Output support for protocols.
- Encoder and decoder algorithms.
- CRC and checksum utilities (CRC-8/16/32, Fletcher, Adler).
- More basic math and geometry caculations.
- More expressive compile-time configuration for the event system.
- AUTOSAR / MISRA C++ conformance notes per module.

---

## License

MIT License © 2026 nguyenchiemminhvu — see [`LICENSE`](LICENSE).
