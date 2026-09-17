# Iterator category tags

## Overview

Defines iterator category tag types used by Castle containers and algorithms.

## Header

```cpp
#include "castle/iterator/tags.h"
```

**Namespace:** `castle::iterator`

## Main API

| API | Purpose |
|---|---|
| Iterator tags | Input, output, forward, bidirectional, and random-access category tags. |

## Example

```cpp
template <typename It> void scan(It first, It last, castle::iterator::forward_iterator_tag* = nullptr) { /* ... */ }
```

## Embedded notes

- The tags let template code select algorithms without depending on STL iterator tags.
