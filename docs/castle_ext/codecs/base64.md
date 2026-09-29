# Base64

## Overview
Implements RFC 4648 Base64 with optional padding and an optional URL-safe alphabet (`-` and `_`).

## Header
`#include "castle_ext/codecs/base64.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)

## Public API
| API | Description |
|---|---|
| `base64_encoded_size(input_size, padding)` | Returns the exact encoded character count. |
| `base64_decoded_size(input_size)` | Returns the maximum decoded byte count. |
| `base64_encode(input, input_size, output, output_capacity, output_size, padding, url_safe)` | Encodes bytes into caller-owned storage. |
| `base64_decode(input, input_size, output, output_capacity, output_size, url_safe, allow_unpadded)` | Decodes strict Base64 and validates unused bits. |

## Usage Example
```cpp
#include "castle_ext/codecs/base64.hpp"
#include <assert.h>

int main()
{
    const uint8_t text[] = "foobar";
    char encoded[16] = {};
    uint8_t decoded[6] = {};
    castle::size_type n = 0U;
    castle::size_type m = 0U;
    assert(castle::codecs::base64_encode(text, 6U, encoded, sizeof(encoded), n) == castle::status::ok);
    assert(castle::codecs::base64_decode(encoded, n, decoded, sizeof(decoded), m) == castle::status::ok);
    assert(m == 6U);
    return 0;
}
```

## Constraints & Notes
No heap allocation and no terminating null byte. Malformed padding and non-zero unused bits are rejected. URL-safe mode is explicit and must match the decoder configuration.
