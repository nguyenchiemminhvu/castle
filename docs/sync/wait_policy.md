# Wait policy

## Overview

`castle::spin_wait` is the default `WaitPolicy` shared by every castle spinlock-based synchronization primitive: `castle::mutex`, `castle::recursive_mutex`, `castle::shared_mutex`, and `castle::semaphore` (and their RAII guards, `castle::scoped_mutex` / `castle::scoped_semaphore`). `WaitPolicy` is castle's compile-time customization point for what happens on each failed retry of a blocked wait loop: any type providing a callable, `noexcept`, static `wait()` function can be substituted, letting an RTOS integration hand control to its real scheduler on each retry instead of spinning.

### Why not real thread scheduling?

A Zephyr-`k_sem`/`k_mutex`-style blocked task, parked on a wait queue and woken by an interrupt or unlock/release, requires an actual kernel: a scheduler owning task state, stacks, and a wait queue, so it can take a task off the CPU and put another one on. These primitives are header-only, allocate no heap memory, and use no virtual functions — they have no kernel to own, so they cannot implement thread scheduling by themselves. `WaitPolicy` is castle's answer: a single, type-checked seam where an integrator's *own* scheduler primitive can be plugged in, without changing any call site in application code.

## Header

```cpp
#include "castle/sync/wait_policy.hpp"
```

**Namespace:** `castle`

## Main API

| API                | Purpose                                                                          |
| -------------------- | ----------------------------------------------------------------------------------- |
| `spin_wait`         | Default `WaitPolicy`: a portable CPU-relax hint, never blocking or touching an OS. |
| `spin_wait::wait()` | Hints the CPU to reduce power/bus-contention cost while spinning.                |

## The WaitPolicy contract

Any type used as a `WaitPolicy` (e.g. for `castle::semaphore<MaxCount, WaitPolicy>`, `castle::basic_mutex<WaitPolicy>`, `castle::basic_recursive_mutex<WaitPolicy>`, or `castle::shared_mutex<MaxReaders, WaitPolicy>`) must provide:

```cpp
static void wait() noexcept;
```

Each of these primitives checks this with a `static_assert` at the point of instantiation, so an incompatible policy type fails to compile with a precise diagnostic instead of a wall of template-instantiation errors.

`wait()` is invoked once per failed retry of a busy-wait loop (e.g. `semaphore::acquire()` finding no permit available, `mutex::lock()` finding the lock held, or losing a compare-and-swap race to a concurrent caller).

## Default: `castle::spin_wait`

```cpp
castle::mutex lock;                      // basic_mutex<spin_wait>
castle::semaphore<4> resource_slots(4U); // semaphore<4, spin_wait>
```

`spin_wait::wait()` expands to a `PAUSE` instruction on x86, a `YIELD` hint on Arm, or a no-op compiler barrier on unrecognized architectures. It never blocks, never yields to an OS, and never changes observable program state — matching castle's bare-metal-first, zero-OS-dependency design.

## Plugging in a real scheduler

```cpp
// Zephyr:
struct zephyr_wait_policy
{
    static void wait() noexcept { k_yield(); }
};

// FreeRTOS:
struct freertos_wait_policy
{
    static void wait() noexcept { taskYIELD(); }
};

castle::basic_mutex<zephyr_wait_policy> lock;
castle::semaphore<4, zephyr_wait_policy> resource_slots(4U);
castle::shared_mutex<4, freertos_wait_policy> resource_mutex;
castle::basic_recursive_mutex<freertos_wait_policy> reentrant_lock;
```

Because `WaitPolicy` is a template parameter, this substitution is resolved entirely at compile time: it costs nothing when the default `spin_wait` is used, and is fully inlined when a custom policy is supplied.

## Example

```cpp
#include "castle/sync/semaphore.hpp"

struct counting_wait_policy
{
    static void wait() noexcept { /* count retries, log, yield, ... */ }
};

castle::semaphore<1, counting_wait_policy> single_slot(1U);
```

## Embedded notes

* `spin_wait` is stateless, has no members, and requires no heap allocation.
* A custom `WaitPolicy` may hold its own `static` state (e.g. retry counters) since each policy type is a distinct type; it must not, however, change the guarded primitive's own state.
* `WaitPolicy::wait()` must be `noexcept`: castle's synchronization primitives never throw.
* Supplying a policy that blocks indefinitely without ever allowing the guarded condition to become true will deadlock the caller, exactly as an unconditional spin would.
* The implementation does not use virtual functions, RTTI, exceptions, or STL.
