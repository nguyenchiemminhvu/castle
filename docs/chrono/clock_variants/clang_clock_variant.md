# Clang Clock Variant

## Overview
Compiler-selection shim used when Castle detects Clang for chrono clocks. The current Clang path reuses the GCC-compatible backend implementation.

## Header
`#include "castle/chrono/clock_variants/clang_clock_variant.hpp"`

## Dependencies
- [GCC Clock Variant](gcc_clock_variant.md)

## Public API
| API | Description |
| --- | --- |
| `castle::chrono::detail::clock_variant::realtime_ns()` | Returns a `timespec` read from `CLOCK_REALTIME` through the reused GCC-compatible backend. Constant time. |
| `castle::chrono::detail::clock_variant::monotonic_ns()` | Returns a `timespec` read from `CLOCK_MONOTONIC` through the reused GCC-compatible backend. Constant time. |

## Usage Example
See `samples/sample_clang_clock_variant.cpp`.

```cpp
#include "castle/chrono/clock_variants/clang_clock_variant.hpp"

int main()
{
    const auto ts = castle::chrono::detail::clock_variant::realtime_ns();
    (void)ts;
    return 0;
}
```

## Constraints & Notes
- `clocks.hpp` selects this header when `CASTLE_COMPILER_CLANG` is active.
- The implementation currently forwards directly to `gcc_clock_variant.hpp`, so resolution and semantics match that backend.
- `realtime_ns()` is wall-clock based and may jump; `monotonic_ns()` is intended for monotonic elapsed-time measurement.
- Any overflow concerns arise later when `timespec` values are converted into Castle durations.
