# Memory

## Overview
Umbrella header that includes Castle's low-level memory helpers in one place. Use it when a translation unit needs several of these components together and the extra include breadth is acceptable.

## Header
`#include "castle/memory/memory.hpp"`

## Dependencies
- [Addressof](addressof.md)
- [Alignment](alignment.md)
- [Construct](construct.md)
- [Destroy](destroy.md)
- [Lifetime](lifetime.md)
- [New](new.md)
- [Object](object.md)
- [SOO Buffer](soo_buffer.md)
- [Static Storage](static_storage.md)
- [Storage](storage.md)

## Public API
This header adds no new declarations. It re-exports the public APIs from:

- `castle/memory/addressof.hpp`
- `castle/memory/alignment.hpp`
- `castle/memory/construct.hpp`
- `castle/memory/destroy.hpp`
- `castle/memory/lifetime.hpp`
- `castle/memory/new.hpp`
- `castle/memory/object.hpp`
- `castle/memory/soo_buffer.hpp`
- `castle/memory/static_storage.hpp`
- `castle/memory/storage.hpp`

## Usage Example
```cpp
// See: samples/sample_memory.cpp
#include "castle/memory/memory.hpp"
```

## Constraints & Notes
- Allocation behavior, alignment rules, and lifetime preconditions come from the included component headers.
- This is a convenience include only; choose narrower headers when compile-time dependency size matters.
