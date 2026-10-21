# RTCM 3 framing

## Overview

`rtcm.hpp` provides the transport framing primitives of RTCM 3.x for Castle. The module is **encode-only**: it builds the frames that are *sent to* a GNSS receiver (for example a u-blox ZED-F9R) as differential correction input. There is deliberately no parser, decoder or frame validator.

An RTCM 3 frame is:

```
[0xD3][6 reserved bits | 10-bit length][payload][CRC-24Q]
```

The CRC-24Q ([`crc24q`](../../checksums/crc24q.md)) covers the preamble, the length field and the payload and is transmitted most-significant byte first. The payload starts with the 12-bit message number (DF002) and is zero-padded to a byte boundary. Payloads are produced by the message models in `castle_ext/protocols/rtcm/messages/` through `encode_message()` (see [`bit_writer.md`](bit_writer.md)); `encode_frame()` frames a payload that already exists.

This frame is what the u-blox interface description lists as the payload of every `RTCM-3X-TYPEnnnn` input message (preamble `0xd3`, `numData`, data, `crc`). It is written to the receiver port as is, with no UBX header: the u-blox "Class/ID 0xf5 0xNN" is a catalogue identifier only and is never on the wire. See [RTCM 3.3 correction input to a u-blox receiver](../ubx/rtcm/rtcm_input.md) for the full type-to-model map and per-type samples.

Everything works on caller-provided buffers.

## Header

`#include "castle_ext/protocols/rtcm/rtcm.hpp"`

## Dependencies

* [`core/compiler.hpp`](../../../core/compiler.md)
* [`core/types.hpp`](../../../core/types.md)
* [`error/status.hpp`](../../../error/status.md)
* [`container/array_view.hpp`](../../../container/array_view.md)
* [`utility/bytes.hpp`](../../../utility/bytes.md)
* [`checksums/crc24q.hpp`](../../checksums/crc24q.md)

## Public API

### Constants (namespace `castle::protocols::rtcm`)

| Constant | Value | Description |
| --- | --- | --- |
| `RTCM_PREAMBLE` | `0xD3` | Frame preamble byte. |
| `RTCM_FRAME_INDEX_PREAMBLE` | `0` | Offset of the preamble. |
| `RTCM_FRAME_INDEX_LENGTH` | `1` | Offset of the two length bytes (2 length MSBs in byte 1, low 8 bits in byte 2). |
| `RTCM_FRAME_INDEX_PAYLOAD` | `3` | Offset of the first payload byte. |
| `RTCM_CRC_SIZE` | `3` | Trailing CRC-24Q size in bytes. |
| `RTCM_FRAME_OVERHEAD` | `6` | Header plus CRC. |
| `RTCM_MAX_PAYLOAD_LEN` | `1023` | Largest payload the 10-bit length field can express. |
| `RTCM_MAX_FRAME_SIZE` | `1029` | Largest complete frame. |
| `RTCM_MESSAGE_NUMBER_BITS` | `12` | Width of the message number (DF002). |
| `RTCM_STATION_ID_BITS` | `12` | Width of the reference station ID (DF003). |
| `RTCM_TYPE_*` | see below | Message numbers accepted by u-blox receivers (UBX-19056845, chapter 4). |

`RTCM_TYPE_*` message numbers: `GPS_L1_OBS` 1001, `GPS_L1_OBS_EXT` 1002, `GPS_L1L2_OBS` 1003, `GPS_L1L2_OBS_EXT` 1004, `STATION_ARP` 1005, `STATION_ARP_HEIGHT` 1006, `ANTENNA_DESCRIPTOR` 1007, `GLO_L1_OBS` 1009, `GLO_L1_OBS_EXT` 1010, `GLO_L1L2_OBS` 1011, `GLO_L1L2_OBS_EXT` 1012, `RECEIVER_ANTENNA` 1033, `GLO_BIASES` 1230.

The u-blox proprietary message 4072.x is not an input message: the receiver only reports its sub-type in UBX-RXM-RTCM ([`rxm_rtcm`](../ubx/messages/rxm_rtcm.md)), so it has no constant or model here.

### Functions

| API | Description |
| --- | --- |
| `constexpr size_type frame_size(size_type payload_length)` | `payload_length + RTCM_FRAME_OVERHEAD`. O(1). |
| `status encode_frame(array_view<const uint8_t> payload, uint8_t* output, size_type output_capacity, size_type& bytes_written)` | Copies the payload into `output` and frames it. O(payload). |

`detail::finish_frame(uint8_t* frame, size_type payload_length)` is the unchecked tail shared by `encode_frame()` and `encode_message()`: it writes the preamble and the big-endian length (the 6 reserved bits stay zero), appends the CRC-24Q and returns the frame size. It is an implementation detail.

`encode_frame()` results; `bytes_written` is the frame size on success and `0` on every failure:

| Status | Condition |
| --- | --- |
| `ok` | Frame written. An empty payload (6-byte frame) is valid. |
| `invalid_argument` | `output` is null, or the payload is non-empty with a null data pointer. Checked first. |
| `out_of_range` | The payload is larger than `RTCM_MAX_PAYLOAD_LEN`. |
| `full` | `output_capacity < frame_size(payload.size())`. Nothing is written. |

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/rtcm.hpp"

using namespace castle::protocols::rtcm;

// An already encoded payload: message number plus data fields, padded to whole bytes.
const uint8_t payload_bytes[2U] = {0x3EU, 0x90U};

uint8_t frame[16U];
castle::size_type written = 0U;
castle::status result = encode_frame(
    castle::container::array_view<const uint8_t>(payload_bytes, sizeof(payload_bytes)),
    frame, sizeof(frame), written);
// result == castle::status::ok, written == frame_size(2U) == 8
// frame[0] == RTCM_PREAMBLE, frame[2] == 2, frame[5..7] == CRC-24Q (big-endian)
```

See [`samples/castle_ext/protocols/rtcm/rtcm.cpp`](../../../../samples/castle_ext/protocols/rtcm/rtcm.cpp).

## Constraints & Notes

- Input flow only: every RTCM 3.3 message listed in UBX-19056845 chapter 4 is of type *Input*. Enabling RTCM 3 input on a port (`CFG-<port>INPROT-RTCM3X`), the DF003 station filter (`CFG-RTCM-DF003_IN*`) and UBX-RXM-RTCM output are UBX configuration items; send them with `ubx::messages::cfg_valset` (see [`ubx_config`](../ubx/ubx_config.md)).
- CRC-24Q: polynomial `0x864CFB`, initial value 0, no reflection, no final XOR; check value `0xCDE703` for `"123456789"`, identical to the table-driven RTCM reference implementations (RTKLIB, gnssplusplus) and pyrtcm.

- Encode only: there is no RTCM decoder, frame validator or stream parser in Castle. Use an external tool (for example `pyrtcm`) to inspect frames.
- No heap allocation, exceptions, RTTI, virtual dispatch or STL containers.
- `encode_frame()` never writes outside `output` and writes nothing when it reports a failure.
- On failure, `bytes_written` is `0`; never transmit `output` after a failure from `encode_message()` because bytes already written are undefined.
- The payload view and `output` must not overlap in `encode_frame()`.
- Functions are stateless and thread-safe as long as the buffers are not shared.
- Complexity: O(payload size) for CRC and copy; at most 1029 bytes per frame.
