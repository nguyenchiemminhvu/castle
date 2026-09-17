# Bit utilities umbrella header

## Overview

Includes all Castle bit-manipulation helpers from one header. Use this when a module needs several bit utilities.

## Header

```cpp
#include "castle/bit/bit.h"
```

**Namespace:** `castle::bit`

## Main API

| API | Purpose |
|---|---|
| All bit helpers | Provides the APIs from `bit_core.h`, `bit_count.h`, `bit_mask.h`, `bit_math.h`, `bit_reverse.h`, `bit_rotate.h`, `bit_utils.h`, and `flags.h`. |

## Example

```cpp
#include "castle/bit/bit.h"
auto x = castle::bit::set<uint8_t>(0x00, 3);
```

## Embedded notes

- Prefer the smaller feature header when include-time cost matters.
