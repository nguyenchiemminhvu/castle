# SM4

## Overview
SM4 is a 128-bit block cipher with a 128-bit key and 32 Feistel-style transformation rounds. Castle provides a fixed-storage, heap-free implementation intended for embedded systems, with raw block operations and an in-place CTR helper. Authentication remains a separate construction.

## Header
`#include "castle_ext/crypto/sm4.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)
- [`../../../include/castle/utility/bytes.hpp`](../../../include/castle/utility/bytes.hpp)

## Public API
| API | Description |
|---|---|
| `castle::crypto::sm4` | SM4 block-cipher object with an inline expanded key schedule. |
| `block_size`, `key_size`, `rounds` | Compile-time constants: 16, 16, and 32. |
| `sm4()` / `sm4(key)` | Constructs the cipher with an all-zero key or a supplied 128-bit key. A null constructor key selects the zero key for consistency with Castle AES. |
| `set_key(key)` | Replaces the key and expands it. Returns `status::invalid_argument` for a null key, otherwise `status::ok`. |
| `encrypt_block(input, output)` | Encrypts one 16-byte block. Input and output may alias. |
| `decrypt_block(input, output)` | Decrypts one 16-byte block. Input and output may alias. |
| `crypt_ctr(data, size, counter_block)` | Applies CTR to any byte length in place, without padding. Returns a status and advances the counter. |

## Usage Example
```cpp
#include "castle_ext/crypto/sm4.hpp"
#include <assert.h>

int main()
{
    const uint8_t key[16] = {};
    const uint8_t plain[16] = {};
    uint8_t cipher[16] = {};
    uint8_t restored[16] = {};
    uint8_t message[37] = {};
    uint8_t counter[16] = {};
    uint8_t counter_for_decrypt[16] = {};

    castle::crypto::sm4 sm4(key);
    assert(sm4.set_key(key) == castle::status::ok);
    sm4.encrypt_block(plain, cipher);
    sm4.decrypt_block(cipher, restored);

    for (unsigned i = 0U; i < 16U; ++i) assert(restored[i] == plain[i]);
    assert(sm4.crypt_ctr(message, sizeof(message), counter) == castle::status::ok);
    assert(sm4.crypt_ctr(message, sizeof(message), counter_for_decrypt) == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
No dynamic allocation, exceptions, RTTI, STL, or virtual functions are used. `crypt_ctr` accepts partial final blocks without padding, increments the final four counter bytes as a big-endian 32-bit integer, and returns `status::out_of_range` before modifying the data or counter if that increment would wrap. Keep the 16-byte counter block separate from the data buffer. Calls discard unused keystream bytes at the end of each call, so split a message only at block boundaries if the output must match a single call. Never reuse a counter block with the same key. CTR does not authenticate ciphertext; use an authenticated construction when integrity is required. The implementation uses S-box lookup tables and should not be assumed constant-time against cache-based side-channel attacks on processors with observable caches.
