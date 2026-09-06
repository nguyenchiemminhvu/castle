# Type-safe flags

## Overview

A small value-based wrapper for groups of unsigned flag bits. A compile-time `MASK` limits which bits may be stored.

## Header

```cpp
#include "castle/bit/flags.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| `flags<T, MASK>` | Flag container for an unsigned integer type. |
| `test()` | Test whether one or more bits are present. |
| `set()` | Set one or more bits, with template and runtime forms. |
| `clear()` / `reset()` | Clear selected bits or clear the whole flag set where provided. |
| `flip()` | Toggle selected bits. |
| Value/bit queries | Read the stored pattern and query whether any/all bits are set. |

## Example

```cpp
using status_flags = castle::bit::flags<uint8_t, 0x0Fu>;
status_flags f;
f.set(0x03u);
if (f.test(0x01u)) { /* enabled */ }
```

## Embedded notes

- Only unsigned integral types are accepted.
- Operations that add bits apply `MASK`, keeping unused bits clear.
- The type does not allocate and does not use virtual dispatch.
