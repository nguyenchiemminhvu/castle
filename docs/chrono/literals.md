# Literals

## Overview
Provides user-defined literals that construct Castle duration aliases directly from integer source literals.

## Header
`#include "castle/chrono/literals.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Error Handler](../core/error_handler.md)
- [Traits](../core/traits.md)
- [Duration](duration.md)

## Public API
| API | Description |
| --- | --- |
| `operator""_ns`, `operator""_us`, `operator""_ms`, `operator""_s` | Construct `nanoseconds`, `microseconds`, `milliseconds`, and `seconds`. |
| `operator""_min`, `operator""_h`, `operator""_d`, `operator""_w` | Construct `minutes`, `hours`, `days`, and `weeks`. |

## Usage Example
See `samples/sample_literals.cpp`.

```cpp
#include "castle/chrono/literals.hpp"

using namespace castle::chrono::literals::chrono_literals;

int main()
{
    const auto period = 10_ms;
    const auto timeout = 2_s;
    (void)period;
    (void)timeout;
    return 0;
}
```

## Constraints & Notes
- Each literal stores its result in the matching `int64_t`-based Castle duration alias.
- Literals do not change clock resolution or monotonicity; they only make constants easier to read.
- Resulting duration arithmetic is still unchecked and may overflow the underlying representation.
