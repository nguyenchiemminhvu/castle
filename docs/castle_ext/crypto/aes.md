# AES

## Overview
Provides AES-128, AES-192, and AES-256 as a fixed-size block cipher with raw block operations and a CTR helper.

## Header
`#include "castle_ext/crypto/aes.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)

## Public API
| API | Description |
|---|---|
| `castle::crypto::aes<KeyBits>` | AES instance for `KeyBits` = 128, 192, or 256. |
| `block_size`, `key_size`, `rounds` | Compile-time sizing constants. |
| `aes()` / `aes(key)` | Constructs using a zero key or supplied key. |
| `set_key(key)` | Expands a new key; null returns `status::invalid_argument`. |
| `encrypt_block(input, output)` | Encrypts one 16-byte block. |
| `decrypt_block(input, output)` | Decrypts one 16-byte block. |
| `crypt_ctr(data, size, counter_block)` | Applies AES-CTR keystream in place and increments the final 32 bits of the counter block as a big-endian integer. Returns `status::invalid_argument` for a null required pointer or `status::out_of_range` if the counter would wrap. |

## Usage Example
```cpp
#include "castle_ext/crypto/aes.hpp"
#include <assert.h>

int main()
{
    const uint8_t key[16] = {};
    const uint8_t plain[16] = {};
    uint8_t cipher[16] = {};
    uint8_t restored[16] = {};
    castle::crypto::aes<128> aes(key);
    aes.encrypt_block(plain, cipher);
    aes.decrypt_block(cipher, restored);
    for (unsigned i = 0U; i < 16U; ++i) assert(restored[i] == plain[i]);
    return 0;
}
```

## Constraints & Notes
No dynamic allocation, exceptions, RTTI, or virtual functions. Raw block encryption is a primitive and does not provide authentication. `crypt_ctr` provides confidentiality only; callers must ensure counter/nonce uniqueness under each key. The counter block must not overlap the data buffer. Counter exhaustion is checked before either buffer is modified. The implementation uses conventional S-box table lookups; on cached processors this should not be assumed to be side-channel resistant.
