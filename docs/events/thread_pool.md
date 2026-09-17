# thread_pool

## Overview
`castle::events::thread_pool` is a pthread-backed worker pool with compile-time worker count, compile-time pending-task capacity, and inline task storage. It exists for POSIX targets that need bounded asynchronous execution without Castle-managed heap allocation.

## Header
`#include "castle/events/thread_pool.hpp"`

## Dependencies
- [`function`](../callbacks/function.md)
- [`ring_buffer`](../container/ring_buffer.md)

## Public API
| API | Description |
| --- | --- |
| `template <size_type ThreadCount, size_type PendingTaskCount, size_type StorageSize, size_type StorageAlignment> class thread_pool` | Creates `ThreadCount` workers and `PendingTaskCount` task slots during construction. Available only when `CASTLE_USING_PTHREAD` is enabled. |
| `using task_type = callbacks::function<void(), StorageSize, StorageAlignment>` | Inline task wrapper accepted by `submit(task_type&&)`. |
| `thread_pool()` | Initializes queues, mutex/condition variable, and worker threads. If pthread setup fails, the object remains valid but may reject work and report `running() == false`. |
| `~thread_pool()` | Calls `stop()` and destroys synchronization primitives. |
| `bool submit(task_type&&)` | Queues a pre-built task. Returns `false` for empty tasks, stopped pools, full capacity, or pthread failures. |
| `template <typename Callable> bool submit(Callable&&)` | Wraps any `void()` callable into `task_type` and queues it. |
| `bool stop()` | Sets the stop flag, wakes workers, joins created threads, and returns whether shutdown signaling plus joins succeeded. |
| `bool running() const` | Returns whether the pool is initialized and still accepting new work. |
| `size_type queued() const` | Returns the number of tasks still waiting in the pending queue. |
| `size_type available() const` | Returns the number of free task slots currently available for submission. |
| `static constexpr size_type thread_count()` | Returns `ThreadCount`. |
| `static constexpr size_type task_capacity()` | Returns `PendingTaskCount`. |

## Usage Example
See [`samples/sample_thread_pool.cpp`](../../samples/sample_thread_pool.cpp).

```cpp
castle::events::thread_pool<1U, 2U> pool;
volatile int finished = 0;

pool.submit([&finished]() {
    ++finished;
});

while (finished != 1)
{
}

pool.stop();
```

## Constraints & Notes
- Castle-owned storage is fixed at compile time: worker handles, task slots, and queue state are embedded directly in the object.
- `submit()`, `stop()`, `running()`, `queued()`, and `available()` synchronize queue state with a pthread mutex.
- Workers move a task out of shared storage, immediately return that slot to the free queue, unlock the mutex, and only then execute user code.
- A successful `submit()` means the task was accepted into the queue, not that it is guaranteed to finish. If `stop()` is requested before a worker pops a queued task, that queued task is abandoned.
- Tasks already executing when `stop()` runs may complete before the join phase ends.
- Linking executables that use this sample or header typically requires pthread support (for example `-pthread` or `-lpthread`, depending on the toolchain).
