# CASTLE

**Common Automotive Safety-critical Template Library for Embedded engineers**

*Prototype version 1.0*

CASTLE is a lightweight, header-only C++17 template library built for safety-critical
embedded systems where every byte of RAM, every millisecond of jitter, and every
allocation counts. It is inspired by the [ETL (Embedded Template Library)](https://www.etlcpp.com/docs/)
and is intended as a drop-in replacement for parts of the C++ standard library and
common enterprise frameworks that are otherwise unsuitable for automotive ECUs,
MCUs, and other resource-constrained targets.

The core idea is simple: give embedded developers a set of small, composable,
statically-sized building blocks — with no heap allocation, no exceptions,
no RTTI, and no hidden costs.

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

## Features

- **Deterministic behaviour, tailored for embedded targets** — bounded
  capacities, bounded stack usage, no exceptions.
- **No heap allocation** — every container, callback, event queue, timer, and
  log record lives on the stack or in static storage. Capacity is a
  compile-time template parameter.
- **Easy-to-set-up event mechanism** — a family of ready-to-use dispatchers
  (`event_dispatcher`, `inplace_event_dispatcher`, `signal_event`,
  `tick_timer`, `inplace_signal_event`, `inplace_tick_timer`, `sigslot`), wiring
  publishers and subscribers takes only a few lines of code.
- **Easy to integrate — header-only** — no build system required for
  consumption; drop `include/` on your include path and `#include
  <castle/...>`. A CMake `INTERFACE` target `castle` is also provided.
- **High availability through compile-time configuration** — sizes,
  capacities, signal maps, event topologies, and even bound member functions
  can be encoded as template parameters (`function_ct`, `function_ct_m`,
  `function_ct_im`, `signal_config<...>`), so misconfiguration becomes a
  compile-time error rather than a runtime failure.
- **Fewer virtual functions** — the library favours static polymorphism
  (templates, CRTP, type erasure with a single small vtable, compile-time
  binding of callbacks) over classical inheritance. Where a virtual interface
  is unavoidable, it is minimal and its lifetime is explicit.
- **Many small, focused utilities** — bit manipulation, safe
  numeric casts, iterators, type traits, design pattern helpers, logger...

---

## Requirements

- **C++17** background and toolchain.
- A conforming compiler: **GCC 7+**, **Clang 5+**, or **MSVC 19.14+**.
- **CMake 3.10+** if you want to build the samples/tests through the provided
  project files (optional — CASTLE itself is header-only).

CASTLE does not use exceptions or RTTI, so it can be compiled with
`-fno-exceptions -fno-rtti` (or the equivalent MSVC switches).

---

## Installation

CASTLE is **header-only**. Any of the following is enough:

### 1. Copy the headers

Copy `include/castle/` into your project and add `include/` to your
compiler's include path. Then:

```cpp
#include <castle/buffers/ring_buffer.h>
#include <castle/events/tick_timer.h>
#include <castle/logging/logging.h>
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

Every module has a matching runnable example under `samples/` (for example
`sample_ring_buffer.cpp`, `sample_signal_event.cpp`, `sample_tick_timer.cpp`,
`sample_logging.cpp`, `sample_safe_cast.cpp`, ...), and per-module
documentation lives in `docs/`.

---

## Roadmap

Prototype 1.0 covers bit utilities, fixed-capacity buffers, callables,
event/timer/signal dispatchers, signals and slots, logging, iterators,
math, safe casts, and a few design-pattern helpers.

Planned for upcoming releases:

- More containers (fixed-capacity heaps) and their utility functions.
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
