# Infinite Loop

## Overview
`castle::infinite_loop` enters a non-returning loop that delegates every iteration to a compile-time wait policy. The policy lets an application yield to its scheduler, block a task, or enter a platform-specific low-power wait state without adding operating-system dependencies to CASTLE.

## Header
`#include "castle/utility/inf_loop.hpp"`

## Dependencies
- [`wait_policy.hpp`](../../include/castle/sync/wait_policy.hpp) — provides the `static void wait() noexcept` policy contract and `castle::spin_wait`.

## Public API
| API | Description |
|---|---|
| `template <typename WaitPolicy> [[noreturn]] void infinite_loop() noexcept` | Repeatedly invokes `WaitPolicy::wait()` and never returns. `WaitPolicy` must satisfy `castle::detail::has_static_wait`. |

### Template parameter
- `WaitPolicy`: a type with a public static `void wait() noexcept` function. It must implement the platform-specific scheduling, blocking, or low-power behavior desired by the application.

### Complexity
The loop performs one policy call per iteration. Its runtime and CPU usage are determined by the selected policy; the wrapper has constant additional storage and no dynamic allocation.

## Usage Example
```cpp
#include "castle/utility/inf_loop.hpp"

struct task_wait_policy
{
    static void wait() noexcept
    {
        // Replace with the RTOS's task-yield, event-wait, or suspend primitive.
        platform_task_wait();
    }
};

void park_current_task() noexcept
{
    castle::infinite_loop<task_wait_policy>();
}
```

See `samples/castle_ext/utility/inf_loop.cpp` for a standalone executable example.

## Constraints & Notes
- The function is marked `[[noreturn]]`; it has no normal exit path.
- The wait policy is mandatory so the library never silently chooses a busy-spin implementation. A no-op policy degenerates into a tight loop and does not satisfy the intended yield/block behavior.
- `castle::spin_wait` may be supplied explicitly for spin-loop behavior, but it does not yield to a scheduler and continues consuming CPU time.
- A plain scheduler-yield operation may immediately make the task runnable again. To avoid consuming CPU while idle, use a policy that blocks/suspends the task or enters an appropriate processor wait state.
- `WaitPolicy::wait()` is called serially on every pass and must itself be `noexcept`. The policy owns any platform-specific synchronization and thread-safety requirements.
- The standalone sample uses `setjmp` only to escape its host-side demonstration after a few wait calls; production embedded code should use the platform scheduler or wait primitive instead.
- The component uses no heap allocation, exceptions, RTTI, virtual functions, or STL facilities.
