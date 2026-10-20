# UBX NAV-ORB

## Overview

`nav_orb<MaxSvs>` models UBX-NAV-ORB (0x01 0x34), the periodic/polled GNSS orbit database info: an 8-byte header followed by `numSv` 6-byte satellite blocks (`8 + numSv·6`). Each `nav_orb_sv` keeps the raw `svFlag`, `eph`, `alm` and `otherOrb` bytes and exposes their sub-fields (health, visibility, usability, source, orbit type).

## Header

`#include "castle_ext/protocols/ubx/messages/nav_orb.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/repeated_group.hpp`](detail/repeated_group.md)
- [`castle_ext/protocols/ubx/messages/nav_sat.hpp`](nav_sat.md) (`nav_sv_health`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_orb_visibility : uint8_t` | `unknown = 0`, `below_horizon = 1`, `above_horizon = 2`, `above_elevation_mask = 3`. |
| `nav_orb_source : uint8_t` | `ephSource` / `almSource`: `not_available = 0`, `gnss_transmission = 1`, `external_aiding = 2` (wire 3..7 = other). |
| `nav_orb_other_type : uint8_t` | `otherOrb.type`: `none = 0`, `assistnow_offline = 1`, `assistnow_autonomous = 2` (wire 3..7 = other). |
| `nav_orb_sv` | One 6-byte satellite block. |
| `nav_orb<MaxSvs = 64U>` | The message; `MaxSvs <= 255`. |

### Constants

| Constant | Value |
| --- | --- |
| `nav_orb::msg_class` / `msg_id` | `UBX_CLASS_NAV` / `UBX_ID_NAV_ORB` |
| `nav_orb::header_length` | `8U` |
| `nav_orb::block_length` / `nav_orb_sv::wire_size` | `6U` |
| `nav_orb_sv::SV_FLAG_HEALTH` / `SV_FLAG_VISIBILITY` | `0x03` (bits 1..0) / `0x0C` (bits 3..2) |
| `nav_orb_sv::EPH_USABILITY` / `EPH_SOURCE` | `0x1F` (bits 4..0) / `0xE0` (bits 7..5) |
| `nav_orb_sv::ALM_USABILITY` / `ALM_SOURCE` | `0x1F` / `0xE0` |
| `nav_orb_sv::OTHER_ORB_ANO_AOP_USABILITY` / `OTHER_ORB_TYPE` | `0x1F` / `0xE0` |
| `nav_orb_sv::USABILITY_UNKNOWN` | `31` - usability period unknown |
| `nav_orb_sv::USABILITY_MAX` | `30` - more than 450 min (eph) / 30 days (alm, other) |
| `nav_orb_sv::USABILITY_EXPIRED` | `0` - data can no longer be used |

### `nav_orb_sv` fields

| Field | Type | Description |
| --- | --- | --- |
| `gnss_id` | `uint8_t` | GNSS identifier. |
| `sv_id` | `uint8_t` | Satellite identifier. |
| `sv_flag` | `uint8_t` | Information flags (`SV_FLAG_*`). |
| `eph` | `uint8_t` | Ephemeris data (`EPH_*`). |
| `alm` | `uint8_t` | Almanac data (`ALM_*`). |
| `other_orb` | `uint8_t` | Other orbit data (`OTHER_ORB_*`). |

### `nav_orb` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| `version` | `uint8_t` | - | Message version (0x01). |
| `num_sv` | `uint8_t` | - | On-wire SV count; may exceed `svs.size()`. |
| `svs` | `castle::container::vector<nav_orb_sv, MaxSvs>` | - | Stored SV blocks. |

### Functions

| Function | Description |
| --- | --- |
| `nav_orb_sv::health()` | `nav_sv_health` from `svFlag` bits 1..0. |
| `nav_orb_sv::visibility()` | `nav_orb_visibility` from `svFlag` bits 3..2. |
| `nav_orb_sv::eph_usability()` | 0..31; 1..29 = usable for (n-1)·15 to n·15 minutes. |
| `nav_orb_sv::eph_source()` / `alm_source()` | `nav_orb_source` from bits 7..5. |
| `nav_orb_sv::alm_usability()` / `ano_aop_usability()` | 0..31; 1..29 = usable for n-1 to n days. |
| `nav_orb_sv::other_orb_type()` | `nav_orb_other_type` from `otherOrb` bits 7..5. |
| `static bool nav_orb::matches(const message_view&)` | Class/ID check. |
| `static bool nav_orb::decode(const message_view&, nav_orb&)` | Validates the exact length, decodes the header and up to `MaxSvs` blocks. |
| `void nav_orb::encode(payload_writer&) const` | Writes the header (count from `svs.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_orb.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    nav_orb<64U> orb;
    if (!nav_orb<64U>::decode(raw, orb))
    {
        return;
    }
    for (const nav_orb_sv& sv : orb.svs)
    {
        if ((sv.eph_source() != nav_orb_source::not_available) &&
            (sv.eph_usability() != nav_orb_sv::USABILITY_EXPIRED))
        {
            // ephemeris still usable ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/nav_orb.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Capacity is `MaxSvs` (default 64). Blocks beyond capacity are skipped; `num_sv` keeps the on-wire count while `svs.size()` is the stored count.
- No heap allocation; the vector is embedded in the value.
- `decode()` returns `false` and resets the output unless the class/ID match and the payload length is exactly `8 + numSv·6`.
- `encode()` takes `numSv` from `svs.size()`; reserved bytes are written as zero.
- The spec labels `visibility` as "SV health" in its description column; the enumerated values are visibility states and are modelled as such.
- Thread-safety: none; plain value type, not thread-safe.
