# GCC Clock Variant

## Overview
POSIX `clock_gettime` backend used by Castle chrono clocks for GCC-compatible builds and currently reused by the ARM, Clang, and default selection shims.

## Header
`#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"`

## Dependencies
- [Compiler](../../core/compiler.md)
- [Error Handler](../../core/error_handler.md)
- [Traits](../../core/traits.md)

## Public API
| API | Description |
| --- | --- |
| `struct castle::chrono::detail::clock_variant` | Backend helper type that exposes POSIX wall-clock and monotonic reads. |
| `static timespec clock_variant::realtime_ns()` | Reads `CLOCK_REALTIME` and returns whole seconds plus nanoseconds. Constant time. |
| `static timespec clock_variant::monotonic_ns()` | Reads `CLOCK_MONOTONIC` and returns whole seconds plus nanoseconds. Constant time. |

## Usage Example
See `samples/sample_gcc_clock_variant.cpp`.

```cpp
#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"

int main()
{
    const auto realtime = castle::chrono::detail::clock_variant::realtime_ns();
    const auto monotonic = castle::chrono::detail::clock_variant::monotonic_ns();
    (void)realtime;
    (void)monotonic;
    return 0;
}
```

## Constraints & Notes
- This backend enables the `CASTLE_CHRONO_SYSTEM_CLOCK_GCC_VARIANT` and `CASTLE_CHRONO_STEADY_CLOCK_GCC_VARIANT` paths consumed by `clocks.hpp`.
- `realtime_ns()` is wall-clock based and may move backward or forward if the system clock changes.
- `monotonic_ns()` is intended for elapsed-time measurement and should not move backward during normal operation.
- Failures are reported through `CASTLE_ASSERT`; no exceptions are thrown.
- Conversion from `timespec` to Castle duration ticks happens in `clocks.hpp` and may overflow if extremely large values are forced into a smaller representation.
