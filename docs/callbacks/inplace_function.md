# In-place callback wrapper

## Overview

A type-erased callable wrapper with caller-selected storage size and alignment. The callable lives inside the object, so captured lambdas can be used without heap allocation.

## Header

```cpp
#include "castle/callbacks/inplace_function.h"
```

**Namespace:** `castle::callbacks`

## Main API

| API | Purpose |
|---|---|
| `inplace_function<R(Args...), StorageSize, StorageAlignment>` | Owns one callable in an internal byte buffer. |
| Callable constructor | Accepts functors and lambdas that fit the configured storage. |
| `operator()` | Invoke the stored callable. |
| `operator bool()` | Test whether a callable is stored. |
| Copy/move operations | Copy and move the contained callable using small function pointers. |

## Example

```cpp
using callback_t = castle::callbacks::inplace_function<void(int), 32>;
callback_t cb = [](int v) { /* ... */ };
cb(7);
```

## Embedded notes

- The callable must fit within `StorageSize` and `StorageAlignment`.
- A too-large or over-aligned callable fails at compile time.
- No heap is required.
