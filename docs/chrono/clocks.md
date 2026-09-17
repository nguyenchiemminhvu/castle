# Clocks

## Overview
Defines Castle wall-clock and monotonic clock types while leaving the actual timer source configurable through compiler-selected backends or platform hooks.

## Header
`#include "castle/chrono/clocks.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Error Handler](../core/error_handler.md)
- [Traits](../core/traits.md)
- [Ratio](../math/ratio.md)
- [Time Point](time_point.md)
- One selected backend: [ARM Clock Variant](clock_variants/arm_clock_variant.md), [Clang Clock Variant](clock_variants/clang_clock_variant.md), [GCC Clock Variant](clock_variants/gcc_clock_variant.md), or [Default Clock Variant](clock_variants/default_clock_variant.md)

## Public API
| API | Description |
| --- | --- |
| `extern "C" int64_t castle_chrono_system_clock_now() noexcept;` | Platform hook used when `CASTLE_CHRONO_SYSTEM_CLOCK_NOW_API` is not defined. Returns system-clock ticks in `CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD` units. |
| `extern "C" int64_t castle_chrono_steady_clock_now() noexcept;` | Platform hook used when `CASTLE_CHRONO_STEADY_CLOCK_NOW_API` is not defined. Returns steady-clock ticks in `CASTLE_CHRONO_STEADY_CLOCK_PERIOD` units. |
| `using system_clock_duration` | Duration type used by `system_clock`; defaults to nanoseconds unless overridden. |
| `using steady_clock_duration` | Duration type used by `steady_clock`; defaults to nanoseconds unless overridden. |
| `class system_clock` | Wall-clock source with `duration`, `rep`, `period`, `time_point`, `is_steady`, `now()`, `to_time_t()`, and `from_time_t()`. All operations are constant time. |
| `class steady_clock` | Monotonic clock source with `duration`, `rep`, `period`, `time_point`, `is_steady`, and `now()`. All operations are constant time. |
| `using high_resolution_clock = system_clock;` | Alias that shares the same epoch and time-point type as `system_clock`. |

## Usage Example
See `samples/sample_clocks.cpp`.

```cpp
#include "castle/chrono/clocks.hpp"

int main()
{
    const auto start = castle::chrono::steady_clock::now();
    const auto epoch_plus_two = castle::chrono::system_clock::from_time_t(2);
    const auto seconds = castle::chrono::system_clock::to_time_t(epoch_plus_two);
    (void)start;
    (void)seconds;
    return 0;
}
```

## Constraints & Notes
- Backend selection follows compiler detection order in `castle/core/compiler.hpp`: ARM, Clang, GCC, then default.
- The current ARM, Clang, and default paths all reuse the GCC-compatible `clock_gettime` backend unless the application overrides the source with `CASTLE_CHRONO_*_NOW_API` or supplies the external C hook functions.
- `system_clock` is not monotonic. `steady_clock` is intended to be monotonic, but the platform hook must preserve that property.
- Default tick periods are nanoseconds; defining `CASTLE_CHRONO_SYSTEM_CLOCK_PERIOD` or `CASTLE_CHRONO_STEADY_CLOCK_PERIOD` changes the interpretation of returned tick counts.
- Conversions and arithmetic are unchecked and may overflow the underlying `int64_t` representation.
