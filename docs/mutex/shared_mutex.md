# Shared mutex

## Overview

Provides a bounded, STL-free reader/writer spinlock for embedded code. Multiple threads may hold a read lock simultaneously, while a write lock is exclusive and prevents all readers and other writers from entering the critical section.

The maximum number of simultaneous readers is specified at compile time through the `MaxReaders` template parameter.

The implementation uses GCC/Clang atomic built-in functions directly and requires no heap allocation, virtual functions, RTTI, exceptions, or STL dependency.

## Header

```cpp
#include "castle/mutex/shared_mutex.hpp"
```

**Namespace:** `castle`

## Main API

| API                        | Purpose                                                                                      |
| -------------------------- | -------------------------------------------------------------------------------------------- |
| `shared_mutex<MaxReaders>` | Bounded reader/writer mutex with a compile-time maximum reader count.                        |
| `lock_read()`              | Blocks until a read lock can be acquired. Multiple readers may hold the lock simultaneously. |
| `unlock_read()`            | Releases a previously acquired read lock.                                                    |
| `try_lock_read()`          | Attempts to acquire a read lock without blocking.                                            |
| `lock_write()`             | Blocks until an exclusive write lock can be acquired.                                        |
| `unlock_write()`           | Releases a previously acquired write lock.                                                   |
| `try_lock_write()`         | Attempts to acquire an exclusive write lock without blocking.                                |

## Reader limit

`MaxReaders` specifies the maximum number of threads that may hold a read lock simultaneously.

```cpp
castle::shared_mutex<4> resource_mutex;
```

The mutex allows up to four concurrent readers:

```text
Reader 1 ────────────────┐
Reader 2 ────────────────┤
Reader 3 ────────────────┤── Shared resource
Reader 4 ────────────────┘
```

A fifth reader cannot acquire the lock until one of the existing readers releases it.

The reader limit is known at compile time and requires no dynamically allocated bookkeeping.

## Read locking

Use `lock_read()` when the protected resource only needs to be observed.

```cpp
resource_mutex.lock_read();

// Read shared resource.

resource_mutex.unlock_read();
```

Multiple threads can execute the read-side critical section concurrently.

`try_lock_read()` provides the non-blocking equivalent:

```cpp
if (resource_mutex.try_lock_read())
{
    // Read shared resource.

    resource_mutex.unlock_read();
}
```

## Write locking

Use `lock_write()` when the protected resource must be modified.

```cpp
resource_mutex.lock_write();

// Modify shared resource.

resource_mutex.unlock_write();
```

A write lock is exclusive. While a writer owns the mutex:

* no other writer can enter;
* no reader can enter;
* the writer has exclusive access to the protected resource.

The non-blocking form is:

```cpp
if (resource_mutex.try_lock_write())
{
    // Modify shared resource.

    resource_mutex.unlock_write();
}
```

## Lock state

The implementation stores the lock state in a single 32-bit word.

```text
31                         0
+----+----------------------+
| W  |     reader count     |
+----+----------------------+
```

The most significant bit represents writer ownership.

The remaining bits represent the number of active readers.

For example:

```text
0x00000000    Unlocked
0x00000001    One active reader
0x00000002    Two active readers
0x00000004    Four active readers
0x80000000    Writer owns the lock
```

Reader acquisition uses an atomic compare-and-swap operation so that a reader cannot enter concurrently with a writer acquiring the lock.

## Writer preference

The implementation uses writer preference to prevent continuous reader admission from starving a waiting writer.

When a writer begins waiting, it increments the waiting-writer count. New readers are then prevented from entering until the waiting writer has acquired and released the write lock.

Existing readers are allowed to finish normally:

```text
Reader ────────────────┐
Reader ────────────────┤
Reader ────────────────┤
                       └── readers drain
                              │
                              ▼
                           Writer
                              │
                              ▼
                         new readers
```

This does not provide a strict real-time scheduling guarantee. Thread scheduling, interrupt latency, preemption, and the target platform's execution environment still determine how long a waiter may actually take to acquire the lock.

## Example

```cpp
castle::shared_mutex<4> resource_mutex;

void read_resource()
{
    resource_mutex.lock_read();

    // Access resource read-only.

    resource_mutex.unlock_read();
}

void write_resource()
{
    resource_mutex.lock_write();

    // Modify resource.

    resource_mutex.unlock_write();
}
```

Multiple calls to `read_resource()` may execute concurrently, while `write_resource()` requires exclusive access.

## Embedded notes

* The mutex owns all of its state; there is no heap allocation.
* `MaxReaders` is a compile-time limit and should be selected according to the maximum number of simultaneous readers expected by the application.
* The implementation uses GCC/Clang `__sync_*` atomic built-ins rather than relying on `std::atomic` or other STL facilities.
* The mutex is non-copyable and non-movable.
* The implementation does not use virtual functions, RTTI, exceptions, or STL.
* `lock_read()` and `lock_write()` are busy-waiting operations and therefore consume CPU time while blocked.
* Spinlocks are most appropriate when lock hold times are short and bounded.
* Do not perform long-running operations, blocking I/O, or operations with unbounded latency while holding the lock.
* `unlock_read()` must only be called by a thread that successfully acquired a read lock.
* `unlock_write()` must only be called by a thread that successfully acquired the write lock.
* The primitive does not provide recursive locking. A thread must not acquire the same write lock recursively.
* The implementation provides synchronization through compiler atomic built-ins, but system-level scheduling and interrupt behavior remain platform dependent.
* `MaxReaders` must be greater than zero.
* For deterministic embedded systems, the worst-case spinning and lock-hold time should be considered as part of the system timing analysis.
