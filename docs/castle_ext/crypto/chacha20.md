# ChaCha20

## Overview
Provides the RFC 8439/IETF ChaCha20 stream cipher using a 256-bit key, 96-bit nonce, and 32-bit block counter.

## Header
`#include "castle_ext/crypto/chacha20.hpp"`

## Dependencies
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)
- [`../../../include/castle_ext/crypto/detail/bytes.hpp`](../../../include/castle_ext/crypto/detail/bytes.hpp)

## Public API
| API | Description |
|---|---|
| `key_size`, `nonce_size`, `block_size` | Compile-time constants: 32, 12, and 64 bytes. |
| `chacha20()` / `chacha20(key, nonce, counter)` | Constructs a cipher instance. |
| `reset(key, nonce, counter)` | Reinitializes state; null inputs return `status::invalid_argument`. |
| `set_counter(counter)` / `counter()` | Sets or reads the current 32-bit block counter. |
| `crypt(data, size)` | Encrypts/decrypts arbitrary-length data in place; O(size). |

## Usage Example
```cpp
#include "castle_ext/crypto/chacha20.hpp"
#include <assert.h>

int main()
{
    const uint8_t key[castle::crypto::chacha20::key_size] = {};
    const uint8_t nonce[castle::crypto::chacha20::nonce_size] = {};
    uint8_t message[8] = {1U,2U,3U,4U,5U,6U,7U,8U};
    const uint8_t original[8] = {1U,2U,3U,4U,5U,6U,7U,8U};
    castle::crypto::chacha20 cipher(key, nonce);
    cipher.crypt(message, sizeof(message));
    cipher.reset(key, nonce);
    cipher.crypt(message, sizeof(message));
    for (unsigned i = 0U; i < 8U; ++i) assert(message[i] == original[i]);
    return 0;
}
```

## Constraints & Notes
No dynamic allocation. Never reuse a key/nonce/counter keystream for different plaintexts. The state is incremental, so a message may be split across multiple `crypt` calls. This implementation does not authenticate data.
