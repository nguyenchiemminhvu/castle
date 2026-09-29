# GCC Clock Variant

## Overview
GCC-compatible clock backend used by Castle chrono clocks and currently reused by the ARM, Clang, and default selection shims. The adapter selects POSIX clock reads before Zephyr precision-clock reads when `CASTLE_USING_POSIX_APIS` is `1`.

## Header
`#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"`

## Dependencies
- [Compiler](../../core/compiler.md)
- [Config](../../core/config.md)
- [Error Handler](../../core/error_handler.md)
- [Traits](../../core/traits.md)

## Public API
| API | Description |
| --- | --- |
| `class castle::chrono::system_clock_adapter` | Platform adapter exposing wall-clock and monotonic reads plus PTP adjustments. |
| `static timespec system_clock_adapter::realtime_ns()` | Reads the platform system clock and returns whole seconds plus nanoseconds. Constant time. |
| `static timespec system_clock_adapter::monotonic_ns()` | Reads the platform steady clock and returns whole seconds plus nanoseconds. Constant time. |
| `castle::status system_clock_adapter::step(int64_t)` | Applies a phase correction; Zephyr uses `precision_clock_adjust_phase()` before any non-Zephyr POSIX path. |
| `castle::status system_clock_adapter::slew(int32_t)` | Applies a frequency correction; Zephyr uses `precision_clock_adjust_rate()` before any non-Zephyr POSIX path. |

## Usage Example
See `samples/sample_gcc_clock_variant.cpp`.

```cpp
#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"

int main()
{
    const auto realtime = castle::chrono::system_clock_adapter::realtime_ns();
    const auto monotonic = castle::chrono::system_clock_adapter::monotonic_ns();
    (void)realtime;
    (void)monotonic;
    return 0;
}
```

## Constraints & Notes
- This backend enables the `CASTLE_CHRONO_SYSTEM_CLOCK_VARIANT` and `CASTLE_CHRONO_STEADY_CLOCK_VARIANT` paths consumed by `clocks.hpp`.
- Reads use POSIX `clock_gettime()` whenever `CASTLE_USING_POSIX_APIS` is `1`, even when `__ZEPHYR__` is also defined.
- If POSIX APIs are unavailable, Zephyr reads use `precision_clock_read()` through the `CASTLE_CHRONO_ZEPHYR_SYSTEM_PRECISION_CLOCK` and `CASTLE_CHRONO_ZEPHYR_STEADY_PRECISION_CLOCK` compile-time pointer expressions.
- Adjustment methods retain Zephyr-first selection: on Zephyr they use the configured precision clock, while non-Zephyr POSIX builds use `clock_settime()` and `clock_adjtime()` when available.
- `realtime_ns()` is wall-clock based and may move backward or forward if the system clock changes.
- `monotonic_ns()` is intended for elapsed-time measurement and should not move backward during normal operation.
- Failures are reported through `CASTLE_ASSERT`; no exceptions are thrown.
- Conversion from `timespec` to Castle duration ticks happens in `clocks.hpp` and may overflow if extremely large values are forced into a smaller representation.
