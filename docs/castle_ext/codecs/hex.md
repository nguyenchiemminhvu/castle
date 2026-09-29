# Hexadecimal Codec

## Overview
Converts bytes to two-character hexadecimal text and back. Encoding can use lowercase or uppercase digits; decoding accepts either form.

## Header
`#include "castle_ext/codecs/hex.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)

## Public API
| API | Description |
|---|---|
| `hex_encoded_size(input_size)` | Returns `input_size * 2`. |
| `hex_encode(input, input_size, output, output_capacity, output_size, uppercase)` | Encodes bytes without a null terminator. |
| `hex_decode(input, input_size, output, output_capacity, output_size)` | Decodes even-length hexadecimal text. |

## Usage Example
```cpp
#include "castle_ext/codecs/hex.hpp"
#include <assert.h>

int main()
{
    const uint8_t data[] = {0xCAU, 0xFEU};
    char text[4] = {};
    uint8_t restored[2] = {};
    castle::size_type n = 0U;
    castle::size_type m = 0U;
    assert(castle::codecs::hex_encode(data, 2U, text, sizeof(text), n, true) == castle::status::ok);
    assert(castle::codecs::hex_decode(text, n, restored, sizeof(restored), m) == castle::status::ok);
    assert(restored[0] == 0xCAU && restored[1] == 0xFEU);
    return 0;
}
```

## Constraints & Notes
The decoder rejects odd-length input and invalid characters. Output is caller-owned and not null-terminated. Complexity is O(size).
