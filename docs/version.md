# Version

## Overview
This header exposes Castle's locked semantic-version information as preprocessor macros and constexpr integers for compile-time gating and lightweight runtime metadata.

## Header
`#include "castle/version.hpp"`

## Dependencies
- [compiler.md](core/compiler.md)

## Public API
| API | Description |
|---|---|
| `CASTLE_VERSION_MAJOR`, `CASTLE_VERSION_MINOR`, `CASTLE_VERSION_PATCH` | Semantic-version components. |
| `CASTLE_VERSION_ENCODE(major, minor, patch)` | Packs version fields into a comparable integer. |
| `CASTLE_VERSION` | Encoded current Castle version. |
| `CASTLE_STRINGIFY_IMPL(x)`, `CASTLE_STRINGIFY(x)` | Preprocessor helpers used to build version strings. |
| `CASTLE_VERSION_STRING` | String literal containing the current semantic version. |
| `CASTLE_VERSION_AT_LEAST(major, minor, patch)` | Tests whether the current version meets a minimum requirement. |
| `castle::version_encoded`, `castle::version_major`, `castle::version_minor`, `castle::version_patch` | `constexpr` mirrors of the macro values. |

## Usage Example
```cpp
#include "castle/version.hpp"

static_assert(CASTLE_VERSION_AT_LEAST(2, 0, 0), "Castle 2.0 or newer required");
```

See also: `samples/sample_version.cpp`.

## Constraints & Notes
- The encoded format uses 8 bits per semantic-version field.
- All exported values are compile-time constants; there is no runtime version registry.
