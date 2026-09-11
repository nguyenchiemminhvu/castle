# Atomic operations

## Overview

Provides an STL-free atomic wrapper for embedded code. Integral and pointer specializations use compiler atomic builtins, with a small mutex-backed fallback for other supported cases in the implementation.

## Header

```cpp
#include "castle/atomic/atomic.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `memory_order_*` | Memory-order constants matching the familiar C++ atomic names. |
| `atomic<T>` | Atomic value wrapper for integral types and pointer types. |
| `load()` / `store()` | Read or write the value with a selected memory order. |
| `exchange()` | Atomically replace the value and return the previous value. |
| `fetch_add()` / `fetch_sub()` | Atomic arithmetic; pointer arithmetic is scaled by `sizeof(T)`. |
| `fetch_and()` / `fetch_or()` / `fetch_xor()` | Atomic bit operations for integral values. |
| `compare_exchange_weak()` / `compare_exchange_strong()` | Compare the current value with `expected` and replace it when equal. |
| `is_lock_free()` | Reports whether the target implementation can operate lock-free. |

## Example

```cpp
castle::atomic<unsigned> counter{0};
counter.fetch_add(1, castle::memory_order_relaxed);
unsigned n = counter.load(castle::memory_order_acquire);
```

## Embedded notes

- The atomic object itself owns its storage; there is no heap allocation.
- The API is intentionally close to `std::atomic`, but it uses compiler builtins directly.
- Do not assume every target type is lock-free; use `is_lock_free()` when that matters.
