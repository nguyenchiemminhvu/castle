# Fixed-size bitset

## Overview

A fixed-size collection of bits stored in integral words. It provides safe individual-bit operations and a proxy reference for `operator[]`.

## Header

```cpp
#include "castle/container/bitset.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `bitset<N>` | Stores exactly `N` logical bits. |
| `set(position)` / `clear(position)` | Change one bit with status checking. |
| `flip(position)` / `flip()` | Toggle one bit or all bits. |
| `test(position)` | Read one bit; out-of-range reads return false. |
| `count()` | Count set bits. |
| Proxy `reference` | Allows writable `operator[]` access to individual bits. |

## Example

```cpp
castle::container::bitset<16> b;
b.set(3);
if (b.test(3)) { /* ... */ }
b.flip();
```

## Embedded notes

- Invalid bit positions return Castle status values for mutating operations.
- Unused bits in the final storage word are sanitized after whole-set operations.
