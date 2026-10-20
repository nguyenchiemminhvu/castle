# UBX-NAV-GEOFENCE

## Overview
`nav_geofence` models UBX-NAV-GEOFENCE (0x01 0x39), the receiver's geofencing status: whether each configured fence contains the current position, plus a combined state over all fences. It is a periodic/polled output with an 8-byte header followed by `numFences` 2-byte fence blocks (`8 + numFences·2`).

## Header
`#include "castle_ext/protocols/ubx/messages/nav_geofence.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/repeated_group.hpp`](detail/repeated_group.md)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_geofence_state` | `enum class : uint8_t`: `unknown = 0`, `inside = 1`, `outside = 2`. Used for each fence and for the combined state. |
| `nav_geofence_fence` | One fence block: `state`, `id` (0 = not available). `wire_size = 2`. |
| `nav_geofence<MaxFences = 4>` | The message. Stores up to `MaxFences` fence blocks. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` | `0x01` / `0x39` | UBX identity. |
| `header_length` | `8` | Fixed header bytes. |
| `block_length` | `2` | Bytes per fence block. |
| `STATUS_NOT_AVAILABLE` / `STATUS_ACTIVE` | `0` / `1` | Values of `status`. |

### Fields

| Field | Type | Spec | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | `iTOW` U4 @0 | GPS time of week [ms]. |
| `version` | `uint8_t` | `version` U1 @4 | Message version (0x00). |
| `status` | `uint8_t` | `status` U1 @5 | Geofencing status, see `STATUS_*`. |
| `num_fences` | `uint8_t` | `numFences` U1 @6 | Fence count **on the wire**; may exceed `fences.size()`. |
| `comb_state` | `nav_geofence_state` | `combState` U1 @7 | Combined state over all fences. |
| `fences` | `castle::container::vector<nav_geofence_fence, MaxFences>` | `8 + n·2` | Stored fence blocks (`state` U1, `id` U1). |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view&)` | `true` for class 0x01, ID 0x39. |
| `static bool decode(const message_view&, nav_geofence&)` | Requires `payload_length == 8 + numFences·2` exactly. Stores up to `MaxFences` blocks and skips the rest. On failure `out` is value-initialised and `false` is returned. |
| `void encode(payload_writer&) const` | Writes the header and every stored fence. `numFences` is taken from `fences.size()`, not from `num_fences`, and the message is not copied or modified. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_geofence.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void on_message(const message_view& raw)
{
    nav_geofence<> fences{};
    if (nav_geofence<>::decode(raw, fences) && (fences.comb_state == nav_geofence_state::outside))
    {
        for (const nav_geofence_fence& fence : fences.fences)
        {
            // fence.id, fence.state
        }
    }
}

// Poll the current status:
// encode_poll<nav_geofence<>>(frame, sizeof(frame), written);
```

See [`samples/castle_ext/protocols/ubx/messages/nav_geofence.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/nav_geofence.cpp).

## Constraints & Notes
- No heap allocation; storage is `sizeof(nav_geofence<N>)`, about `8 + 2·N` bytes plus vector bookkeeping.
- The length check uses the declared `numFences`, so a truncated or padded payload is rejected rather than partially decoded.
- `num_fences` is the wire value; use `fences.size()` for what was stored.
- Value type; not thread-safe. Decoded data does not alias the parser buffer.
