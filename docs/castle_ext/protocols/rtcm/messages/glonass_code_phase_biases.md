# RTCM GLONASS code-phase biases (1230)

## Overview

`glonass_code_phase_biases` builds RTCM 3 message 1230. It lets a rover align GLONASS observations from receivers of different manufacturers. `signals_mask` selects which of the four biases (L1 C/A, L1 P, L2 C/A, L2 P) are present on the wire; a bias whose mask bit is clear is not written, so the payload length depends on the mask.

This is an **encode-only** model: it produces the message payload for `encode_message()` and there is no decoder.

## Header

`#include "castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../../core/compiler.md)
- [`core/types.hpp`](../../../../core/types.md)
- [`castle_ext/protocols/rtcm/bit_writer.hpp`](../bit_writer.md)
- [`castle_ext/protocols/rtcm/rtcm.hpp`](../rtcm.md)

## Public API

All names live in `castle::protocols::rtcm::messages`.

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `glonass_code_phase_biases::MESSAGE_TYPE` | `RTCM_TYPE_GLO_BIASES` (1230) | Message number (DF002), written first. |
| `glonass_code_phase_biases::MASK_L1_CA` | `0x08` | L1 C/A bias present (written first). |
| `glonass_code_phase_biases::MASK_L1_P` | `0x04` | L1 P bias present. |
| `glonass_code_phase_biases::MASK_L2_CA` | `0x02` | L2 C/A bias present. |
| `glonass_code_phase_biases::MASK_L2_P` | `0x01` | L2 P bias present (written last). |
| `glonass_code_phase_biases::RESOLUTION_M` | `0.02` | Resolution of the bias fields, in metres. |

### Fields

| Field | Type | Wire format | Description |
| --- | --- | --- | --- |
| `station_id` | `uint16_t` | DF003, 12 bit | Reference station ID, 0..4095. |
| `code_phase_bias_indicator` | `bool` | DF421 | `true` when the station observations are aligned to GLONASS time. Followed by 3 reserved zero bits. |
| `signals_mask` | `uint8_t` | DF422, 4 bit | Which biases are present, `MASK_*` combined; valid range 0..15. |
| `l1_ca_bias` | `int16_t` | DF423, 16 bit signed | L1 C/A bias, units of 0.02 m. Written only with `MASK_L1_CA`. |
| `l1_p_bias` | `int16_t` | DF424 | L1 P bias. Written only with `MASK_L1_P`. |
| `l2_ca_bias` | `int16_t` | DF425 | L2 C/A bias. Written only with `MASK_L2_CA`. |
| `l2_p_bias` | `int16_t` | DF426 | L2 P bias. Written only with `MASK_L2_P`. |

### Functions

| Function | Returns | Description | Complexity |
| --- | --- | --- | --- |
| `static constexpr bool accepts(uint16_t type)` | `bool` | `type == MESSAGE_TYPE`, i.e. `true` for 1230 only. | O(1) |
| `void encode(bit_writer& writer) const` | - | Writes `MESSAGE_TYPE`, the header fields and the selected biases. Used by `encode_message()`. | O(1) |

### Encode results

`encode_message(biases, output, capacity, written)` returns `ok` (payload `4 + 2 * (set mask bits)` bytes, frame 10..22 bytes), `invalid_argument` for a `nullptr` output, `out_of_range` for `signals_mask` > 15 or `station_id` > 4095, and `full` when the frame does not fit. `written` is `0` on failure.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

castle::status encode_biases(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    glonass_code_phase_biases biases{};
    biases.station_id = 7U;
    biases.code_phase_bias_indicator = true;
    biases.signals_mask = glonass_code_phase_biases::MASK_L1_CA | glonass_code_phase_biases::MASK_L2_P;
    biases.l1_ca_bias = -100;   // -2.00 m
    biases.l2_p_bias = 25;      // +0.50 m
    return encode_message(biases, frame, capacity, written);
}
```

See [`samples/castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.cpp`](../../../../../samples/castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.cpp).

## Constraints & Notes

- Encode only: there is no parser or decoder for 1230.
- Bias values are raw `int16_t` counts of 0.02 m (`+-655.36 m` range); there are no metre conversion helpers.
- A bias whose mask bit is clear is ignored even if non-zero.
- The reserved bits are always zero.
- No heap allocation, exceptions, RTTI or STL.
- Thread-safety: none. Instances are plain values.
