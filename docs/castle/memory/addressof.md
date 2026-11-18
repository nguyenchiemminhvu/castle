# Addressof

## Overview
Returns the true address of an existing object even when `operator&` is overloaded. This is mainly useful in low-level storage and container code that must recover a raw pointer without changing lifetime or ownership.

## Header
`#include "castle/memory/addressof.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Traits](../core/traits.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T> T* castle::memory::addressof(T& value)` | Returns the real mutable address of `value`. O(1). |
| `template <typename T> T const* castle::memory::addressof(T const& value)` | Returns the real const address of `value`. O(1). |

## Usage Example
```cpp
// See: samples/sample_addressof.cpp
struct RegisterRef
{
    RegisterRef* operator&() { return nullptr; }
    uint32_t value;
};

RegisterRef ref{7U};
RegisterRef* ptr = castle::memory::addressof(ref);
```

## Constraints & Notes
- No allocation or lifetime transition happens here.
- `value` must already be a valid object.
- The helper only bypasses overloaded address-of operators; it does not validate alignment.
