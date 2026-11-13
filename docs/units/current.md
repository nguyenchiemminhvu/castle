# Current

## Overview
Electric current units for `castle::units`: SI multiples of the ampere. Use them in place of raw integers so that a driver taking `milliamperes<int32_t>` cannot receive a voltage, a resistance, or a value in the wrong scale.

## Header
`#include "castle/units/current.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `ampere_unit`, `kiloampere_unit`, `milliampere_unit`, `microampere_unit` | SI current units (`ampere_unit` is coherent); built on the base dimension `dim::current`. |
| `amperes<Rep>`, `kiloamperes<Rep>`, `milliamperes<Rep>`, `microamperes<Rep>` | `quantity` aliases for the current units. |
| `literals::_A`, `_kA`, `_mA`, `_uA` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/current.cpp`.

```cpp
#include "castle/units/current.hpp"

using namespace castle::units::literals;

const motor_driver driver(castle::units::amperes<int32_t>(2));          // limit stored as 2000 mA
const castle::units::milliamperes<int64_t> load = 1_A + 250_mA;         // 1250 mA
const castle::units::milliamperes<int32_t> coarse(castle::units::microamperes<int32_t>(3999)); // explicit: 3 mA
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Conversions to a coarser integral unit (for example microamperes to milliamperes) truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- `dim::current` is a base dimension declared in quantity.hpp; this header adds no dimension.
- Multiplying a current by a `chrono::duration` gives a charge quantity (`I T`).
