# Compiler Abstraction

## Overview
Castle's compiler abstraction layer centralizes compiler detection, standard attribute wrappers, and keyword-style portability macros so the rest of the library can stay toolchain-neutral.

## Header
`#include "castle/core/compiler.hpp"`

## Dependencies
- [clang.md](compiler_variants/clang.md)
- [arm.md](compiler_variants/arm.md)
- [gcc.md](compiler_variants/gcc.md)
- [default.md](compiler_variants/default.md)

## Public API
| API | Description |
|---|---|
| `CASTLE_UNUSED`, `CASTLE_NODISCARD`, `CASTLE_FALL_THROUGH`, `CASTLE_DEPRECATED`, `CASTLE_DEPRECATED_MSG(x)` | Wrap standard C++17 attributes behind Castle names. |
| `CASTLE_VOLATILE`, `CASTLE_MUTABLE`, `CASTLE_CONST`, `CASTLE_CONSTEXPR`, `CASTLE_IF_CONSTEXPR`, `CASTLE_NOEXCEPT` | Wrap core C++ qualifiers and statements used throughout Castle. |
| `CASTLE_DEFAULT`, `CASTLE_DELETE`, `CASTLE_OVERRIDE`, `CASTLE_FINAL`, `CASTLE_VIRTUAL` | Wrap special-member and inheritance keywords. |
| `CASTLE_MOVE`, `CASTLE_FORWARD`, `CASTLE_STD` | Name Castle forwarding helpers or the `std` namespace without spelling them directly. |
| Backend selection | Includes one compiler-variant header according to the active compiler. |

## Usage Example
```cpp
#include "castle/core/compiler.hpp"

CASTLE_NODISCARD CASTLE_CONSTEXPR int answer() CASTLE_NOEXCEPT
{
    return 42;
}
```

See also: `samples/sample_compiler.cpp`.

## Constraints & Notes
- Intended as a low-level dependency used transitively by most Castle headers.
- Performs compile-time selection only; there is no runtime state or allocation.
- `CASTLE_MOVE` and `CASTLE_FORWARD` refer to Castle utilities and are only useful once those helpers are available.
