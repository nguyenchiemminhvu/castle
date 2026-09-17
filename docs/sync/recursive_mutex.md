# Recursive Mutex

## Overview
`castle::basic_recursive_mutex` allows the same thread to acquire the same mutex multiple times while still excluding every other thread. It is useful when nested calls or recursion need to protect one shared resource with a single lock.

## Header
`#include "castle/sync/recursive_mutex.hpp"`

## Dependencies
- [`compiler.hpp`](../core/compiler.md)
- [`config.hpp`](../core/config.md)
- [`types.hpp`](../core/types.md)
- [`mutex.hpp`](mutex.md)
- [`wait_policy.hpp`](wait_policy.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename WaitPolicy = spin_wait> class basic_recursive_mutex` | Recursive mutex that tracks the owning thread and recursion depth. |
| `using wait_policy_type = WaitPolicy` | Exposes the policy type used by the mutex specialization. |
| `basic_recursive_mutex() noexcept` | Constructs the mutex in the unlocked state with recursion depth `0`. |
| `void lock() noexcept` | Blocks until the mutex is free or already owned by the calling thread, then increments recursion depth. |
| `bool try_lock() noexcept` | Non-blocking acquire or re-acquire; returns `true` on success, `false` when another thread owns the mutex. |
| `void unlock() noexcept` | Decrements recursion depth; when the depth reaches `0`, the mutex becomes available to other threads. |
| `using recursive_mutex = basic_recursive_mutex<>` | Convenience alias for `basic_recursive_mutex<spin_wait>`. |

## Usage Example
```cpp
#include "castle/sync/recursive_mutex.hpp"

static void nested(castle::recursive_mutex& lock, int depth)
{
    lock.lock();

    if (depth > 0)
    {
        nested(lock, depth - 1);
    }

    lock.unlock();
}

int main()
{
    castle::recursive_mutex lock;
    nested(lock, 2);
    return 0;
}
```
See `samples/sample_recursive_mutex.cpp` for the full sample.

## Constraints & Notes
- Each successful `lock()` or `try_lock()` increments recursion depth and must be matched with one `unlock()` from the owning thread.
- `unlock()` silently does nothing when the caller is not the owner or when the depth is already `0`.
- Contention is surfaced only through `try_lock()` returning `false`; `lock()` has no timeout or error code.
- Ownership tracking uses a per-thread token plus an internal `basic_mutex`, with no heap allocation, exceptions, RTTI, virtual functions, or STL synchronization support.
- The default `spin_wait` policy busy-waits; provide a custom `WaitPolicy` to integrate with a scheduler.
