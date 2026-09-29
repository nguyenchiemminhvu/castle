# Base32

## Overview
Implements the RFC 4648 Base32 alphabet using caller-owned buffers. Encoding emits uppercase text; decoding accepts uppercase and lowercase.

## Header
`#include "castle_ext/codecs/base32.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)

## Public API
| API | Description |
|---|---|
| `base32_encoded_size(input_size, padding)` | Returns the exact encoded character count. |
| `base32_decoded_size(input_size)` | Returns the maximum decoded byte count. |
| `base32_encode(input, input_size, output, output_capacity, output_size, padding)` | Encodes into caller-owned storage; no terminator is written. |
| `base32_decode(input, input_size, output, output_capacity, output_size, allow_unpadded)` | Decodes and validates canonical unused bits. |

## Usage Example
```cpp
#include "castle_ext/codecs/base32.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "foobar";
    char encoded[16] = {};
    uint8_t decoded[6] = {};
    castle::size_type n = 0U;
    castle::size_type m = 0U;
    assert(castle::codecs::base32_encode(text, 6U, encoded, sizeof(encoded), n) == castle::status::ok);
    assert(castle::codecs::base32_decode(encoded, n, decoded, sizeof(decoded), m) == castle::status::ok);
    assert(m == 6U);
    return 0;
}
```

## Constraints & Notes
Returns `status::full` before writing when the output buffer is too small. Output is not null-terminated. Complexity is O(input size), with no dynamic allocation. Padded encoding is the default; unpadded decoding is enabled by default.
