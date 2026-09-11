# Bit counting

## Overview

Counts set and clear bits using small integer algorithms. The main operations are constexpr-friendly and do not require lookup tables.

## Header

```cpp
#include "castle/bit/bit_count.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `popcount(value)` | Count all set bits. |
| `count_ones(value)` | Alias for the number of set bits. |
| `count_zeros(value)` | Count zero bits across the full width of the type. |
| Leading/trailing zero helpers | Count consecutive zeros at the most-significant or least-significant side where provided by the header. |

## Example

```cpp
uint8_t v = 0b10110000;
auto ones = castle::bit::popcount(v);
auto zeros = castle::bit::count_zeros(v);
```

## Embedded notes

- The result is based on `sizeof(T) * CHAR_BIT`, so the width is the width of the supplied type.
- The implementation uses branch-light integer algorithms suitable for small MCUs.
