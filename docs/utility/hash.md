# Hash helpers

## Overview

Small hash functions and hash functors for fixed-capacity associative containers and IDs.

## Header

```cpp
#include "castle/utility/hash.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| Hash functions | Hash common integer and byte/string inputs. |
| Hash functors | Callable hash objects suitable for `hash_map`/`hash_set` template parameters. |
| Combine helpers | Combine multiple fields into one hash where provided. |

## Example

```cpp
castle::hash<int> hasher;
auto h = hasher(42);
```

## Embedded notes

- A hash is for distribution, not cryptographic security.
- Provide your own hash functor when the key type needs domain-specific mixing.
