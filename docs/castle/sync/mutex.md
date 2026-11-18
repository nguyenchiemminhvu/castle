# Mutex

## Overview
`castle::basic_mutex` is Castle's non-recursive mutual-exclusion primitive. It protects a short critical section with a spinlock-style `lock()`/`unlock()` pair and exposes a pluggable `WaitPolicy` so blocked callers can either spin or yield through platform-specific integration code.

## Header
`#include "castle/sync/mutex.hpp"`

## Dependencies
- [`compiler.hpp`](../core/compiler.md)
- [`wait_policy.hpp`](wait_policy.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename WaitPolicy = spin_wait> class basic_mutex` | Non-recursive mutex; `WaitPolicy::wait()` runs once per failed `lock()` retry. |
| `using wait_policy_type = WaitPolicy` | Exposes the policy type used by the mutex specialization. |
| `basic_mutex()` | Constructs the mutex in the unlocked state. |
| `void lock() noexcept` | Blocks until the mutex is acquired; no timeout or error return. |
| `bool try_lock() noexcept` | Single non-blocking acquire attempt; returns `true` on success, `false` on contention. |
| `void unlock() noexcept` | Releases the mutex; no ownership checks or status reporting. |
| `using mutex = basic_mutex<>` | Convenience alias for `basic_mutex<spin_wait>`. |

## Usage Example
```cpp
#include "castle/sync/mutex.hpp"

int main()
{
    castle::mutex lock;

    if (lock.try_lock())
    {
        lock.unlock();
    }

    lock.lock();
    lock.unlock();

    return 0;
}
```
See `samples/sample_mutex.cpp` for the full sample.

## Constraints & Notes
- `basic_mutex` is non-recursive. Calling `lock()` again from the current holder waits indefinitely.
- `try_lock()` is the only API that reports lock contention directly.
- `unlock()` does not validate ownership; callers must balance successful acquires correctly.
- The implementation uses compiler atomic built-ins directly, with no heap allocation, exceptions, RTTI, virtual functions, or STL synchronization support.
- The default `spin_wait` policy busy-waits. Supply a custom `WaitPolicy` when blocked callers should yield to an RTOS scheduler.
