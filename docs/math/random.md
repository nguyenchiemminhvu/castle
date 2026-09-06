# Deterministic random helpers

## Overview

Lightweight pseudo-random generators for embedded use, with state held explicitly in value types.

## Header

```cpp
#include "castle/math/random.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| Random engine/type | Stateful pseudo-random generator supplied by the header. |
| Next-value functions | Generate the next integer value or bounded range according to the engine API. |
| Seed/state | Initialize the generator from an explicit seed. |

## Example

```cpp
castle::math::random rng(1234u);
auto value = rng.next();
```

## Embedded notes

- These are not cryptographic generators.
- Keep the generator state in the object so randomness is deterministic and testable when a fixed seed is used.
