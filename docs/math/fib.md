# Fibonacci

## Overview

Compile-time/runtime Fibonacci number helpers for small integer sequences.

## Header

```cpp
#include "castle/math/fib.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Fibonacci helper | Compute the Fibonacci number for an index. |
| Compile-time form | Use the template/constexpr form for constants. |

## Example

```cpp
constexpr auto f10 = castle::math::fib<10>::value;
```

## Embedded notes

- Fibonacci grows quickly; choose a type wide enough for the requested index.
