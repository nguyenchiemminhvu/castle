# UBX ESF data word

## Overview

Shared codec for the 32-bit `data` word used by UBX-ESF-MEAS and UBX-ESF-RAW: `dataField` in bits 23..0
(two's-complement 24-bit) and `dataType` above it (6 bits in ESF-MEAS, 8 bits in ESF-RAW). It is written once here
so the sign extension and range checks cannot drift between the two messages.

## Header

`#include "castle_ext/protocols/ubx/messages/detail/esf_data.hpp"`

## Dependencies

- [castle/core/compiler.hpp](../../../../../core/compiler.md)
- [castle_ext/protocols/ubx/payload_reader.hpp](../../payload_reader.md)
- [castle_ext/protocols/ubx/payload_writer.hpp](../../payload_writer.md)

## Public API

All in `castle::protocols::ubx::messages::detail`.

| Name | Description |
| --- | --- |
| `ESF_DATA_FIELD_MASK` / `ESF_DATA_FIELD_SIGN` / `ESF_DATA_TYPE_SHIFT` | `0x00FFFFFF` / `0x00800000` / `24`. |
| `ESF_DATA_FIELD_MIN` / `ESF_DATA_FIELD_MAX` | `-0x800000` / `0x7FFFFF`. |
| `constexpr int32_t esf_data_field(uint32_t word)` | Bits 23..0 sign-extended; type bits never leak in. |
| `template <uint8_t TypeMask> void esf_data_word(payload_reader&, uint8_t& type, int32_t& value)` | Reads one word. |
| `template <uint8_t TypeMask> void esf_data_word(payload_writer&, const uint8_t& type, const int32_t& value)` | Writes one word; latches `status::out_of_range` if the type or value does not fit. |

## Usage Example

```cpp
template <typename Self, typename Stream>
static void layout(Self& self, Stream& stream) noexcept
{
    detail::esf_data_word<0x3FU>(stream, self.data_type, self.data_value);
}
```

See `samples/castle_ext/protocols/ubx/messages/esf_meas.cpp` and `esf_raw.cpp`.

## Constraints & Notes

- Internal helper; message users go through `esf_meas` / `esf_raw`.
- No state; free functions. Thread-safety: none required (pure functions over caller-owned streams).
