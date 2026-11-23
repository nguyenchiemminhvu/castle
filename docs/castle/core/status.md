# Status

Castle-wide status codes for expected runtime outcomes in a no-exception environment. Use `castle::status` for capacity, lookup, configuration, parsing, protocol, lifecycle, and numerical failures or pending operations.

## Header
`#include "castle/core/status.hpp"`

## Dependencies
None

## Public API
| API | Description |
| --- | --- |
| `enum class status : uint8_t` | Common status code enum used throughout Castle. |
| `succeeded(status)` | Returns `true` only for `status::ok`. |

Status values:
- `ok`: operation completed successfully.
- `full`: a fixed-capacity destination was exhausted.
- `empty`: the requested source contains no data or elements.
- `out_of_range`: an index, numeric conversion, or configured bound exceeded the supported range.
- `not_found`: the requested key, item, attribute, or entry does not exist.
- `not_configured`: required configuration or initialization has not been supplied.
- `invalid_config`: configuration data exists but is malformed or inconsistent.
- `invalid_argument`: caller input or an operation argument is invalid.
- `invalid_callback`: callback registration or invocation target is invalid.
- `invalid_subscription`: subscription token or observer registration is invalid.
- `already_exists`: creation or insertion failed because the target already exists.
- `system_call_error`: a wrapped platform or compiler primitive reported failure.
- `data_loss`: the request would lose information or received data was truncated.
- `time_out`: the operation did not complete before its deadline.
- `unknown_error`: fallback for an unclassified failure.
- `parse_error`: a complete encoded or serialized input cannot be parsed. Incomplete streaming input may use `empty` when more bytes can arrive.
- `protocol_error`: a decoded protocol message violates protocol rules or sequencing.
- `unsupported`: a valid operation, version, or feature is not supported.
- `invalid_state`: the operation conflicts with the object's lifecycle or pending state.
- `unavailable`: a remote service or resource is temporarily unavailable.
- `permission_denied`: an explicit authorization or policy refusal.
- `in_progress`: an asynchronous operation has started but has not completed.
- `resource_exhausted`: a required external resource could not be acquired; distinct from fixed-capacity `full`.
- `interrupted`: the operation was interrupted before completion.
- `redirected`: the remote endpoint directs the caller to another endpoint.
- `not_converged`: a bounded iterative calculation did not satisfy its convergence criterion.
- `insufficient_data`: a non-empty data set has fewer observations than required.
- `cycle_detected`: a graph operation found a cycle that prevents the requested result.

## Usage Example
```cpp
// See: samples/castle/core/status.cpp
#include "castle/core/status.hpp"

if (!castle::succeeded(castle::status::not_found))
{
    // Handle the bounded error path.
}
```

## Constraints & Notes
- `status` is intentionally small (`uint8_t`) and trivially copyable.
- `succeeded()` returns `true` only for `status::ok`. All other values are non-success outcomes; `status::in_progress` reports pending work.
- Higher-level subsystems may pair `castle::status` with a format-specific error enum for finer diagnostics.
