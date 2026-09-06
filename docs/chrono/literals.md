# Chrono literals

## Overview

Provides user-defined literals for the common Castle chrono duration aliases.

## Header

```cpp
#include "castle/chrono/literals.h"
```

**Namespace:** `castle::chrono_literals`

## Main API

| API | Purpose |
|---|---|
| Duration literals | Construct nanoseconds, microseconds, milliseconds, seconds, minutes, hours, days, and weeks from integer literals where defined. |

## Example

```cpp
using namespace castle::chrono_literals;
auto a = 10_ms;
auto b = 2_s;
```

## Embedded notes

- Use the literals mainly for readable constants and timeout configuration.
