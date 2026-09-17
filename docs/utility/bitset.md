# Bitset utility

## Overview

STL-independent fixed bitset utility with proxy bit references and common bitset operations.

## Header

```cpp
#include "castle/utility/bitset.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `bitset<N>` | Compile-time sized set of `N` bits. |
| Bit access | `operator[]`, `test`, and proxy references for writes. |
| Whole-set operations | Set, reset/clear, flip, and count operations. |
| Conversions/queries | Read or convert the stored pattern using the APIs provided by the header. |

## Example

```cpp
castle::bitset<32> flags;
flags.set(3);
if (flags.test(3)) { /* ... */ }
```

## Embedded notes

- This is a utility-level bitset; the separate `container/bitset.h` lives with the container family.
- No heap is used.
