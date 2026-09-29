# XOR Cipher

## Overview
Repeating-key XOR cipher: each data byte is XORed with `key[offset % key_size]`. Encryption and decryption are the same operation. Provides stateless one-shot functions and an incremental `xor` that stores its key inline.

> **Warning:** XOR with a repeating key is obfuscation, not encryption. It provides no confidentiality against known-plaintext or frequency analysis and no integrity. For protection of sensitive data use [`aes`](aes.md) or [`chacha20`](chacha20.md).

## Header
`#include "castle_ext/crypto/xor.hpp"`

## Dependencies
- [`../../container/array.md`](../../container/array.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)
- [`../../../include/castle/utility/bytes.hpp`](../../../include/castle/utility/bytes.hpp) (`secure_zero`)

## Public API
Free functions (stateless, all return `status`, `CASTLE_NODISCARD`, key restarts at index 0):

| API | Description |
|---|---|
| `xor_encrypt(data, size, key, key_size)` | In-place encrypt; O(size). |
| `xor_decrypt(data, size, key, key_size)` | In-place decrypt. |
| `xor_encrypt(input, output, size, key, key_size)` | Out-of-place encrypt; `output` may equal `input`, partial overlap is not allowed. |
| `xor_decrypt(input, output, size, key, key_size)` | Out-of-place decrypt. |

XOR is symmetric, so `xor_decrypt` performs the same operation as `xor_encrypt`; both names exist so call sites read clearly. `invalid_argument` is returned for a null/empty key or null buffers with `size != 0`; nothing is written on error.

`template <size_type MaxKeySize = 32> class xor_cipher` (the name `xor` is a reserved C++ alternative token for `^`, so it cannot be used as an identifier):

| API | Description |
|---|---|
| `max_key_size` | Compile-time key capacity. |
| `xor_cipher()` | Unconfigured cipher. |
| `xor_cipher(key, key_size)` / `xor_cipher(key_array)` | Loads a key; an invalid key leaves the cipher unconfigured. The array form checks `N <= MaxKeySize` at compile time. |
| `set_key(key, key_size)` / `set_key(key_array)` | Replaces the key and rewinds. `invalid_argument` for null/zero length, `out_of_range` when longer than `MaxKeySize`; the previous key is kept on error. |
| `encrypt(data, size)` / `encrypt(input, output, size)` | Encrypts and advances the stream. `not_configured` without a key, `invalid_argument` for null buffers with `size != 0`; state is unchanged on error. |
| `decrypt(data, size)` / `decrypt(input, output, size)` | Same operation as `encrypt`, named for readability. |
| `seek(offset)` | Positions the stream at an absolute byte offset (`offset % key_size`). |
| `reset()` | Rewinds to position 0, keeping the key. Call it between encrypting and decrypting a message with the same object. |
| `clear()` | Wipes the key (volatile writes) and unconfigures; also run by the destructor. |
| `configured()`, `key_size()`, `position()` | State queries. |

## Usage Example
```cpp
#include "castle_ext/crypto/xor.hpp"
#include <assert.h>

int main()
{
    const uint8_t key[3] = {0x4BU, 0x45U, 0x59U};
    uint8_t data[6] = {1U, 2U, 3U, 4U, 5U, 6U};

    // One-shot
    assert(castle::crypto::xor_encrypt(data, sizeof(data), key, sizeof(key)) == castle::status::ok);
    assert(castle::crypto::xor_decrypt(data, sizeof(data), key, sizeof(key)) == castle::status::ok);

    // Chunked
    castle::crypto::xor_cipher<16U> cipher(key);
    assert(cipher.encrypt(data, 4U) == castle::status::ok);
    assert(cipher.encrypt(data + 4U, 2U) == castle::status::ok);

    cipher.reset();
    assert(cipher.decrypt(data, sizeof(data)) == castle::status::ok);
    assert(data[0] == 1U && data[5] == 6U);
    return 0;
}
```

## Constraints & Notes
Header-only, no dynamic allocation, no exceptions, no RTTI, no virtual calls. `xor_cipher` is copyable; copies carry the key, so `clear()` each copy that must not retain it. Key bytes must never be treated as secret strength: keys shorter than the message repeat and are recoverable from known plaintext.
