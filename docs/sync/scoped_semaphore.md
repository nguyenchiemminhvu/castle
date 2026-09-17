# Scoped semaphore

## Overview

RAII guard that acquires one permit from a `castle::semaphore<MaxCount, WaitPolicy>` at construction and returns it at destruction. Mirrors `castle::scoped_mutex`, applied to a semaphore's permit pool instead of a single lock.

## Header

```cpp
#include "castle/sync/scoped_semaphore.hpp"
```

**Namespace:** `castle`

## Main API

| API                                          | Purpose                                                        |
| ---------------------------------------------- | ------------------------------------------------------------------ |
| `scoped_semaphore<MaxCount, WaitPolicy = spin_wait>` | RAII guard for a `castle::semaphore<MaxCount, WaitPolicy>`.  |
| Constructor                                  | Blocks (busy-waits) until a permit is acquired.                |
| Destructor                                   | Returns the permit automatically.                              |

`WaitPolicy` must match the guarded semaphore's own `WaitPolicy`; see [Semaphore's wait policy](semaphore.md#wait-policy) for the RTOS-integration extension point this enables.

## Example

```cpp
castle::semaphore<2> pool(2U);

{
    castle::scoped_semaphore<2> guard(pool);

    // One of up to 2 permits is held here.
}
// Permit automatically returned here.
```

A custom `WaitPolicy` is passed through the same way as on the underlying `castle::semaphore`:

```cpp
struct freertos_wait_policy
{
    static void wait() noexcept { taskYIELD(); }
};

castle::semaphore<2, freertos_wait_policy> pool(2U);
castle::scoped_semaphore<2, freertos_wait_policy> guard(pool);
```

## Embedded notes

* RAII is local and deterministic here: there is no allocation.
* The constructor blocks (busy-waits, or defers to a custom `WaitPolicy`) the same way `semaphore::acquire()` does; use short critical sections.
* Non-copyable and non-movable, matching the underlying `castle::semaphore`.
* The destructor always returns exactly the one permit acquired by the constructor, so it can never exceed the semaphore's `MaxCount`.

