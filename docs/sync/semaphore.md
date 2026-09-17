# Semaphore

## Overview
`castle::semaphore` is a bounded counting semaphore for fixed-capacity resource pools and ownership-free signaling. It lets callers acquire permits, try to acquire without blocking, release permits back, and inspect the current permit snapshot without bringing in an OS semaphore or STL support.

## Header
`#include "castle/sync/semaphore.hpp"`

## Dependencies
- [`compiler.hpp`](../core/compiler.md)
- [`wait_policy.hpp`](wait_policy.md)

## Public API
| API | Description |
| --- | --- |
| `template <uint32_t MaxCount, typename WaitPolicy = spin_wait> class semaphore` | Counting semaphore with compile-time capacity and wait policy. |
| `using value_type = uint32_t` | Permit-count type. |
| `using wait_policy_type = WaitPolicy` | Exposes the policy type used by the semaphore specialization. |
| `explicit semaphore(value_type initial_count) noexcept` | Constructs the semaphore; `initial_count` is clamped to `MaxCount`. |
| `void acquire() noexcept` | Blocks until one permit is available, then consumes it. |
| `bool try_acquire() noexcept` | Single non-blocking acquire attempt; returns `true` on success, `false` when no permit is available. |
| `bool release(value_type update = 1U) noexcept` | Returns `update` permits; returns `false` and leaves the count unchanged if that would exceed `MaxCount`. |
| `value_type count() noexcept` | Returns the current permit snapshot. |
| `static constexpr value_type max() noexcept` | Returns `MaxCount`. |
| `using binary_semaphore = semaphore<1U>` | Single-permit semaphore useful as a signal. |

## Usage Example
```cpp
#include "castle/sync/semaphore.hpp"

int main()
{
    castle::semaphore<2> slots(2U);

    slots.acquire();
    slots.release();

    return 0;
}
```
See `samples/sample_semaphore.cpp` for the full sample.

## Constraints & Notes
- `MaxCount` must be greater than `0`.
- `acquire()` has no timeout or error code; it waits until another context returns a permit.
- `release()` is the only API that reports permit overflow, so callers must check its boolean result.
- Semaphore permits are ownership-free: a context may call `release()` even if it never called `acquire()`.
- `count()` is only a snapshot and may become stale immediately.
- The implementation uses compiler atomic built-ins directly, with no heap allocation, exceptions, RTTI, virtual functions, or STL synchronization support.
- The default `spin_wait` policy busy-waits; provide a custom `WaitPolicy` when blocked callers should yield to a scheduler.
