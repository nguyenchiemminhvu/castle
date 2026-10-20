# UBX MON-SPAN

## Overview

Model of UBX-MON-SPAN (0x0a 0x31), a basic spectrum analyzer output (periodic/polled): one 256-bin spectrum per RF path. A 4-byte header carrying `numRfBlocks` is followed by that many 272-byte RF blocks (`4 + numRfBlocks·272`). The amplitudes are for comparative analysis only; the PGA gain is not included in the spectrum.

## Header

`#include "castle_ext/protocols/ubx/messages/mon_span.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../core/compiler.md)
- [castle/core/types.hpp](../../../../core/types.md)
- [castle/container/array.hpp](../../../../container/array.md)
- [castle/container/vector.hpp](../../../../container/vector.md)
- [castle_ext/protocols/ubx/ubx.hpp](../ubx.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../payload_writer.md)
- [detail/repeated_group.hpp](detail/repeated_group.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `mon_span_rf_block` | One 272-byte RF block (`wire_size = 272`, `layout()`). |
| `mon_span<MaxRfBlocks = 4>` | Message model; stores up to `MaxRfBlocks` RF blocks. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `MON_SPAN_SPECTRUM_BINS` | `256` | Bins per spectrum (namespace scope). |
| `mon_span::msg_class` / `msg_id` | `0x0A` / `0x31` | UBX class / ID. |
| `mon_span::header_length` / `block_length` | `4` / `272` | Header and RF block sizes. |

### `mon_span` fields

| Field | Type | Offset | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | 0 | Message version (`0x00`). |
| `num_rf_blocks` | `uint8_t` | 1 | On-wire RF block count; may exceed `rf_blocks.size()`. Encode writes `rf_blocks.size()`. |
| (reserved0) | `U1[2]` | 2 | Skipped on decode, written as zero. |
| `rf_blocks` | `castle::container::vector<mon_span_rf_block, MaxRfBlocks>` | 4 | RF blocks in wire order. |

### `mon_span_rf_block` fields

| Field | Type | Offset | Unit | Description |
| --- | --- | --- | --- | --- |
| `spectrum` | `array<uint8_t, 256>` | 4 + n·272 | dB | Amplitude per bin. |
| `span` | `uint32_t` | 260 + n·272 | Hz | Spectrum span. |
| `res` | `uint32_t` | 264 + n·272 | Hz | Resolution (bin width). |
| `center` | `uint32_t` | 268 + n·272 | Hz | Center of the span. |
| `pga` | `uint8_t` | 272 + n·272 | dB | Programmable gain amplifier gain. |
| (reserved1) | `U1[3]` | 273 + n·272 | - | Skipped on decode, written as zero. |

### Functions

| Function | Description |
| --- | --- |
| `static bool mon_span::matches(const message_view&)` | `true` for class `0x0A` / ID `0x31`. |
| `static bool mon_span::decode(const message_view&, mon_span&)` | `false` for wrong class/ID or a length other than `4 + numRfBlocks·272`. |
| `void mon_span::encode(payload_writer&) const` | Writes the header (count from `rf_blocks.size()`) and every RF block. |
| `double mon_span_rf_block::bin_frequency_hz(size_type bin) const` | Spec formula `center + span·(bin − 128)/256` [Hz]. |


## Usage Example

```cpp
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void on_message(const message_view& raw) {
    mon_span<2U> span{};
    if (mon_span<2U>::decode(raw, span)) {
        for (const mon_span_rf_block& block : span.rf_blocks) {
            double lowest_bin_hz = block.bin_frequency_hz(0U);
            uint8_t level_db = block.spectrum[0U];
            (void)lowest_bin_hz; (void)level_db;
        }
    }
}
```

The full, runnable example is `samples/castle_ext/protocols/ubx/messages/mon_span.cpp`.

## Constraints & Notes

- Payload length must be exactly `4 + numRfBlocks·272` (`has_group_length`). A payload whose length disagrees with the declared `numRfBlocks` is rejected (the previous implementation ignored the declared count).
- RF blocks beyond `MaxRfBlocks` are skipped; `num_rf_blocks` still reports the on-wire count. Each stored block is 272 bytes of state, so `mon_span<4>` is about 1.1 KiB.
- Use `rf_blocks.size()` for the number of stored blocks (previously `num_rf_blocks` meant the stored count).
- No heap allocation: every member lives in the value itself (fixed-capacity Castle containers only); no exceptions, no RTTI.
- `decode()` sets `out` to a value-initialised object first and resets it again on any failure, so callers never observe partial state.
- `message_view` is non-owning; its payload must stay valid during `decode()`. Decoded values own their data and may outlive the frame.
- Thread-safety: none. Values are plain value types; concurrent access to one instance needs external synchronisation.
