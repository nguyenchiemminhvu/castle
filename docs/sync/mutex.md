# Mutex

## Overview

A basic spinlock-based mutex for embedded code that needs mutual exclusion without pulling in STL threading facilities, heap allocation, virtual dispatch, RTTI, or exceptions.

What happens on each retry of a blocked `lock()` is specified at compile time through the `WaitPolicy` template parameter (see [Wait policy](#wait-policy)).

The implementation uses GCC/Clang atomic built-in functions (`__sync_lock_test_and_set` / `__sync_lock_release`) directly.

## Header

```cpp
#include "castle/sync/mutex.hpp"
```

**Namespace:** `castle`

## Main API

| API                                     | Purpose                                                                          |
| ------------------------------------------ | ------------------------------------------------------------------------------------ |
| `basic_mutex<WaitPolicy = spin_wait>`   | Spinlock mutex with a pluggable blocked-retry policy.                           |
| `lock()`                                | Blocks (busy-waits) until the lock becomes available.                          |
| `try_lock()`                            | Attempts to acquire the lock without blocking.                                 |
| `unlock()`                              | Releases the lock.                                                              |
| `wait_policy_type`                      | The `WaitPolicy` the mutex was instantiated with.                               |
| `mutex`                                 | Alias for `basic_mutex<>`, i.e. `basic_mutex<spin_wait>`.                       |

## Example

```cpp
castle::mutex lock;

lock.lock();
// protected access
lock.unlock();
```

`try_lock()` provides the non-blocking equivalent:

```cpp
if (lock.try_lock())
{
    // protected access
    lock.unlock();
}
```

## Wait policy

`lock()` is a busy-wait loop: on every failed retry it calls `WaitPolicy::wait()`, a compile-time customization point defined in `castle/sync/wait_policy.hpp` and shared by every castle spinlock-based primitive (`castle::mutex`, `castle::recursive_mutex`, `castle::shared_mutex`, `castle::semaphore`).

Waking a descheduled task from a wait queue (as `k_mutex_lock`/`pthread_mutex_lock` do) requires an actual kernel owning task state, stacks, and a wait queue — out of scope for a header-only, no-heap, no-virtual-function, freestanding template. Instead, castle provides a single, type-checked extension point: any type with a callable, `noexcept`, static `wait()` can be supplied as `WaitPolicy`, letting an RTOS integration yield to its real scheduler on each retry instead of spinning:

```cpp
struct freertos_wait_policy
{
    static void wait() noexcept { taskYIELD(); }
};

castle::basic_mutex<freertos_wait_policy> lock;
```

Without a template argument, `castle::mutex` uses `castle::spin_wait`, a portable CPU-relax hint (`PAUSE` on x86, `YIELD` on Arm, a no-op compiler barrier elsewhere) — never blocking, never touching an OS. See [Wait policy](wait_policy.md) for the full rationale and contract.

`WaitPolicy` is checked with a `static_assert` at the point of instantiation, so an incompatible policy type fails to compile with a clear diagnostic.

## Embedded notes

* The mutex owns all of its state; there is no heap allocation.
* The implementation uses GCC/Clang `__sync_*` atomic built-ins rather than relying on `std::mutex` or other STL facilities.
* The mutex is non-copyable and non-movable.
* The implementation does not use virtual functions, RTTI, exceptions, or STL.
* `lock()` is a busy-waiting operation by default and therefore consumes CPU time while blocked; supply a custom `WaitPolicy` to hand control to a real scheduler instead.
* Use `castle::scoped_mutex` to make unlock-on-exit harder to forget.
* For deterministic embedded systems, the worst-case spinning time in `lock()` should be considered as part of the system timing analysis.
