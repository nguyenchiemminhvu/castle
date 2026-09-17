# Compile-time ratios

## Overview

Represents rational numbers at compile time and provides the period building blocks used by Castle chrono.

## Header

```cpp
#include "castle/math/ratio.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `ratio<Num, Den>` | Compile-time rational value reduced to a canonical form. |
| Arithmetic aliases | Ratio addition, subtraction, multiplication, and division where provided. |
| Comparison helpers | Compare ratios at compile time. |
| Time-unit aliases | `nano`, `micro`, `milli`, `seconds`, `minutes`, `hours`, `days`, and `weeks` used by chrono. |

## Example

```cpp
using half = castle::math::ratio<1, 2>;
using milli = castle::milli;
```

## Embedded notes

- Denominator and numerator validity are checked at compile time.
- Ratios are a good way to keep time scaling out of runtime code.
