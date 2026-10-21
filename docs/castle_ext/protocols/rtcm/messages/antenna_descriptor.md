# RTCM antenna descriptor (1007)

## Overview

`antenna_descriptor` builds RTCM 3 message 1007. It names the antenna model of the reference station (an IGS/NGS antenna type such as `"TRM59800.00     NONE"`) and carries the antenna setup ID. The header also defines `descriptor_string`, the 31-character string type shared with [`receiver_antenna_descriptor`](receiver_antenna_descriptor.md).

This is an **encode-only** model: it produces the message payload for `encode_message()` and there is no decoder.

## Header

`#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../../core/compiler.md)
- [`core/types.hpp`](../../../../core/types.md)
- [`container/string.hpp`](../../../../container/string.md)
- [`error/status.hpp`](../../../../error/status.md)
- [`castle_ext/protocols/rtcm/bit_writer.hpp`](../bit_writer.md)
- [`castle_ext/protocols/rtcm/rtcm.hpp`](../rtcm.md)

## Public API

All names live in `castle::protocols::rtcm::messages`.

### Types and constants

| Name | Description |
| --- | --- |
| `RTCM_MAX_DESCRIPTOR_LEN` | `31U`, the longest descriptor string RTCM allows in messages 1007 and 1033. |
| `descriptor_string` | `castle::container::string<RTCM_MAX_DESCRIPTOR_LEN>`. |
| `antenna_descriptor::MESSAGE_TYPE` | `RTCM_TYPE_ANTENNA_DESCRIPTOR` (1007), the message number (DF002), written first. |

### Fields

| Field | Type | Wire format | Description |
| --- | --- | --- | --- |
| `station_id` | `uint16_t` | DF003, 12 bit | Reference station ID, 0..4095. |
| `descriptor` | `descriptor_string` | DF029 (count, 8 bit) + DF030 (8-bit characters) | Antenna descriptor, 0..31 characters. |
| `setup_id` | `uint8_t` | DF031, 8 bit | Antenna setup ID, 0..255. |

### Functions

| Function | Returns | Description | Complexity |
| --- | --- | --- | --- |
| `static constexpr bool accepts(uint16_t type)` | `bool` | `type == MESSAGE_TYPE`, i.e. `true` for 1007 only. | O(1) |
| `void encode(bit_writer& writer) const` | - | Writes `MESSAGE_TYPE`, `station_id`, the counted string and `setup_id`. Used by `encode_message()`. | O(descriptor length) |

### Encode results

`encode_message(antenna, output, capacity, written)` returns `ok` (frame size `6 + 5 + length` bytes), `invalid_argument` for a `nullptr` output, `out_of_range` for `station_id` > 4095, and `full` when the frame does not fit. `written` is `0` on failure.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

castle::status encode_antenna(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    antenna_descriptor antenna{};
    antenna.station_id = 7U;
    static_cast<void>(antenna.descriptor.assign("TRM59800.00     NONE"));
    antenna.setup_id = 0U;
    return encode_message(antenna, frame, capacity, written);
}
```

See [`samples/castle_ext/protocols/rtcm/messages/antenna_descriptor.cpp`](../../../../../samples/castle_ext/protocols/rtcm/messages/antenna_descriptor.cpp).

## Constraints & Notes

- Encode only: there is no parser or decoder for 1007.
- The string capacity is 31; `descriptor.assign()` with a longer text returns `castle::status::full` and the string stays empty, so the limit is enforced when the text is stored rather than at encode time.
- Characters are written as raw 8-bit values with no encoding check.
- An empty descriptor is written as a zero count with no characters.
- No heap allocation, exceptions, RTTI or STL.
- Thread-safety: none. Instances are plain values.
