# Core Configuration

## Overview
This header exposes Castle's compile-time view of the language level, target endianness, pointer-width class, optional standard-header availability, and default in-place storage sizes.

## Header
`#include "castle/core/config.hpp"`

## Dependencies
- [compiler.md](compiler.md)
- [traits.md](traits.md)
- [types.md](types.md)

## Public API
| API | Description |
|---|---|
| `CASTLE_CPP_11`, `CASTLE_CPP_14`, `CASTLE_CPP_17`, `CASTLE_CPP_20` | Report whether the active language mode meets the named standard level. |
| `CASTLE_ENDIAN_LITTLE`, `CASTLE_ENDIAN_BIG`, `CASTLE_ENDIAN_NATIVE`, `CASTLE_IS_LITTLE_ENDIAN`, `CASTLE_IS_BIG_ENDIAN`, `CASTLE_HAS_CONSTEXPR_ENDIANNESS` | Encode Castle's native-endianness detection and compile-time flags. |
| `CASTLE_PLATFORM_16BIT`, `CASTLE_PLATFORM_32BIT`, `CASTLE_PLATFORM_64BIT` | Report the pointer-width class of the active target. |
| `CASTLE_USING_STD_NEW`, `CASTLE_USING_STD_INITIALIZER_LIST`, `CASTLE_USING_PTHREAD` | Indicate whether optional platform facilities appear to be available. |
| `castle::endian` | Enum describing little-endian, big-endian, and native byte order. |
| `castle::native_endian`, `castle::is_little_endian`, `castle::is_big_endian` | `constexpr` mirrors of the detected endianness. |
| `castle::platform_16bit`, `castle::platform_32bit`, `castle::platform_64bit` | `constexpr` platform-width flags. |
| `castle::inplace_function_storage_words`, `castle::inplace_storage_reserved`, `castle::inplace_alignment_default` | Default sizing/alignment constants used by Castle's in-place storage helpers. |

## Usage Example
```cpp
#include "castle/core/config.hpp"

static_assert(castle::native_endian == castle::endian::native, "native endian available");
```

See also: `samples/sample_config.cpp`.

## Constraints & Notes
- Endianness is detected at compile time and may be overridden by defining `CASTLE_ENDIAN_NATIVE` before inclusion.
- The header may include `<unistd.h>` when probing for POSIX threads.
- All exported values are compile-time constants; there is no runtime configuration object.
