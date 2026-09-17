# Construct

## Overview
Starts object lifetime in caller-provided storage with placement construction. It exists for fixed-capacity, deterministic code paths where the bytes are already reserved and no heap allocation is allowed.

## Header
`#include "castle/memory/construct.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Traits](../core/traits.md)
- [Forward](../utility/forward.md)
- [New](new.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T, typename... Args> T* castle::memory::construct_at(void* address, Args&&... args)` | Placement-constructs `T` at `address` and returns `T*`. Constructor arguments are perfectly forwarded. O(1). |

## Usage Example
```cpp
// See: samples/sample_construct.cpp
castle::aligned_storage_as_t<sizeof(uint32_t), uint32_t> storage;
uint32_t* value = castle::memory::construct_at<uint32_t>(storage.get_address<uint32_t>(), 42U);
```

## Constraints & Notes
- `address` must be non-null, sufficiently aligned for `T`, and large enough to hold `T`.
- The destination bytes must not already contain an unrelated live object unless that object's lifetime has already ended.
- Construction does not allocate memory; destruction remains the caller's responsibility.
