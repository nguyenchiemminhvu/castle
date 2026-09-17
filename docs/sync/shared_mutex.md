# Shared Mutex

## Overview
`castle::shared_mutex` is a reader/writer mutex with a bounded number of simultaneous readers and exclusive writers. It is useful when read-mostly data should be observed concurrently but all writes must remain serialized.

## Header
`#include "castle/sync/shared_mutex.hpp"`

## Dependencies
- [`compiler.hpp`](../core/compiler.md)
- [`config.hpp`](../core/config.md)
- [`traits.hpp`](../core/traits.md)
- [`wait_policy.hpp`](wait_policy.md)

## Public API
| API | Description |
| --- | --- |
| `template <uint32_t MaxReaders, typename WaitPolicy = spin_wait> class shared_mutex` | Reader/writer mutex with writer preference and bounded concurrent readers. |
| `using wait_policy_type = WaitPolicy` | Exposes the policy type used by the mutex specialization. |
| `shared_mutex()` | Constructs the mutex in the unlocked state. |
| `void lock_read() noexcept` | Blocks until a shared read lock can be acquired. |
| `bool try_lock_read() noexcept` | Single non-blocking read-lock attempt; returns `true` on success. |
| `void unlock_read() noexcept` | Releases one read lock. |
| `void lock_write() noexcept` | Blocks until exclusive write ownership is acquired. |
| `bool try_lock_write() noexcept` | Single non-blocking write-lock attempt; returns `true` on success. |
| `void unlock_write() noexcept` | Releases the write lock. |

## Usage Example
```cpp
#include "castle/sync/shared_mutex.hpp"

int main()
{
    castle::shared_mutex<2> lock;

    lock.lock_read();
    lock.unlock_read();

    lock.lock_write();
    lock.unlock_write();

    return 0;
}
```
See `samples/sample_shared_mutex.cpp` for the full sample.

## Constraints & Notes
- `MaxReaders` must be `31` or less. A value of `0` makes read locking unavailable.
- New readers are blocked while any writer is waiting, which gives writers preference.
- `lock_read()` and `lock_write()` have no timeout or error code; contention is reported only through the `try_` APIs returning `false`.
- The mutex is non-recursive and does not track reader or writer ownership. Unbalanced `unlock_read()` or `unlock_write()` calls corrupt state.
- The implementation uses compiler atomic built-ins directly, with no heap allocation, exceptions, RTTI, virtual functions, or STL synchronization support.
- The default `spin_wait` policy busy-waits; provide a custom `WaitPolicy` when blocked callers should yield to a scheduler.
