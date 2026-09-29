# Camellia

## Overview
Camellia is a 128-bit block cipher supporting 128-, 192-, and 256-bit keys. Castle exposes it as `camellia<KeyBits>` with a compile-time key size, fixed inline key storage, raw block operations, and in-place CTR processing suitable for deterministic embedded firmware.

## Header
`#include "castle_ext/crypto/camellia.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)
- [`../../../include/castle/utility/bytes.hpp`](../../../include/castle/utility/bytes.hpp)

## Public API
| API | Description |
|---|---|
| `castle::crypto::camellia<KeyBits>` | Camellia instance where `KeyBits` is exactly 128, 192, or 256. |
| `block_size`, `key_size`, `rounds` | Compile-time constants; block size is 16 bytes, key size is `KeyBits / 8`, and rounds are 18 for 128-bit keys or 24 otherwise. |
| `camellia<KeyBits>()` / `camellia<KeyBits>(key)` | Constructs with an all-zero key or a supplied key. A null constructor key selects the zero key for consistency with Castle AES. |
| `set_key(key)` | Replaces and expands the key. Returns `status::invalid_argument` for a null key, otherwise `status::ok`. |
| `encrypt_block(input, output)` | Encrypts one 16-byte block. Input and output may alias. |
| `decrypt_block(input, output)` | Decrypts one 16-byte block. Input and output may alias. |
| `crypt_ctr(data, size, counter_block)` | Applies CTR to any byte length in place, without padding. Returns a status and advances the counter. |

## Usage Example
```cpp
#include "castle_ext/crypto/camellia.hpp"
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

    castle::crypto::camellia<128> camellia(key);
    camellia.encrypt_block(plain, cipher);
    camellia.decrypt_block(cipher, restored);

    for (unsigned i = 0U; i < 16U; ++i) assert(restored[i] == plain[i]);
    assert(camellia.crypt_ctr(message, sizeof(message), counter) == castle::status::ok);
    assert(camellia.crypt_ctr(message, sizeof(message), counter_for_decrypt) == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
`KeyBits` is constrained at compile time to 128, 192, or 256. There is no dynamic allocation, exception handling, RTTI, STL dependency, or virtual dispatch. The key schedule is stored inline and wiped during object destruction. `crypt_ctr` accepts partial final blocks without padding, increments the final four counter bytes as a big-endian 32-bit integer, and returns `status::out_of_range` before modifying the data or counter if that increment would wrap. Keep the 16-byte counter block separate from the data buffer. Calls discard unused keystream bytes at the end of each call, so split a message only at block boundaries if the output must match a single call. Never reuse a counter block with the same key. CTR does not authenticate ciphertext; use an authenticated construction when integrity is required. The S-box implementation uses lookup tables and should not be assumed to provide cache-independent constant-time behavior. The implementation follows RFC 3713 key scheduling and data-randomization definitions.
