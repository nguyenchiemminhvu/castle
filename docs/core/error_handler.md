# Error Handler

## Overview
This header provides Castle's exception-free failure descriptor and assertion-routing macros for embedded builds that need deterministic error handling without heap allocation.

## Header
`#include "castle/core/error_handler.hpp"`

## Dependencies
- [compiler.md](compiler.md)

## Public API
| API | Description |
|---|---|
| `castle::exception` | Stores a reason string, source file string, and line number without dynamic allocation. |
| `castle::error_handler` | Optional global callback slot used when `CASTLE_LOG_ERRORS` or `CASTLE_USE_ASSERT_FUNCTION` is enabled. |
| `CASTLE_ERROR(TYPE)`, `CASTLE_ERROR_GENERIC(TEXT)` | Build error objects according to the active detail mode. |
| `CASTLE_ASSERT*` macros | Validate conditions, optionally dispatch errors, and optionally return from the current function depending on the configured assertion mode. |
| `CASTLE_DO_NOTHING` | Explicit no-op helper used by disabled assertion paths. |

## Usage Example
```cpp
#include "castle/core/error_handler.hpp"

castle::exception error("range", "sensor.cpp", 12);
```

See also: `samples/sample_error_handler.cpp`.

## Constraints & Notes
- Behavior is selected by preprocessor switches such as `CASTLE_NO_CHECKS`, `CASTLE_USE_ASSERT_FUNCTION`, `CASTLE_LOG_ERRORS`, `CASTLE_VERBOSE_ERRORS`, and `CASTLE_MINIMAL_ERRORS`.
- The default fallback uses C `assert()` in debug builds and mostly evaporates in release builds.
- No exceptions are thrown and the callback path uses a single plain function pointer.
