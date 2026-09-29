// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file aes.hpp
 * @brief Heap-free AES-128, AES-192, and AES-256 block cipher.
 *
 * The class exposes single-block encryption/decryption plus AES-CTR keystream
 * processing. CTR treats the final four bytes of the supplied counter block
 * as a big-endian counter and increments that block after each encrypted block.
 */
#ifndef CASTLE_EXT_CRYPTO_AES_HPP
#define CASTLE_EXT_CRYPTO_AES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bytes.hpp"

#include <stdint.h>

namespace castle
{
namespace crypto
{

namespace detail
{

/** @brief AES forward S-box (FIPS 197, figure 7). */
static inline CASTLE_CONST uint8_t aes_sbox[256U] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

/** @brief AES inverse S-box (FIPS 197, figure 14). */
static inline CASTLE_CONST uint8_t aes_inv_sbox[256U] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

} // namespace detail

/**
 * @brief AES block cipher with a compile-time selected key size.
 * @tparam KeyBits One of 128, 192, or 256.
 */
template <size_type KeyBits>
class aes
{
    static_assert(KeyBits == 128U || KeyBits == 192U || KeyBits == 256U,
                  "castle::crypto::aes supports 128, 192, or 256-bit keys");

public:
    /** @brief Block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 16U;
    /** @brief Key length in bytes. */
    static CASTLE_CONSTEXPR size_type key_size = KeyBits / 8U;
    /** @brief Number of cipher rounds (10, 12 or 14). */
    static CASTLE_CONSTEXPR size_type rounds = KeyBits / 32U + 6U;

    /**
     * @brief Constructs a cipher and expands the key schedule.
     * @param key `key_size` key bytes; a null pointer selects an all-zero key.
     */
    explicit aes(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (key != 0)
        {
            expand_key(key);
        }
        else
        {
            uint8_t zero_key[key_size] = {0U};
            expand_key(zero_key);
        }
        // LCOV_EXCL_STOP
    }

    /** @brief Constructs a cipher keyed with all-zero bytes; call `set_key()` before real use. */
    aes() CASTLE_NOEXCEPT
    {
        uint8_t zero_key[key_size] = {0U};
        set_key(zero_key);
    }

    /**
     * @brief Replaces the key and recomputes the key schedule.
     * @param key `key_size` key bytes.
     * @return `status::ok` or `status::invalid_argument` for a null key.
     */
    status set_key(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
    {
        if (key == 0)
        {
            return status::invalid_argument;
        }
        expand_key(key);
        return status::ok;
    }

    /**
     * @brief Encrypts one 16-byte block.
     * @param input  Plaintext block.
     * @param output Ciphertext block; may alias `input`. Nothing is written if either pointer is null.
     */
    void encrypt_block(CASTLE_CONST uint8_t input[block_size], uint8_t output[block_size]) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (input == 0 || output == 0)
        {
            return;
        }
        uint8_t state[block_size];
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = input[i];
        }
        add_round_key(state, 0U);

        for (size_type round = 1U; round < rounds; ++round)
        {
            sub_bytes(state);
            shift_rows(state);
            mix_columns(state);
            add_round_key(state, round);
        }
        sub_bytes(state);
        shift_rows(state);
        add_round_key(state, rounds);
        for (size_type i = 0U; i < block_size; ++i)
        {
            output[i] = state[i];
        }
    }

    /**
     * @brief Decrypts one 16-byte block.
     * @param input  Ciphertext block.
     * @param output Plaintext block; may alias `input`. Nothing is written if either pointer is null.
     */
    void decrypt_block(CASTLE_CONST uint8_t input[block_size], uint8_t output[block_size]) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (input == 0 || output == 0)
        {
            return;
        }
        uint8_t state[block_size];
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = input[i];
        }
        add_round_key(state, rounds);

        for (size_type round = rounds; round > 1U; --round)
        {
            inv_shift_rows(state);
            inv_sub_bytes(state);
            add_round_key(state, round - 1U);
            inv_mix_columns(state);
        }
        inv_shift_rows(state);
        inv_sub_bytes(state);
        add_round_key(state, 0U);
        for (size_type i = 0U; i < block_size; ++i)
        {
            output[i] = state[i];
        }
    }

    /**
     * @brief XORs arbitrary-length data with AES-CTR keystream.
     * @param data Input/output buffer.
     * @param size Number of bytes to process.
     * @param counter_block Sixteen-byte counter block. Its final four bytes
     *        are incremented as a big-endian uint32 value after each block.
     * @return `status::ok`, `status::invalid_argument` for a null required pointer,
     *         or `status::out_of_range` if the counter would wrap.
     * @note `data` and `counter_block` must not overlap. CTR provides no authentication.
     */
    CASTLE_NODISCARD status crypt_ctr(uint8_t* data, size_type size,
                                      uint8_t counter_block[block_size]) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if ((data == 0 && size != 0U) || counter_block == 0)
        {
            return status::invalid_argument;
        }

        size_type block_count = size / block_size;
        if ((size % block_size) != 0U)
        {
            ++block_count;
        }

        CASTLE_CONST uint32_t initial_counter = castle::read_be32(counter_block + 12U);
        if (block_count > static_cast<size_type>(0xFFFFFFFFU - initial_counter))
        {
            return status::out_of_range;
        }

        uint8_t keystream[block_size];
        while (size != 0U)
        {
            encrypt_block(counter_block, keystream);
            CASTLE_CONST size_type take = size < block_size ? size : block_size;
            for (size_type i = 0U; i < take; ++i)
            {
                data[i] = static_cast<uint8_t>(data[i] ^ keystream[i]);
            }
            increment_counter(counter_block);
            data += take;
            size -= take;
        }
        return status::ok;
    }

private:
    /** @brief Multiplies by x in GF(2^8) modulo the AES polynomial 0x11B. */
    static uint8_t xtime(uint8_t value) CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>((value << 1U) ^ ((value & 0x80U) ? 0x1BU : 0U));
    }

    /** @brief Multiplies two elements of GF(2^8) (shift-and-add). */
    static uint8_t multiply(uint8_t a, uint8_t b) CASTLE_NOEXCEPT
    {
        uint8_t result = 0U;
        uint8_t x = a;
        uint8_t y = b;
        for (size_type i = 0U; i < 8U; ++i)
        {
            if ((y & 1U) != 0U)
            {
                result = static_cast<uint8_t>(result ^ x);
            }
            x = xtime(x);
            y = static_cast<uint8_t>(y >> 1U);
        }
        return result;
    }

    /** @brief Returns the key-schedule round constant for 1-based `index`. */
    static uint8_t rcon(size_type index) CASTLE_NOEXCEPT
    {
        uint8_t value = 1U;
        for (size_type i = 1U; i < index; ++i)
        {
            value = xtime(value);
        }
        return value;
    }

    /** @brief Applies the S-box to every state byte. */
    static void sub_bytes(uint8_t state[block_size]) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = detail::aes_sbox[state[i]];
        }
    }

    /** @brief Applies the inverse S-box to every state byte. */
    static void inv_sub_bytes(uint8_t state[block_size]) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = detail::aes_inv_sbox[state[i]];
        }
    }

    /** @brief Cyclically shifts row `r` of the column-major state left by `r` positions. */
    static void shift_rows(uint8_t state[block_size]) CASTLE_NOEXCEPT
    {
        uint8_t tmp[block_size];
        for (size_type row = 0U; row < 4U; ++row)
        {
            for (size_type col = 0U; col < 4U; ++col)
            {
                tmp[row + 4U * col] = state[row + 4U * ((col + row) & 3U)];
            }
        }
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = tmp[i];
        }
    }

    /** @brief Inverse of `shift_rows()`; shifts row `r` right by `r` positions. */
    static void inv_shift_rows(uint8_t state[block_size]) CASTLE_NOEXCEPT
    {
        uint8_t tmp[block_size];
        for (size_type row = 0U; row < 4U; ++row)
        {
            for (size_type col = 0U; col < 4U; ++col)
            {
                tmp[row + 4U * col] = state[row + 4U * ((col + 4U - row) & 3U)];
            }
        }
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = tmp[i];
        }
    }

    /** @brief Mixes each state column with the fixed MDS matrix over GF(2^8). */
    static void mix_columns(uint8_t state[block_size]) CASTLE_NOEXCEPT
    {
        for (size_type col = 0U; col < 4U; ++col)
        {
            uint8_t* a = state + 4U * col;
            CASTLE_CONST uint8_t a0 = a[0];
            CASTLE_CONST uint8_t a1 = a[1];
            CASTLE_CONST uint8_t a2 = a[2];
            CASTLE_CONST uint8_t a3 = a[3];
            a[0] = static_cast<uint8_t>(xtime(a0) ^ (xtime(a1) ^ a1) ^ a2 ^ a3);
            a[1] = static_cast<uint8_t>(a0 ^ xtime(a1) ^ (xtime(a2) ^ a2) ^ a3);
            a[2] = static_cast<uint8_t>(a0 ^ a1 ^ xtime(a2) ^ (xtime(a3) ^ a3));
            a[3] = static_cast<uint8_t>((xtime(a0) ^ a0) ^ a1 ^ a2 ^ xtime(a3));
        }
    }

    /** @brief Inverse of `mix_columns()`. */
    static void inv_mix_columns(uint8_t state[block_size]) CASTLE_NOEXCEPT
    {
        for (size_type col = 0U; col < 4U; ++col)
        {
            uint8_t* a = state + 4U * col;
            CASTLE_CONST uint8_t a0 = a[0];
            CASTLE_CONST uint8_t a1 = a[1];
            CASTLE_CONST uint8_t a2 = a[2];
            CASTLE_CONST uint8_t a3 = a[3];
            a[0] = static_cast<uint8_t>(multiply(a0, 0x0EU) ^ multiply(a1, 0x0BU) ^ multiply(a2, 0x0DU) ^ multiply(a3, 0x09U));
            a[1] = static_cast<uint8_t>(multiply(a0, 0x09U) ^ multiply(a1, 0x0EU) ^ multiply(a2, 0x0BU) ^ multiply(a3, 0x0DU));
            a[2] = static_cast<uint8_t>(multiply(a0, 0x0DU) ^ multiply(a1, 0x09U) ^ multiply(a2, 0x0EU) ^ multiply(a3, 0x0BU));
            a[3] = static_cast<uint8_t>(multiply(a0, 0x0BU) ^ multiply(a1, 0x0DU) ^ multiply(a2, 0x09U) ^ multiply(a3, 0x0EU));
        }
    }

    /**
     * @brief XORs the round key for `round` into the state.
     * @param state Block being transformed.
     * @param round Round index in `0..rounds`.
     */
    void add_round_key(uint8_t state[block_size], size_type round) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < block_size; ++i)
        {
            state[i] = static_cast<uint8_t>(state[i] ^ round_keys_[round * block_size + i]);
        }
    }

    /** @brief Expands `key_size` key bytes into `rounds + 1` round keys (FIPS 197, section 5.2). */
    void expand_key(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type words = 4U * (rounds + 1U);
        CASTLE_CONST size_type key_words = key_size / 4U;
        for (size_type i = 0U; i < key_size; ++i)
        {
            round_keys_[i] = key[i];
        }

        for (size_type word = key_words; word < words; ++word)
        {
            uint8_t temp[4U] = {
                round_keys_[4U * (word - 1U)],
                round_keys_[4U * (word - 1U) + 1U],
                round_keys_[4U * (word - 1U) + 2U],
                round_keys_[4U * (word - 1U) + 3U]
            };

            if ((word % key_words) == 0U)
            {
                uint8_t first = temp[0];
                temp[0] = detail::aes_sbox[temp[1]];
                temp[1] = detail::aes_sbox[temp[2]];
                temp[2] = detail::aes_sbox[temp[3]];
                temp[3] = detail::aes_sbox[first];
                temp[0] = static_cast<uint8_t>(temp[0] ^ rcon(word / key_words));
            }
            else if (KeyBits == 256U && (word % key_words) == 4U)
            {
                temp[0] = detail::aes_sbox[temp[0]];
                temp[1] = detail::aes_sbox[temp[1]];
                temp[2] = detail::aes_sbox[temp[2]];
                temp[3] = detail::aes_sbox[temp[3]];
            }

            for (size_type byte = 0U; byte < 4U; ++byte)
            {
                round_keys_[4U * word + byte] = static_cast<uint8_t>(round_keys_[4U * (word - key_words) + byte] ^ temp[byte]);
            }
        }
    }

    /** @brief Increments the trailing 32 bits of the counter block as a big-endian integer. */
    static void increment_counter(uint8_t counter[block_size]) CASTLE_NOEXCEPT
    {
        for (size_type i = block_size; i > 12U; --i)
        {
            ++counter[i - 1U];
            if (counter[i - 1U] != 0U)
            {
                break;
            }
        }
    }

    uint8_t round_keys_[16U * (rounds + 1U)];
};

} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_AES_HPP
