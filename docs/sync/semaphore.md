# Semaphore

## Overview

Provides a bounded, STL-free counting semaphore for embedded code. A semaphore gates access to a pool of up to `MaxCount` interchangeable units (buffers, worker slots, hardware channels, ...): `acquire()` takes one unit, blocking while none are available, and `release()` returns one or more units, up to the compile-time capacity.

The maximum number of permits is specified at compile time through the `MaxCount` template parameter. What happens on each retry of a blocked `acquire()` is specified at compile time through the `WaitPolicy` template parameter (see [Wait policy](#wait-policy)).

Unlike `castle::mutex`, a semaphore has no notion of ownership: any context may call `release()`, including an interrupt handler that never called `acquire()`. This makes it usable both as a classic counting resource guard and as an ISR-to-task signal, matching the role of Zephyr's `k_sem` or a POSIX `sem_t`.

The implementation uses GCC/Clang atomic built-in functions directly and requires no heap allocation, virtual functions, RTTI, exceptions, or STL dependency.

## Header

```cpp
#include "castle/sync/semaphore.hpp"
```

**Namespace:** `castle`

## Main API

| API                     | Purpose                                                                                     |
| ------------------------ | -------------------------------------------------------------------------------------------- |
| `semaphore<MaxCount, WaitPolicy = spin_wait>` | Counting semaphore with a compile-time maximum permit count and a pluggable blocked-retry policy. |
| `acquire()`              | Blocks until a permit can be taken.                                                         |
| `try_acquire()`          | Attempts to take a permit without blocking.                                                 |
| `release(update = 1)`    | Returns `update` permit(s); fails without side effects if that would exceed `MaxCount`.      |
| `count()`                | Returns a snapshot of the number of permits currently available.                            |
| `max()`                  | Returns the compile-time capacity (`MaxCount`).                                              |
| `wait_policy_type`       | The `WaitPolicy` the semaphore was instantiated with.                                       |
| `binary_semaphore`       | Alias for `semaphore<1>`; a single-permit on/off signal.                                    |
| `spin_wait`              | Default `WaitPolicy`: a portable CPU-relax hint (see `castle/sync/wait_policy.hpp`).        |

## Permit capacity

`MaxCount` specifies the maximum number of permits the semaphore can ever hold.

```cpp
castle::semaphore<4> resource_slots(4U);
```

The semaphore allows up to four interchangeable resource slots to be acquired concurrently:

```text
Worker 1 ────────────────┐
Worker 2 ────────────────┤
Worker 3 ────────────────┤── Resource pool (4 slots)
Worker 4 ────────────────┘
```

A fifth worker cannot acquire a slot until one of the existing holders releases it.

The capacity is known at compile time and requires no dynamically allocated bookkeeping.

## Construction

The constructor takes the initial permit count explicitly; there is no default, so the starting state is always a deliberate choice at the call site.

```cpp
castle::semaphore<4> full(4U);   // fully available: 4 permits to hand out
castle::semaphore<4> empty(0U);  // start empty: workers block until release()
```

An `initial_count` greater than `MaxCount` is clamped to `MaxCount` rather than invoking undefined behavior.

## Acquiring and releasing

Use `acquire()` to take a permit, blocking (busy-waiting) while none are available:

```cpp
resource_slots.acquire();

// Use one of the resource slots.

CASTLE_SAMPLE_CHECK(resource_slots.release());
```

`try_acquire()` provides the non-blocking equivalent:

```cpp
if (resource_slots.try_acquire())
{
    // Use one of the resource slots.

    resource_slots.release();
}
```

`release()` returns `true` when the permit(s) were returned successfully. It returns `false`, leaving the count unchanged, if returning `update` permits would raise the count above `MaxCount`:

```cpp
if (!resource_slots.release())
{
    // Programming error: released more permits than were ever acquired.
}
```

This fail-fast behavior avoids both silent saturation and undefined behavior, matching castle's safety-critical design goals. `release()` is marked `[[nodiscard]]` so a caller cannot silently ignore this failure.

## Wait policy

`acquire()` (and the rare CAS-contention retry inside `release()`) is a busy-wait loop: on every failed retry it calls `WaitPolicy::wait()`, a compile-time customization point defined in `castle/sync/wait_policy.hpp`.

### Why not a real, Zephyr-`k_sem`-style blocked task queue?

Waking a descheduled task from a wait queue (as `k_sem_take`/`sem_wait` do) requires an actual kernel: a scheduler that owns task state, stacks, and a wait queue, so it can take a task off the CPU and put another one on. `castle::semaphore` is a header-only, no-heap, no-virtual-function, freestanding template — it has no kernel to own, so it cannot implement thread scheduling by itself. Building one would mean either silently depending on an OS (violating castle's zero-OS-dependency, bare-metal-first design) or reimplementing a scheduler inside a template library, which is out of scope and unsafe to get right generically across every target RTOS/bare-metal environment.

### What castle provides instead

A single, type-checked extension point. Any type providing a callable

```cpp
static void wait() noexcept;
```

can be supplied as `WaitPolicy`. It runs once per failed retry, so an RTOS integration can yield to its *real* scheduler instead of spinning:

```cpp
#include "castle/sync/semaphore.hpp"

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

castle::semaphore<4, zephyr_wait_policy> resource_slots(4U);
```

### Default: `castle::spin_wait`

Without a second template argument, `semaphore` uses `castle::spin_wait`, which hints the CPU to reduce power/bus-contention cost while spinning (`PAUSE` on x86, `YIELD` on Arm, a no-op compiler barrier elsewhere) — never blocking, never touching an OS, and remaining fully bare-metal safe.

### Compile-time safety

`WaitPolicy` is checked with a `static_assert` at the point of instantiation: a type without a matching, `noexcept`, callable, static `wait()` fails to compile with a clear diagnostic rather than a wall of template-instantiation errors.

## Binary semaphore

`castle::binary_semaphore` is `semaphore<1>` and is convenient as an unowned, ISR-safe signal:

```cpp
castle::binary_semaphore data_ready(0U);

// Interrupt context:
data_ready.release();

// Task context:
data_ready.acquire();
// Process the data made available by the interrupt.
```

Because a semaphore has no owning-thread requirement, `release()` can safely be called from a context that never called `acquire()` — something `castle::mutex` explicitly disallows.

## RAII guard

`castle::scoped_semaphore<MaxCount, WaitPolicy>` (in `castle/sync/scoped_semaphore.hpp`) acquires a permit at construction and returns it at destruction, mirroring `castle::scoped_mutex`:

```cpp
#include "castle/sync/scoped_semaphore.hpp"

castle::semaphore<2> pool(2U);

{
    castle::scoped_semaphore<2> guard(pool);

    // One of up to 2 permits is held here.
}
// Permit automatically returned here.
```

## Example

```cpp
castle::semaphore<4> resource_slots(4U);

void use_resource_slot()
{
    resource_slots.acquire();

    // Access one of the up to 4 interchangeable resource slots.

    resource_slots.release();
}
```

Up to four calls to `use_resource_slot()` may hold a slot concurrently; a fifth blocks until one is released.

## Embedded notes

* The semaphore owns all of its state; there is no heap allocation.
* `MaxCount` is a compile-time limit and should be selected according to the maximum number of simultaneous permit holders expected by the application.
* The implementation uses GCC/Clang `__sync_*` atomic built-ins rather than relying on `std::counting_semaphore` or other STL facilities.
* The semaphore is non-copyable and non-movable.
* The implementation does not use virtual functions, RTTI, exceptions, or STL.
* `acquire()` is a busy-waiting operation by default and therefore consumes CPU time while blocked; supply a custom `WaitPolicy` to hand control to a real scheduler instead (see [Wait policy](#wait-policy)).
* Spinlocks/spin-waits are most appropriate when hold times are short and bounded; avoid long-running operations, blocking I/O, or unbounded-latency work while holding a permit.
* `release()` never invokes undefined behavior: it fails and leaves the count unchanged rather than exceeding `MaxCount`.
* Unlike `castle::mutex`, there is no owner tracking — any context, including an ISR, may call `release()`.
* `MaxCount` must be greater than zero.
* `WaitPolicy` must provide a public, callable, `noexcept` `static void wait()`; this is enforced with a `static_assert`.
* For deterministic embedded systems, the worst-case spinning time in `acquire()` should be considered as part of the system timing analysis.

