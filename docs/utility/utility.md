# Utility

## Overview
This is the umbrella header for Castle's utility layer. It exists for translation units that want the standard-library-like utility subset in a single include.

## Header
`#include "castle/utility/utility.hpp"`

## Dependencies
[macros.md](macros.md), [forward.md](forward.md), [move.md](move.md), [swap.md](swap.md), [safe_cast.md](safe_cast.md), [bit_cast.md](bit_cast.md), [tuple.md](tuple.md), [pair.md](pair.md), [variant.md](variant.md), [optional.md](optional.md), [bitset.md](bitset.md), [hash.md](hash.md)

## Public API
This header adds no direct runtime API of its own. It re-exports:
- `macros.hpp`
- `forward.hpp`
- `move.hpp`
- `swap.hpp`
- `safe_cast.hpp`
- `bit_cast.hpp`
- `tuple.hpp`
- `pair.hpp`
- `variant.hpp`
- `optional.hpp`
- `bitset.hpp`
- `hash.hpp`

## Usage Example
See `samples/sample_utility.cpp`.

```cpp
#include "castle/utility/utility.hpp"
```

## Constraints & Notes
- Pulls in the full Castle utility surface listed above.
- Does not allocate or define additional runtime state by itself.
- Useful as a convenience include, but narrower includes may reduce compile time.
