# Scale

## Overview
The official home of CASTLE's SI prefixes and time scales. Each one is a reduced compile-time `castle::math::ratio` that expresses a unit in terms of its coherent SI unit. `castle::units` uses them to build scaled units, and `castle::chrono::duration` uses the time scales as its periods.

## Header
`#include "castle/units/scale.hpp"`

## Dependencies
- [ratio](../math/ratio.md)

## Public API
| API | Description |
|---|---|
| `castle::units::atto`, `femto`, `pico`, `nano`, `micro`, `milli`, `centi`, `deci` | SI sub-multiples, from 10^-18 to 10^-1. |
| `castle::units::deca`, `hecto`, `kilo`, `mega`, `giga` | SI multiples, from 10^1 to 10^9. |
| `castle::units::second`, `minute`, `hour`, `day`, `week` | Time scales in seconds (1, 60, 3600, 86400, 604800). |

## Usage Example
See `samples/units/scale.cpp`.

```cpp
#include "castle/units/scale.hpp"

using per_mille = castle::math::ratio_multiply_t<castle::units::kilo, castle::units::micro>;
static_assert(castle::math::ratio_equal<per_mille, castle::units::milli>::value, "");

using ms32 = castle::chrono::duration<uint32_t, castle::units::milli>;
```

## Constraints & Notes
- Compile-time only: no storage and no runtime cost.
- Time scales use singular names. The plural names (`seconds`, `minutes`, ...) are the duration alias templates in [time](time.md).
- These aliases replace the former `castle::kilo` / `castle::minutes` names that used to live in `math/ratio.hpp`.
- `atto` and `femto` use denominators near the `intmax_t` limit, so ratio arithmetic on them overflows easily.
