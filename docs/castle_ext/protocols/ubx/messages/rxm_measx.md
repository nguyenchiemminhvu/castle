# UBX RXM-MEASX

## Overview

Defines a bounded UBX RXM-MEASX measurement model with per-satellite code/carrier/doppler data.

## Header

`#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/array.hpp`](../../../../container/array.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- `stdint.h`

## Public API

| API | Description |
| --- | --- |
| `rxm_measx_sv` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `rxm_measx<MaxSvs=64U>` | Typed, value-owning decoded model with fixed-capacity storage only. |
| `rxm_measx::msg_class` | `static constexpr uint8_t` protocol/configuration constant (`UBX_CLASS_RXM`). |
| `rxm_measx::msg_id` | `static constexpr uint8_t` protocol/configuration constant (`UBX_ID_RXM_MEASX`). |
| `rxm_measx::header_length` | `static constexpr castle::size_type` protocol/configuration constant (`44U`). |
| `rxm_measx::block_length` | `static constexpr castle::size_type` protocol/configuration constant (`24U`). |
| `rxm_measx::matches()` | Returns true when the raw view identifies this message/sentence type. |
| `rxm_measx::decode()` | Validates the raw payload/field count and fills the output structure using deterministic cursor/reader operations. |

### `rxm_measx_sv` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `gnss_id` | `uint8_t` | `0U` | GNSS constellation identifier. |
| `sv_id` | `uint8_t` | `0U` | Satellite identifier. |
| `cno` | `uint8_t` | `0U` | Carrier-to-noise density ratio / signal-strength field. |
| `mpath_indic` | `uint8_t` | `0U` | Decoded `mpath indic` field. |
| `doppler_ms` | `int32_t` | `0` | Decoded `doppler ms` field. |
| `doppler_hz` | `int32_t` | `0` | Decoded `doppler hz` field. |
| `whole_chips` | `uint16_t` | `0U` | Decoded `whole chips` field. |
| `frac_chips` | `uint16_t` | `0U` | Decoded `frac chips` field. |
| `code_phase` | `uint32_t` | `0U` | Decoded `code phase` field. |
| `int_code_phase` | `uint8_t` | `0U` | Decoded `int code phase` field. |
| `pseu_range_rms_err` | `uint8_t` | `0U` | Decoded `pseu range rms err` field. |

### `rxm_measx` data members

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | `0U` | Protocol/message version field. |
| `gps_tow` | `uint32_t` | `0U` | Decoded `gps tow` field. |
| `glo_tow` | `uint32_t` | `0U` | Decoded `glo tow` field. |
| `bds_tow` | `uint32_t` | `0U` | Decoded `bds tow` field. |
| `qzss_tow` | `uint32_t` | `0U` | Decoded `qzss tow` field. |
| `gps_tow_acc` | `uint16_t` | `0U` | Associated accuracy/uncertainty value decoded from the payload. |
| `glo_tow_acc` | `uint16_t` | `0U` | Associated accuracy/uncertainty value decoded from the payload. |
| `bds_tow_acc` | `uint16_t` | `0U` | Associated accuracy/uncertainty value decoded from the payload. |
| `qzss_tow_acc` | `uint16_t` | `0U` | Associated accuracy/uncertainty value decoded from the payload. |
| `num_svs` | `uint8_t` | `0U` | Number of satellite records in the payload. |
| `flags` | `uint8_t` | `0U` | Protocol-defined packed flags. |
| `svs` | `castle::container::array<rxm_measx_sv,MaxSvs>` | `—` | Fixed-capacity satellite records. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"

using castle_ext::protocols::ubx::message_view;
using castle_ext::protocols::ubx::messages::rxm_measx;

void handle(const message_view& raw) {
    rxm_measx<16U> value{};
    if (rxm_measx::decode(raw, value)) {
        // The fixed-capacity arrays contain at most the configured bound.
    }
}
```

## Constraints & Notes

- All decoded members are stored directly in the destination object or fixed-capacity Castle containers; no heap allocation is required.
- `message_view` is non-owning and its payload must remain valid while decoding or while any field view aliases parser storage.
- UBX payload bytes are consumed through `payload_reader`; malformed lengths cause `decode()` to return false instead of exposing partial state.
- Payload length must be at least the fixed header length; variable records are then validated against their encoded block count.
- Wire record counts are accepted only when the payload length exactly matches the encoded header-plus-block calculation; storage is capped by the template parameter.
