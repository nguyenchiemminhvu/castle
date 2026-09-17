# Random

## Overview
Deterministic `uint32_t` pseudo-random generator for embedded systems, using explicit object-held state instead of global entropy or libc facilities.

## Header
`#include "castle/math/random.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| API | Description |
|---|---|
| `class castle::math::random` | xoshiro128** generator with 128 bits of internal state and deterministic seeding. |
| `random()` / `explicit random(result_type seed_value)` | Construct with the library default seed or an explicit caller-provided seed. |
| `void seed(result_type seed_value)` | Reinitializes the generator from a single seed using SplitMix32-style expansion. |
| `result_type operator()()` / `result_type next()` | Produces the next 32-bit output. |
| `result_type uniform(result_type bound)` | Produces a uniform result in `[0, bound)` using multiply-high rejection sampling. |
| `result_type range(result_type low, result_type high)` | Produces a uniform result in the inclusive interval `[low, high]`. |
| `void discard(uint32_t count)` | Advances the sequence by `count` outputs. |
| `static result_type min()` / `static result_type max()` | Returns the generator's output bounds. |

## Usage Example
See `samples/sample_random.cpp` for a complete example.

```cpp
castle::math::random a(1234U);
castle::math::random b(1234U);
const uint32_t first_a = a.next();
const uint32_t first_b = b.next();
```

## Constraints & Notes
- Same seed, same sequence; default construction is also reproducible because it uses a fixed seed.
- `uniform()` requires `bound != 0`; `range()` requires `low <= high`.
- `uniform()` avoids modulo bias; `range()` becomes `next()` when the requested interval spans the full `uint32_t` domain.
- This is not a cryptographic RNG.
