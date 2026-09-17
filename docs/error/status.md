# Status

## Overview
Castle-wide status codes for expected runtime outcomes in a no-exception environment. Use `castle::status` for capacity, lookup, configuration, and conversion failures instead of exceptions.

## Header
`#include "castle/error/status.hpp"`

## Dependencies
None

## Public API
| API | Description |
| --- | --- |
| `enum class status : uint8_t` | Common status code enum used throughout Castle. |
| `succeeded(status)` | Returns `true` only for `status::ok`. |

Status values:
- `ok`: operation completed successfully.
- `full`: destination capacity was exhausted.
- `empty`: requested data structure or document contained no elements.
- `out_of_range`: a value, index, or configured bound exceeded the supported range.
- `not_found`: requested key, item, attribute, or entry does not exist.
- `not_configured`: required configuration or initialization has not been supplied.
- `invalid_config`: configuration data exists but is malformed or inconsistent.
- `invalid_argument`: caller input or parsed data is syntactically invalid for the operation.
- `invalid_callback`: callback registration or invocation target is invalid.
- `invalid_subscription`: subscription token or observer registration is invalid.
- `already_exists`: creation or insertion failed because the target already exists.
- `system_call_error`: a wrapped platform or compiler primitive reported failure.
- `data_loss`: the requested conversion/view would lose information.
- `unknown_error`: fallback code for an unclassified failure.

## Usage Example
```cpp
// See: samples/sample_status.cpp
#include "castle/error/status.hpp"

if (!castle::succeeded(castle::status::not_found))
{
    // Handle the bounded error path.
}
```

## Constraints & Notes
- `status` is intentionally small (`uint8_t`) and trivially copyable.
- `succeeded()` only treats `status::ok` as success; every other enumerator is a failure path.
- Higher-level subsystems often pair `castle::status` with a format-specific error enum for finer diagnostics.
