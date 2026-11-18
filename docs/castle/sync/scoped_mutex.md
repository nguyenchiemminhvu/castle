# Scoped Mutex

## Overview
`castle::basic_scoped_mutex` is an RAII guard for `castle::basic_mutex`. It acquires the referenced mutex in its constructor and releases it in its destructor so scope exit automatically unlocks the critical section.

## Header
`#include "castle/sync/scoped_mutex.hpp"`

## Dependencies
- [`mutex.hpp`](mutex.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename WaitPolicy = spin_wait> class basic_scoped_mutex` | RAII wrapper around `basic_mutex<WaitPolicy>`. |
| `explicit basic_scoped_mutex(basic_mutex<WaitPolicy>& mutex) noexcept` | Locks `mutex` immediately; no timeout or error code. |
| `~basic_scoped_mutex() noexcept` | Unlocks the guarded mutex automatically. |
| `using scoped_mutex = basic_scoped_mutex<>` | Convenience alias for guarding `castle::mutex`. |

## Usage Example
```cpp
#include "castle/sync/scoped_mutex.hpp"

int main()
{
    castle::mutex lock;

    {
        castle::scoped_mutex guard(lock);
        (void)guard;
    }

    return 0;
}
```
See `samples/sample_scoped_mutex.cpp` for the full sample.

## Constraints & Notes
- Construction directly calls `basic_mutex::lock()`, so guard creation waits while the mutex is busy.
- The guard is non-copyable and non-movable.
- The guard must not outlive the referenced mutex.
- Because the guarded mutex is non-recursive, creating another guard on the same held mutex from the current holder waits indefinitely.
- The type adds no heap allocation, exceptions, RTTI, virtual functions, or STL synchronization support.
