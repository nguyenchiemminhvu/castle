# Scoped Semaphore

## Overview
`castle::scoped_semaphore` is an RAII guard for `castle::semaphore`. It acquires exactly one permit in its constructor and returns exactly one permit in its destructor, making scoped permit management deterministic and hard to forget.

## Header
`#include "castle/sync/scoped_semaphore.hpp"`

## Dependencies
- [`semaphore.hpp`](semaphore.md)

## Public API
| API | Description |
| --- | --- |
| `template <uint32_t MaxCount, typename WaitPolicy = spin_wait> class scoped_semaphore` | RAII wrapper around `semaphore<MaxCount, WaitPolicy>`. |
| `explicit scoped_semaphore(semaphore<MaxCount, WaitPolicy>& sem) noexcept` | Acquires one permit from `sem`; no timeout or error code. |
| `~scoped_semaphore() noexcept` | Returns the held permit automatically. |

## Usage Example
```cpp
#include "castle/sync/scoped_semaphore.hpp"

int main()
{
    castle::semaphore<2> pool(2U);

    {
        castle::scoped_semaphore<2> guard(pool);
        (void)guard;
    }

    return 0;
}
```
See `samples/sample_scoped_semaphore.cpp` for the full sample.

## Constraints & Notes
- Construction directly calls `semaphore::acquire()`, so guard creation waits while no permit is available.
- The guard always manages one permit and is non-copyable and non-movable.
- The guard must not outlive the referenced semaphore.
- The destructor ignores the boolean result from `release()` because it only returns the single permit acquired by the constructor.
- The type adds no heap allocation, exceptions, RTTI, virtual functions, or STL synchronization support.
