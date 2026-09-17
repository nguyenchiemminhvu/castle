# Scoped mutex

## Overview

RAII guard that locks a `castle::basic_mutex<WaitPolicy>` at construction and unlocks it at scope exit, mirroring `castle::scoped_semaphore`.

## Header

```cpp
#include "castle/sync/scoped_mutex.hpp"
```

**Namespace:** `castle`

## Main API

| API                                        | Purpose                                                        |
| --------------------------------------------- | ------------------------------------------------------------------ |
| `basic_scoped_mutex<WaitPolicy = spin_wait>` | RAII guard for a `castle::basic_mutex<WaitPolicy>`.             |
| Constructor                                | Blocks (busy-waits) until the lock is acquired.                |
| Destructor                                 | Releases the lock automatically.                                |
| `scoped_mutex`                             | Alias for `basic_scoped_mutex<>`, guarding a `castle::mutex`.  |

`WaitPolicy` must match the guarded mutex's own `WaitPolicy`; see [Mutex's wait policy](mutex.md#wait-policy) for the RTOS-integration extension point this enables.

## Example

```cpp
castle::mutex lock;

{
    castle::scoped_mutex guard(lock);

    // protected work
}
// mutex automatically released here
```

A custom `WaitPolicy` is passed through the same way as on the underlying `castle::basic_mutex`:

```cpp
struct freertos_wait_policy
{
    static void wait() noexcept { taskYIELD(); }
};

castle::basic_mutex<freertos_wait_policy> lock;
castle::basic_scoped_mutex<freertos_wait_policy> guard(lock);
```

## Embedded notes

- RAII is local and deterministic here: there is no allocation.
- The constructor blocks (busy-waits, or defers to a custom `WaitPolicy`) the same way `basic_mutex::lock()` does; use short critical sections, especially around interrupt/shared-state code.
- Non-copyable and non-movable, matching the underlying `castle::basic_mutex`.
