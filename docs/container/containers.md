# Container umbrella header

## Overview

Convenience include for the fixed-capacity containers in Castle.

## Header

```cpp
#include "castle/container/containers.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| Container families | Pulls in arrays, vectors, maps/sets, hash containers, lists, stacks, ring buffers, strings, bitsets, heaps, and related views. |

## Example

```cpp
#include "castle/container/containers.h"
castle::container::vector<int, 8> values;
```

## Embedded notes

- Use individual container headers to minimize preprocessing when build times are important.
