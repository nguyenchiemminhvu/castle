# Wait Policy

## Overview
Castle wait policies define what a blocking synchronization primitive does after one failed retry. The default policy, `castle::spin_wait`, issues only a CPU spin hint, while a custom policy can yield into an RTOS scheduler without changing user code at each call site.

## Header
`#include "castle/sync/wait_policy.hpp"`

## Dependencies
- [`compiler.hpp`](../core/compiler.md)
- [`traits.hpp`](../core/traits.md)

## Public API
| API | Description |
| --- | --- |
| `struct spin_wait` | Default wait policy used by Castle synchronization primitives. |
| `static void spin_wait::wait() noexcept` | Issues one architecture-specific spin hint through `CASTLE_CPU_RELAX()`. |

## Usage Example
```cpp
#include "castle/sync/mutex.hpp"

struct scheduler_wait_policy
{
    static void wait() noexcept
    {
        /* call the platform scheduler here */
    }
};

int main()
{
    castle::basic_mutex<scheduler_wait_policy> lock;
    return lock.try_lock() ? (lock.unlock(), 0) : 0;
}
```
See `samples/sample_wait_policy.cpp` for the full sample.

## Constraints & Notes
- A wait policy must provide `static void wait() noexcept;`.
- Compatibility is checked at compile time by the consuming synchronization primitive.
- `spin_wait::wait()` never blocks on an OS primitive by itself; it only reduces the cost of busy-waiting.
- Wait policies report no runtime status and must not throw exceptions.
- The wait-policy mechanism is allocation-free, deterministic, and independent of RTTI, virtual functions, and STL threading support.
