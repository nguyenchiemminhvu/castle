# RTCM receiver and antenna descriptor (1033)

## Overview

`receiver_antenna_descriptor` builds RTCM 3 message 1033. It extends [message 1007](antenna_descriptor.md) with the antenna serial number and the receiver type, firmware version and serial number of the reference station.

This is an **encode-only** model: it produces the message payload for `encode_message()` and there is no decoder.

## Header

`#include "castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../../core/compiler.md)
- [`core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/rtcm/bit_writer.hpp`](../bit_writer.md)
- [`castle_ext/protocols/rtcm/rtcm.hpp`](../rtcm.md)
- [`castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp`](antenna_descriptor.md) (`descriptor_string`)

## Public API

All names live in `castle::protocols::rtcm::messages`.

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `receiver_antenna_descriptor::MESSAGE_TYPE` | `RTCM_TYPE_RECEIVER_ANTENNA` (1033) | Message number (DF002), written first. |

### Fields

Every string is a `descriptor_string` (at most 31 characters, written as an 8-bit count followed by the 8-bit characters). Fields are written in this order:

| Field | Type | Wire format | Description |
| --- | --- | --- | --- |
| `station_id` | `uint16_t` | DF003, 12 bit | Reference station ID, 0..4095. |
| `antenna` | `descriptor_string` | DF029 / DF030 | Antenna descriptor. |
| `antenna_setup_id` | `uint8_t` | DF031, 8 bit | Antenna setup ID, 0..255. |
| `antenna_serial` | `descriptor_string` | DF032 / DF033 | Antenna serial number. |
| `receiver_type` | `descriptor_string` | DF227 / DF228 | Receiver type descriptor. |
| `firmware` | `descriptor_string` | DF229 / DF230 | Receiver firmware version. |
| `receiver_serial` | `descriptor_string` | DF231 / DF232 | Receiver serial number. |

### Functions

| Function | Returns | Description | Complexity |
| --- | --- | --- | --- |
| `static constexpr bool accepts(uint16_t type)` | `bool` | `type == MESSAGE_TYPE`, i.e. `true` for 1033 only. | O(1) |
| `void encode(bit_writer& writer) const` | - | Writes `MESSAGE_TYPE` and the fields above. Used by `encode_message()`. | O(total string length) |

### Encode results

`encode_message(info, output, capacity, written)` returns `ok` (frame size `6 + 9 + total characters` bytes; 15 bytes with all strings empty, 170 bytes with all five at 31 characters), `invalid_argument` for a `nullptr` output, `out_of_range` for `station_id` > 4095, and `full` when the frame does not fit. `written` is `0` on failure.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

castle::status encode_info(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    receiver_antenna_descriptor info{};
    info.station_id = 7U;
    static_cast<void>(info.antenna.assign("ANT"));
    static_cast<void>(info.antenna_serial.assign("SN123"));
    static_cast<void>(info.receiver_type.assign("UBLOX ZED-F9P"));
    static_cast<void>(info.firmware.assign("HPG 1.32"));
    static_cast<void>(info.receiver_serial.assign("0042"));
    return encode_message(info, frame, capacity, written);
}
```

See [`samples/castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.cpp`](../../../../../samples/castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.cpp).

## Constraints & Notes

- Encode only: there is no parser or decoder for 1033.
- The 31-character limit is enforced by `descriptor_string::assign()` (it returns `castle::status::full`), not at encode time.
- Empty strings are valid and written as a zero count.
- The largest payload is 164 bytes, well below the 1023-byte RTCM limit.
- No heap allocation, exceptions, RTTI or STL.
- Thread-safety: none. Instances are plain values.
