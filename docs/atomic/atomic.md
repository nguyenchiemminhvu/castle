# Atomic

## Overview
Embedded-friendly atomic storage and memory-order primitives built directly on compiler atomics, with a mutex-backed fallback for trivially copyable non-integral types. Use it when shared state must be updated without heap allocation or STL atomics.

## Header
`#include "castle/atomic/atomic.hpp"`

## Dependencies
- [`sync/mutex.hpp`](../sync/mutex.md)
- `castle/core/traits.hpp`

## Public API
| API | Description |
| --- | --- |
| `enum memory_order` | Castle memory-order constants mapped directly to compiler `__ATOMIC_*` values. |
| `atomic<T>` | Primary template for integral atomics; lock-free when the compiler/runtime supports the target type. |
| `atomic<T*>` | Pointer specialization with scaled `fetch_add` / `fetch_sub` semantics. |
| `atomic<T, false>` | Mutex-backed fallback for trivially copyable, copyable non-integral types. |
| `store`, `load`, `exchange` | Basic atomic read/write/replace operations with explicit memory ordering. |
| `fetch_add`, `fetch_sub`, `fetch_and`, `fetch_or`, `fetch_xor` | Read-modify-write operations for integral and pointer specializations. |
| `compare_exchange_weak`, `compare_exchange_strong` | Conditional replacement; update `expected` on failure. |
| `is_lock_free`, `is_always_lock_free` | Query runtime and compile-time lock-free properties. |
| `atomic_thread_fence`, `atomic_signal_fence` | Free-function fence helpers mirroring the standard atomic fence API. |
| `atomic_*` aliases | Convenience typedefs such as `atomic_uint32_t`, `atomic_bool`, and `atomic_size_t`. |

## Usage Example
```cpp
// See: samples/sample_atomic.cpp
#include "castle/atomic/atomic.hpp"

castle::atomic<uint32_t> flags{0U};
flags.fetch_or(0x01U, castle::memory_order_release);

uint32_t snapshot = flags.load(castle::memory_order_acquire);
(void)snapshot;
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, or STL atomics.
- Integral and pointer specializations use compiler `__atomic` builtins directly.
- The non-integral fallback is deterministic but not lock-free; it serializes access with `castle::mutex`.
- Pointer arithmetic is scaled by `sizeof(T)`.
- `compare_exchange_*` for the non-integral fallback performs bitwise comparison through `memcmp`, matching the trivially copyable requirement.
