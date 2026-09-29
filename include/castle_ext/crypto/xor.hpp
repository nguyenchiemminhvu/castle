// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file xor.hpp
 * @brief Heap-free repeating-key XOR cipher.
 *
 * Every data byte is XORed with the key byte at `(offset % key_size)`, so
 * encryption and decryption are the same operation; both names are provided for
 * readability. The stateless `xor_encrypt()`/`xor_decrypt()` suit one-shot buffers;
 * `xor_cipher` keeps the key and stream position so a message can be processed in
 * arbitrarily sized chunks.
 *
 * @warning XOR with a repeating key is obfuscation, not encryption. It offers no
 * confidentiality against known-plaintext or frequency analysis and no integrity.
 * Do not use it to protect sensitive data; use `castle::crypto::aes` or
 * `castle::crypto::chacha20` instead.
 */
#ifndef CASTLE_EXT_CRYPTO_XOR_HPP
#define CASTLE_EXT_CRYPTO_XOR_HPP

#include "castle/container/array.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bytes.hpp"

#include <stdint.h>

namespace castle
{
namespace crypto
{

namespace detail
{

/**
 * @brief XORs `size` bytes with the repeating key starting at key index `position`.
 * @param position Start index in `[0, key_size)`.
 * @return Key index to continue with for the next byte.
 * @note `input` and `output` may be identical but must not partially overlap.
 */
inline size_type xor_apply(CASTLE_CONST uint8_t* input,
                           uint8_t* output,
                           size_type size,
                           CASTLE_CONST uint8_t* key,
                           size_type key_size,
                           size_type position) CASTLE_NOEXCEPT
{
    for (size_type i = 0U; i < size; ++i)
    {
        output[i] = static_cast<uint8_t>(input[i] ^ key[position]);
        ++position;
        if (position == key_size)
        {
            position = 0U;
        }
    }
    return position;
}

} // namespace detail

/**
 * @brief Encrypts `input` into `output` with a repeating key (out-of-place).
 * @param input    Plaintext; may be null only when `size == 0`.
 * @param output   Ciphertext destination of `size` bytes; may equal `input`.
 * @param size     Number of bytes to process.
 * @param key      Key bytes.
 * @param key_size Key length in bytes; must be non-zero.
 * @return `status::ok`, or `status::invalid_argument` for a null/empty key or null buffers with `size != 0`.
 * @note Complexity: O(size). Nothing is written on error.
 */
CASTLE_NODISCARD inline status xor_encrypt(CASTLE_CONST uint8_t* input,
                                           uint8_t* output,
                                           size_type size,
                                           CASTLE_CONST uint8_t* key,
                                           size_type key_size) CASTLE_NOEXCEPT
{
    if (key == 0 || key_size == 0U)
    {
        return status::invalid_argument;
    }
    if (size != 0U && (input == 0 || output == 0))
    {
        return status::invalid_argument;
    }
    static_cast<void>(detail::xor_apply(input, output, size, key, key_size, 0U));
    return status::ok;
}

/** @brief Encrypts `data` in place with a repeating key; see the out-of-place overload. */
CASTLE_NODISCARD inline status xor_encrypt(uint8_t* data,
                                           size_type size,
                                           CASTLE_CONST uint8_t* key,
                                           size_type key_size) CASTLE_NOEXCEPT
{
    return xor_encrypt(data, data, size, key, key_size);
}

/** @brief Decrypts `input` into `output`; XOR is symmetric, so this equals `xor_encrypt()` and exists for readability. */
CASTLE_NODISCARD inline status xor_decrypt(CASTLE_CONST uint8_t* input,
                                           uint8_t* output,
                                           size_type size,
                                           CASTLE_CONST uint8_t* key,
                                           size_type key_size) CASTLE_NOEXCEPT
{
    return xor_encrypt(input, output, size, key, key_size);
}

/** @brief Decrypts `data` in place; see `xor_decrypt()` out-of-place overload. */
CASTLE_NODISCARD inline status xor_decrypt(uint8_t* data,
                                           size_type size,
                                           CASTLE_CONST uint8_t* key,
                                           size_type key_size) CASTLE_NOEXCEPT
{
    return xor_encrypt(data, data, size, key, key_size);
}

/**
 * @brief Incremental XOR cipher holding its key in fixed inline storage.
 * @tparam MaxKeySize Key capacity in bytes (> 0); the runtime key length may be anything in `1..MaxKeySize`.
 *
 * The cipher tracks the key index of the next byte, so splitting a message across
 * several `crypt()` calls gives the same result as one call. Heap-free; the key
 * is wiped on `clear()` and on destruction.
 */
template <size_type MaxKeySize = 32U>
class xor_cipher
{
    static_assert(MaxKeySize > 0U, "castle::crypto::xor_cipher requires MaxKeySize > 0");

public:
    /** @brief Key capacity in bytes. */
    static CASTLE_CONSTEXPR size_type max_key_size = MaxKeySize;

    /** @brief Constructs an unconfigured cipher; `crypt()` fails with `status::not_configured` until a key is set. */
    xor_cipher() CASTLE_NOEXCEPT : key_(), key_size_(0U), position_(0U) {}

    /**
     * @brief Constructs a cipher and loads a key.
     * @note An invalid key leaves the cipher unconfigured; check `configured()`.
     */
    xor_cipher(CASTLE_CONST uint8_t* key, size_type key_size) CASTLE_NOEXCEPT
        : key_(), key_size_(0U), position_(0U)
    {
        static_cast<void>(set_key(key, key_size));
    }

    /** @brief Constructs a cipher from a fixed-size key array; `N <= MaxKeySize` is checked at compile time. */
    template <size_type N>
    explicit xor_cipher(CASTLE_CONST uint8_t (&key)[N]) CASTLE_NOEXCEPT
        : key_(), key_size_(0U), position_(0U)
    {
        static_assert(N <= MaxKeySize, "key array exceeds xor_cipher capacity");
        static_cast<void>(set_key(key, N));
    }

    ~xor_cipher() CASTLE_NOEXCEPT
    {
        clear();
    }

    /**
     * @brief Replaces the key and rewinds the stream to position 0.
     * @param key      Key bytes.
     * @param key_size Key length in bytes, `1..MaxKeySize`.
     * @return `status::ok`; `status::invalid_argument` for a null key or zero length;
     *         `status::out_of_range` when `key_size > MaxKeySize`. The previous key is kept on error.
     */
    CASTLE_NODISCARD status set_key(CASTLE_CONST uint8_t* key, size_type key_size) CASTLE_NOEXCEPT
    {
        if (key == 0 || key_size == 0U)
        {
            return status::invalid_argument;
        }
        if (key_size > MaxKeySize)
        {
            return status::out_of_range;
        }
        for (size_type i = 0U; i < key_size; ++i)
        {
            key_[i] = key[i];
        }
        key_size_ = key_size;
        position_ = 0U;
        return status::ok;
    }

    /** @brief Fixed-size-array overload of `set_key()`; the length bound is checked at compile time. */
    template <size_type N>
    CASTLE_NODISCARD status set_key(CASTLE_CONST uint8_t (&key)[N]) CASTLE_NOEXCEPT
    {
        static_assert(N <= MaxKeySize, "key array exceeds xor_cipher capacity");
        return set_key(key, N);
    }

    /** @brief Securely wipes the key and returns to the unconfigured state. */
    void clear() CASTLE_NOEXCEPT
    {
        castle::secure_zero(key_.data(), MaxKeySize);
        key_size_ = 0U;
        position_ = 0U;
    }

    /** @brief Rewinds the stream to position 0, keeping the key. */
    void reset() CASTLE_NOEXCEPT
    {
        position_ = 0U;
    }

    /**
     * @brief Moves the stream to an absolute byte offset.
     * @param offset Stream offset; the next byte uses key index `offset % key_size()`.
     * @return `status::ok` or `status::not_configured` when no key is set.
     */
    CASTLE_NODISCARD status seek(size_type offset) CASTLE_NOEXCEPT
    {
        if (key_size_ == 0U)
        {
            return status::not_configured;
        }
        position_ = offset % key_size_;
        return status::ok;
    }

    /** @brief Returns `true` once a valid key has been set. */
    bool configured() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return key_size_ != 0U;
    }

    /** @brief Returns the active key length in bytes (0 when unconfigured). */
    size_type key_size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return key_size_;
    }

    /** @brief Returns the key index that the next processed byte will use. */
    size_type position() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return position_;
    }

    /**
     * @brief Encrypts `size` bytes of `input` into `output` and advances the stream.
     * @param input  Plaintext; may be null only when `size == 0`.
     * @param output Destination; may equal `input`.
     * @param size   Number of bytes to process.
     * @return `status::ok`, `status::not_configured` without a key, or `status::invalid_argument`
     *         for null buffers with `size != 0`. State is unchanged on error.
     * @note Call `reset()` before decrypting a message that was just encrypted with the same object.
     */
    CASTLE_NODISCARD status encrypt(CASTLE_CONST uint8_t* input, uint8_t* output, size_type size) CASTLE_NOEXCEPT
    {
        if (key_size_ == 0U)
        {
            return status::not_configured;
        }
        if (size != 0U && (input == 0 || output == 0))
        {
            return status::invalid_argument;
        }
        position_ = detail::xor_apply(input, output, size, key_.data(), key_size_, position_);
        return status::ok;
    }

    /** @brief In-place variant of `encrypt(input, output, size)`. */
    CASTLE_NODISCARD status encrypt(uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        return encrypt(data, data, size);
    }

    /** @brief Decrypts `input` into `output`; identical to `encrypt()` since XOR is symmetric. */
    CASTLE_NODISCARD status decrypt(CASTLE_CONST uint8_t* input, uint8_t* output, size_type size) CASTLE_NOEXCEPT
    {
        return encrypt(input, output, size);
    }

    /** @brief In-place variant of `decrypt(input, output, size)`. */
    CASTLE_NODISCARD status decrypt(uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        return encrypt(data, data, size);
    }

private:
    castle::container::array<uint8_t, MaxKeySize> key_;
    size_type key_size_;
    size_type position_;
};

} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_XOR_HPP
