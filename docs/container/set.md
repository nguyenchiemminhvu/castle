# Ordered set wrapper

## Overview

Convenience fixed-capacity ordered set built on Castle’s balanced tree machinery.

## Header

```cpp
#include "castle/container/set.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| Set type | Stores unique ordered keys. |
| Insertion | Insert a value if it is not present. |
| Lookup | Find keys and use ordered bound queries. |
| Erase/clear | Remove values and reset the container. |

## Example

```cpp
castle::container::set<int, 16> s;
s.insert(10);
if (s.find(10) != s.end()) { /* ... */ }
```

## Embedded notes

- For key-only storage, a set avoids the mapped-value field used by `map`.
