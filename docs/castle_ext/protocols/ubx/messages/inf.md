# UBX INF

## Overview

Typed models of the UBX-INF text messages (class 0x04): ERROR (0x00), WARNING (0x01), NOTICE (0x02), TEST (0x03) and DEBUG (0x04). The receiver sends them as output. The whole payload (`[0..n]` bytes, possibly empty) is an ASCII string that is not NUL-terminated on the wire, and the message ID carries the severity.

- `basic_inf<MaxText, MsgId>` models one severity, so a decoder registry can subscribe to a single one (`inf_error<>`, `inf_warning<>`, `inf_notice<>`, `inf_test<>`, `inf_debug<>`).
- `inf<MaxText>` matches all five IDs and records which one arrived.

Both use the same text decoder.

## Header

`#include "castle_ext/protocols/ubx/messages/inf.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/string.hpp`](../../../../container/string.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- `stdint.h`

## Public API

### Types

| Type | Description |
| --- | --- |
| `enum class inf_subtype : uint8_t` | `error`, `warning`, `notice`, `test`, `debug`. Each enumerator's value is its message ID. |
| `basic_inf<MaxText, MsgId>` | Single-severity model (`MsgId` 0x00..0x04, checked with `static_assert`). |
| `inf_error<MaxText = 256>` ... `inf_debug<MaxText = 256>` | Aliases of `basic_inf` for each ID. |
| `inf<MaxText = 256>` | Any-severity model. |

### Constants

| Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `basic_inf::msg_class`, `inf::msg_class` | `uint8_t` | `UBX_CLASS_INF` (0x04) | UBX class. |
| `basic_inf::msg_id` | `uint8_t` | `MsgId` | UBX message ID. |
| `basic_inf::subtype` | `inf_subtype` | `static_cast<inf_subtype>(MsgId)` | Severity of this model. |

### Fields

| Model | Field | Type | Description |
| --- | --- | --- | --- |
| both | `text` | `castle::container::string<MaxText>` | Up to `MaxText` characters, stopping at the first NUL if one is present. |
| `inf` | `message_id` | `uint8_t` | ID that was received. |
| `inf` | `subtype` | `inf_subtype` | Severity (same value as `message_id`). |

### Functions

| Function | Returns | Description |
| --- | --- | --- |
| `static bool basic_inf::matches(const message_view& raw)` | `bool` | True for class 0x04 / `MsgId` only. |
| `static bool basic_inf::decode(const message_view& raw, basic_inf& out)` | `bool` | Copies the text. Any payload length, including 0, is valid. For another class/ID returns `false` and leaves `out` value-initialised. |
| `void basic_inf::encode(payload_writer& writer) const` | - | Writes the stored characters without a terminator. Used for loop-back and simulation through `encode_message()`. |
| `static bool inf::matches(const message_view& raw)` | `bool` | True for class 0x04 and IDs 0x00..0x04. |
| `static bool inf::decode(const message_view& raw, inf& out)` | `bool` | Records `message_id` / `subtype` and copies the text. Any payload length is valid. For another class/ID returns `false` and leaves `out` value-initialised. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/inf.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void on_message(const message_view& raw)
{
    inf_error<128U> error{};
    if (inf_error<128U>::decode(raw, error))
    {
        const char* line = error.text.c_str(); // NUL-terminated copy
        static_cast<void>(line);
    }
}
```

A complete, self-checking example is in [`samples/castle_ext/protocols/ubx/messages/inf.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/inf.cpp).

## Constraints & Notes

- Capacity: text longer than `MaxText` is cut off; nothing is allocated. `string<MaxText>` stores `MaxText + 1` bytes inline, so its `c_str()` is always NUL-terminated.
- An empty INF frame reaches `decode()` as a null payload view. It decodes to an empty `text` without creating a payload reader.
- Fixed-size value types: no heap, exceptions, RTTI or virtual functions.
- `message_view` is non-owning. The decoded struct owns a copy of the text.
- Thread-safety: none. Instances are plain values and are not thread-safe.
