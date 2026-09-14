# pthread_pool

## Overview

`castle::events::pthread_pool` is a fixed-capacity thread pool implemented on top of POSIX threads.

The implementation follows the CASTLE design goals for embedded systems:

- no dynamic memory allocation by CASTLE;
- no exceptions;
- no RTTI;
- no STL dependency;
- deterministic storage requirements;
- fixed worker-thread count;
- fixed pending-task capacity;
- task execution outside the queue mutex.

The pool is available only when `CASTLE_USING_PTHREAD` is enabled.

## Header

```cpp
#include "castle/events/pthread_pool.hpp"
```

## Namespace

```cpp
namespace castle
{
namespace events
{
class pthread_pool;
}
}
```

## Declaration

```cpp
template <castle::size_type ThreadCount, castle::size_type PendingTaskCount>
class pthread_pool;
```

`ThreadCount` and `PendingTaskCount` must both be greater than zero.

## Storage model

The pool owns all storage required for workers and tasks:

```text
pthread_pool
|
+-- pthread_t[ThreadCount]
|
+-- task_type[PendingTaskCount]
|
+-- pending queue of task indices
|
+-- free queue of task indices
|
+-- pthread mutex
|
+-- pthread condition variable
```

A task is stored as `castle::callbacks::inplace_function<void()>`. The queues contain only task-slot indices. This is intentional because `castle::container::ring_buffer` is constrained to trivially copyable and trivially destructible element types, while `inplace_function` owns callable state.

The number of task slots is therefore exactly `PendingTaskCount`, and a submitted task consumes one slot until a worker moves it into its local task object.

## Task type

```cpp
using task_type = castle::callbacks::inplace_function<void()>;
```

A submitted task must:

- be callable with no arguments;
- return exactly `void`;
- fit inside the configured `inplace_function` storage.

The callable may capture its required state, so no separate task argument is necessary.

## Constructor

```cpp
pthread_pool() CASTLE_NOEXCEPT;
```

Constructs and starts `ThreadCount` worker threads.

The constructor does not allocate heap memory. POSIX thread creation itself is delegated to the platform implementation.

Construction is intended to be used as a fixed-lifetime object. Copy and move construction are disabled because worker threads retain a pointer to the pool object.

## Destructor

```cpp
~pthread_pool();
```

Stops the pool and destroys the synchronization primitives.

Already accepted tasks are drained before worker threads are joined. A task that is currently executing is allowed to complete.

## submit

```cpp
CASTLE_NODISCARD bool submit(task_type&& task) CASTLE_NOEXCEPT;

template <typename Callable>
CASTLE_NODISCARD bool submit(Callable&& callable) CASTLE_NOEXCEPT;
```

Adds a task to the pending queue.

The callable overload checks at compile time that the callable is invocable with no arguments and returns `void`.

Returns `false` when:

- the task is empty;
- the pool has been stopped or is stopping;
- no task slot is available;
- the pending queue is full;
- a pthread synchronization operation fails.

Submission does not wait for a free queue slot. This is important for deterministic embedded behavior: queue capacity is a hard bound rather than a hidden blocking resource.

### Example

```cpp
castle::events::pthread_pool<2U, 8U> pool;

int value = 0;

pool.submit([&value]() {
value++;
});
```

## stop

```cpp
CASTLE_NODISCARD bool stop() CASTLE_NOEXCEPT;
```

Requests shutdown and joins all worker threads.

Shutdown semantics are:

1. reject new submissions;
2. wake all workers;
3. allow already queued work to be consumed;
4. allow an already executing task to finish;
5. join every worker thread.

Calling `stop()` again is valid and returns `true` after the pool has already stopped.

## running

```cpp
CASTLE_NODISCARD bool running() CASTLE_CONST CASTLE_NOEXCEPT;
```

Returns `true` while the pool is accepting work and its worker set is active.

After shutdown has been requested, `running()` returns `false` even while worker threads are finishing already accepted tasks.

## queued

```cpp
CASTLE_NODISCARD castle::size_type queued() CASTLE_CONST CASTLE_NOEXCEPT;
```

Returns the number of tasks currently waiting in the pending queue.

A task being executed by a worker is not counted as queued.

## available

```cpp
CASTLE_NODISCARD castle::size_type available() CASTLE_CONST CASTLE_NOEXCEPT;
```

Returns the number of task slots currently available for submission.

The invariant is:

```text
queued() + executing task slots + available() == PendingTaskCount
```

The task slot moves to the free queue as soon as the worker takes ownership of the task, so an executing task does not consume queue capacity.

## Compile-time capacity queries

```cpp
static CASTLE_CONSTEXPR castle::size_type thread_count() CASTLE_NOEXCEPT;
static CASTLE_CONSTEXPR castle::size_type task_capacity() CASTLE_NOEXCEPT;
```

These return the template configuration without accessing runtime state.

Example:

```cpp
static_assert(
castle::events::pthread_pool<4U, 16U>::thread_count() == 4U,
"Unexpected worker count"
);

static_assert(
castle::events::pthread_pool<4U, 16U>::task_capacity() == 16U,
"Unexpected task capacity"
);
```

## Concurrency model

`submit()`, `stop()`, `running()`, `queued()`, and `available()` synchronize access to shared queue state with a pthread mutex.

Workers sleep on a pthread condition variable while no task is pending. A successful submission signals a worker, while shutdown broadcasts to all workers.

The callback is moved out of the shared task storage before releasing the mutex:

```text
lock
  pop task index
  move task into worker-local task
  release task slot
unlock
execute callback
```

This keeps callback execution independent of the queue lock and prevents user code from running while the pool mutex is held.

## Lifetime requirements

The pool object must remain alive until all worker threads have terminated. This is naturally satisfied when the pool is an automatic object because its destructor calls `stop()`.

A callback must also obey the normal C++ lifetime requirements of any captured object. The pool does not extend the lifetime of captured references or pointers.

## Error handling

The implementation does not throw exceptions.

Operations that can fail at runtime return `bool` where the failure is meaningful to the caller. Synchronization failures are treated as operation failures rather than being silently ignored.

The constructor is `noexcept`; platform-level failures during initialization result in an incompletely started internal state that is cleaned up by the implementation.

## Determinism

For a pool declared as:

```cpp
castle::events::pthread_pool<4U, 32U> pool;
```

the CASTLE-owned storage requirements are fixed at compile time.

The pool does not grow the task queue, allocate task objects, allocate worker arrays, or create additional worker threads after construction.

Task completion order is intentionally not deterministic because multiple pthread workers may execute tasks concurrently. Queue admission order is FIFO for the pending task indices.

## Configuration

The header requires:

```cpp
#define CASTLE_USING_PTHREAD 1
```

when pthread support is intended to be compiled in.

When pthread support is disabled, including this header is a configuration error.

## Constraints and non-goals

`pthread_pool` is intentionally a small fixed-capacity execution primitive. It does not provide:

- futures or promises;
- task return values;
- dynamic task allocation;
- dynamic worker creation;
- task cancellation;
- priority scheduling;
- timed submission;
- a blocking submit operation.

Applications that need these behaviors should build the additional policy outside the core pool rather than adding hidden runtime resources to this component.

## Example

```cpp
#include "castle/events/pthread_pool.hpp"

void process_sensor_sample();

int main()
{
castle::events::pthread_pool<2U, 8U> pool;

pool.submit([]() {
process_sensor_sample();
});

pool.stop();
return 0;
}
```
