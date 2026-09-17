# Recursive mutex

## Overview

A spinlock-based mutex that the owning thread may lock multiple times. This is
useful when a function that already holds the lock calls another function
(directly, or through recursion) that needs to take the same lock again.

## Header

```cpp
#include "castle/sync/recursive_mutex.hpp"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `basic_recursive_mutex<WaitPolicy = spin_wait>` | Lockable type, re-acquirable by the owning thread, with a pluggable blocked-retry policy. |
| `lock()` | Blocks (busy-waits) until the calling thread owns the lock. |
| `try_lock()` | Non-blocking attempt to acquire or re-acquire the lock. |
| `unlock()` | Releases one level of recursion; must be balanced with `lock()`/`try_lock()`. |
| `wait_policy_type` | The `WaitPolicy` the mutex was instantiated with. |
| `recursive_mutex` | Alias for `basic_recursive_mutex<>`, i.e. `basic_recursive_mutex<spin_wait>`. |

## Example

```cpp
castle::recursive_mutex m;

m.lock();
m.lock();   // same thread, allowed

m.unlock();
m.unlock(); // lock is now free for other threads
```

## Thread identity

`recursive_mutex` needs to know whether the calling thread already owns the
lock, without STL, heap allocation, or OS calls:

- When `CASTLE_USING_PTHREAD` is enabled (see `castle/core/config.hpp`), the
  thread id is `pthread_self()`, compared with `pthread_equal()`.
- Otherwise, the address of a `thread_local` object is used as a per-thread
  identifier: each thread gets its own instance, so the address is unique.

## Wait policy

`lock()`'s outer retry loop (waiting for another thread to release ownership) calls `WaitPolicy::wait()` once per failed retry, the same compile-time customization point used by `castle::mutex`, `castle::shared_mutex`, and `castle::semaphore`. See [Mutex's wait policy](mutex.md#wait-policy) for the full rationale (why real thread scheduling is out of scope for a freestanding template library) and the RTOS-integration contract:

```cpp
struct freertos_wait_policy
{
    static void wait() noexcept { taskYIELD(); }
};

castle::basic_recursive_mutex<freertos_wait_policy> m;
```

Without a template argument, `castle::recursive_mutex` uses `castle::spin_wait`, castle's zero-dependency, bare-metal-safe default.

## Embedded notes

- `lock()` busy-waits by default; there is no blocking/sleeping OS wait involved unless a custom `WaitPolicy` is supplied.
- Calling `unlock()` from a thread that does not own the lock is undefined
  behavior, matching `std::recursive_mutex`.
- Ownership bookkeeping is guarded internally by a plain `castle::basic_mutex<WaitPolicy>`, so
  the type stays free of heap allocation, exceptions, virtual dispatch, and
  RTTI.
