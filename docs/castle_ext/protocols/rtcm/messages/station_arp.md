# RTCM station ARP (1005 / 1006)

## Overview

`station_arp_message<Type>` builds RTCM 3 messages 1005 (stationary RTK reference station ARP) and 1006 (the same plus the antenna height above the marker). Both carry the antenna reference point of the base station in ECEF coordinates (ITRF) and the satellite systems it serves. A rover such as a u-blox ZED-F9R needs this position to apply the observation corrections.

| Type | Alias | Content | Payload bits | Frame bytes |
| --- | --- | --- | --- | --- |
| 1005 | `station_arp` | ARP | 152 | 25 |
| 1006 | `station_arp_height` | ARP and antenna height | 168 | 27 |

As for the MSM models, there is **one type per message number**: the template argument `Type` fixes the message number at compile time. Use the aliases.

This is an **encode-only** model: it produces the message payload for `encode_message()` and there is no decoder. Coordinates are kept as the raw 0.0001 m integers sent on the wire; `set_position_m()` and `x_m()` / `y_m()` / `z_m()` convert from and to metres.

## Header

`#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../../core/compiler.md)
- [`core/types.hpp`](../../../../core/types.md)
- [`error/status.hpp`](../../../../error/status.md)
- [`castle_ext/protocols/rtcm/bit_writer.hpp`](../bit_writer.md)
- [`castle_ext/protocols/rtcm/rtcm.hpp`](../rtcm.md)
- [`math/round.hpp`](../../../../math/round.md)

## Public API

All names live in `castle::protocols::rtcm::messages`.

### Types

| Type | Description |
| --- | --- |
| `station_arp_message<uint16_t Type>` | The message. `Type` must be `RTCM_TYPE_STATION_ARP` (1005) or `RTCM_TYPE_STATION_ARP_HEIGHT` (1006); any other number is a compile error (`static_assert`). |
| `station_arp` | Alias of `station_arp_message<RTCM_TYPE_STATION_ARP>` (1005). |
| `station_arp_height` | Alias of `station_arp_message<RTCM_TYPE_STATION_ARP_HEIGHT>` (1006). |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `MESSAGE_TYPE` | `uint16_t`, `Type` | DF002, written first. |
| `HAS_ANTENNA_HEIGHT` | `true` for 1006 | The message carries `antenna_height` (DF028). |
| `RESOLUTION_M` | `0.0001` | Resolution of the coordinates and the antenna height, in metres. |

### Fields

Both aliases have the same fields.

| Field | Type | Wire format | Description |
| --- | --- | --- | --- |
| `station_id` | `uint16_t` | DF003, 12 bit | Reference station ID, 0..4095. |
| `itrf_year` | `uint8_t` | DF021, 6 bit | ITRF realization year, 0..63. |
| `gps` / `glonass` / `galileo` | `bool` | DF022 / DF023 / DF024 | Service supported. |
| `reference_station_indicator` | `bool` | DF141 | `false` = physical station, `true` = non-physical (network) station. |
| `ecef_x` | `int64_t` | DF025, 38 bit signed | ARP ECEF X in 0.0001 m, `-2^37 .. 2^37 - 1`. |
| `single_receiver_oscillator` | `bool` | DF142 | All observations use one oscillator. Followed by one reserved zero bit. |
| `ecef_y` | `int64_t` | DF026, 38 bit signed | ARP ECEF Y in 0.0001 m. |
| `quarter_cycle_indicator` | `uint8_t` | DF364, 2 bit | 0 = unspecified, 1 = correct, 2 = correct for L1 C/A, 3 = reserved. |
| `ecef_z` | `int64_t` | DF027, 38 bit signed | ARP ECEF Z in 0.0001 m. |
| `antenna_height` | `uint16_t` | DF028, 16 bit | Antenna height in 0.0001 m. Written by 1006 only; ignored by 1005. |

### Functions

| Function | Returns | Description | Complexity |
| --- | --- | --- | --- |
| `static constexpr bool accepts(uint16_t type)` | `bool` | `type == MESSAGE_TYPE`, so `station_arp::accepts(1006U)` is `false`. | O(1) |
| `void set_position_m(double x, double y, double z)` | - | Stores the ARP in metres, rounded to the nearest 0.0001 m (ties away from zero). Values far outside the 38-bit range saturate to a value that `encode()` rejects. | O(1) |
| `void set_antenna_height_m(double height)` | - | Stores the height in metres, rounded to 0.0001 m and clamped to `0 .. 65535` units (0 .. 6.5535 m). `static_assert`s `HAS_ANTENNA_HEIGHT`, so it compiles only for `station_arp_height`. | O(1) |
| `double x_m() const` / `y_m()` / `z_m()` | `double` | The stored coordinate in metres. | O(1) |
| `double antenna_height_m() const` | `double` | The stored antenna height in metres. | O(1) |
| `void encode(bit_writer& writer) const` | - | Writes the payload (152 bits for 1005, 168 bits for 1006). Used by `encode_message()`. | O(1) |

### Encode results

`encode_message(station, output, capacity, written)` returns:

| Status | Cause |
| --- | --- |
| `ok` | Frame written; 25 bytes for 1005, 27 bytes for 1006. |
| `invalid_argument` | `output` is `nullptr`. |
| `out_of_range` | A coordinate does not fit 38 bits signed, or `station_id` > 4095, `itrf_year` > 63, `quarter_cycle_indicator` > 3. |
| `full` | `capacity` is smaller than the frame. |

`written` is `0` on every failure. A wrong message number cannot reach `encode()`: it does not compile.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

castle::status encode_base(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    station_arp_height station{}; // 1006
    station.station_id = 7U;
    station.gps = true;
    station.glonass = true;
    station.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    station.set_antenna_height_m(1.5);
    return encode_message(station, frame, capacity, written);
}
```

See [`samples/castle_ext/protocols/rtcm/messages/station_arp.cpp`](../../../../../samples/castle_ext/protocols/rtcm/messages/station_arp.cpp).

## Constraints & Notes

- Encode only: there is no parser or decoder for 1005/1006.
- One type per message number. Sending both 1005 and 1006 needs one `station_arp` and one `station_arp_height`; there is no run-time message-number field to change.
- The reserved bit between DF142 and DF026 is always written as zero.
- `antenna_height` exists on both aliases but is ignored by 1005; the frame is then 25 bytes instead of 27. `set_antenna_height_m()` does not compile for `station_arp`.
- Range errors are not reported by `set_position_m()`; they surface as `out_of_range` from `encode_message()`. The valid range is about +-13743895.3472 m per axis.
- `set_position_m()` and `set_antenna_height_m()` store `0` for a NaN argument; infinities saturate and are then rejected by `encode()` with `out_of_range` (positions) or clamped to the 16-bit height range.
- No heap allocation, exceptions, RTTI or STL; the payload is written straight into the caller's frame buffer.
- Thread-safety: none. Instances are plain values.
