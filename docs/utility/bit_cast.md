# Bit-preserving cast

## Overview

Reinterprets the object representation of one trivially copyable type as another same-size type without pointer aliasing tricks.

## Header

```cpp
#include "castle/utility/bit_cast.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `bit_cast<To>(from)` | Return a same-size value with the same bits. |

## Example

```cpp
float f = 1.0f;
auto raw = castle::bit_cast<uint32_t>(f);
```

## Embedded notes

- Source and destination must satisfy the header’s size and type constraints.
- Use this for serialization/inspection, not for changing object lifetime.
