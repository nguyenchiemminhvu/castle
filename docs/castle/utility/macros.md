# Macros

## Overview
This header collects Castle's lightweight utility macros for token concatenation, stringification, and compile-time bit masks. It exists for low-level embedded code that often needs small preprocessor helpers.

## Header
`#include "castle/utility/macros.hpp"`

## Dependencies
None

## Public API
| API | Description |
|---|---|
| `CASTLE_STRINGIFY_HELPER(...)` | Raw token-to-string helper. |
| `CASTLE_STRINGIFY(...)` | Expands then stringifies its arguments. |
| `CASTLE_CONCAT_HELPER(x, y)` | Raw token concatenation helper. |
| `CASTLE_CONCAT(x, y)` | Expands then concatenates two tokens. |
| `CASTLE_STRING(x)` | Produces a narrow string literal. |
| `CASTLE_WIDE_STRING(x)` | Produces a wide string literal. |
| `CASTLE_U8_STRING(x)` | Produces a UTF-8 string literal. |
| `CASTLE_U16_STRING(x)` | Produces a UTF-16 string literal. |
| `CASTLE_U32_STRING(x)` | Produces a UTF-32 string literal. |
| `CASTLE_BIT(n)` | Produces a 32-bit single-bit mask. |
| `CASTLE_BIT64(n)` | Produces a 64-bit single-bit mask. |

## Usage Example
See `samples/sample_macros.cpp`.

```cpp
uint32_t mask = CASTLE_BIT(5U);
auto text = CASTLE_STRING(device_id);
```

## Constraints & Notes
- Pure preprocessor utilities; no runtime allocation or state.
- Bit-mask macros shift a 32-bit or 64-bit literal respectively.
- String helper macros operate on tokens, not runtime strings.
