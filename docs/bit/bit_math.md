# Bit-oriented integer math

## Overview

Useful integer tests and power-of-two helpers expressed in terms of bits.

## Header

```cpp
#include "castle/bit/bit_math.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `is_even()` / `is_odd()` | Test integer parity from the least-significant bit. |
| `is_power_of_two()` | Checks for an exact positive power of two. |
| `next_power_of_two()` | Returns the smallest power of two greater than or equal to the input; zero maps to one. |
| Compile-time forms | Matching `_const` and template forms are supplied for compile-time use. |

## Example

```cpp
bool ok = castle::bit::is_power_of_two(256u);
auto n = castle::bit::next_power_of_two(300u);
```

## Embedded notes

- Negative signed inputs are not treated as powers of two.
- These helpers are useful for buffer sizing and ring-buffer indexing.
