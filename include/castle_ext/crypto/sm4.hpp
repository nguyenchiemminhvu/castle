// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sm4.hpp
 * @brief Heap-free SM4-128 block cipher.
 *
 * SM4 uses a 128-bit block and a 128-bit key. The class exposes raw block
 * operations and in-place CTR processing; authentication is intentionally
 * left to separate constructions.
 */
#ifndef CASTLE_EXT_CRYPTO_SM4_HPP
#define CASTLE_EXT_CRYPTO_SM4_HPP

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

/** @brief SM4 nonlinear substitution box (GM/T 0002 / ISO/IEC 18033-3). */
static inline CASTLE_CONST uint8_t sm4_sbox[256U] = {
    0xD6U, 0x90U, 0xE9U, 0xFEU, 0xCCU, 0xE1U, 0x3DU, 0xB7U, 0x16U, 0xB6U, 0x14U, 0xC2U, 0x28U, 0xFBU, 0x2CU, 0x05U,
    0x2BU, 0x67U, 0x9AU, 0x76U, 0x2AU, 0xBEU, 0x04U, 0xC3U, 0xAAU, 0x44U, 0x13U, 0x26U, 0x49U, 0x86U, 0x06U, 0x99U,
    0x9CU, 0x42U, 0x50U, 0xF4U, 0x91U, 0xEFU, 0x98U, 0x7AU, 0x33U, 0x54U, 0x0BU, 0x43U, 0xEDU, 0xCFU, 0xACU, 0x62U,
    0xE4U, 0xB3U, 0x1CU, 0xA9U, 0xC9U, 0x08U, 0xE8U, 0x95U, 0x80U, 0xDFU, 0x94U, 0xFAU, 0x75U, 0x8FU, 0x3FU, 0xA6U,
    0x47U, 0x07U, 0xA7U, 0xFCU, 0xF3U, 0x73U, 0x17U, 0xBAU, 0x83U, 0x59U, 0x3CU, 0x19U, 0xE6U, 0x85U, 0x4FU, 0xA8U,
    0x68U, 0x6BU, 0x81U, 0xB2U, 0x71U, 0x64U, 0xDAU, 0x8BU, 0xF8U, 0xEBU, 0x0FU, 0x4BU, 0x70U, 0x56U, 0x9DU, 0x35U,
    0x1EU, 0x24U, 0x0EU, 0x5EU, 0x63U, 0x58U, 0xD1U, 0xA2U, 0x25U, 0x22U, 0x7CU, 0x3BU, 0x01U, 0x21U, 0x78U, 0x87U,
    0xD4U, 0x00U, 0x46U, 0x57U, 0x9FU, 0xD3U, 0x27U, 0x52U, 0x4CU, 0x36U, 0x02U, 0xE7U, 0xA0U, 0xC4U, 0xC8U, 0x9EU,
    0xEAU, 0xBFU, 0x8AU, 0xD2U, 0x40U, 0xC7U, 0x38U, 0xB5U, 0xA3U, 0xF7U, 0xF2U, 0xCEU, 0xF9U, 0x61U, 0x15U, 0xA1U,
    0xE0U, 0xAEU, 0x5DU, 0xA4U, 0x9BU, 0x34U, 0x1AU, 0x55U, 0xADU, 0x93U, 0x32U, 0x30U, 0xF5U, 0x8CU, 0xB1U, 0xE3U,
    0x1DU, 0xF6U, 0xE2U, 0x2EU, 0x82U, 0x66U, 0xCAU, 0x60U, 0xC0U, 0x29U, 0x23U, 0xABU, 0x0DU, 0x53U, 0x4EU, 0x6FU,
    0xD5U, 0xDBU, 0x37U, 0x45U, 0xDEU, 0xFDU, 0x8EU, 0x2FU, 0x03U, 0xFFU, 0x6AU, 0x72U, 0x6DU, 0x6CU, 0x5BU, 0x51U,
    0x8DU, 0x1BU, 0xAFU, 0x92U, 0xBBU, 0xDDU, 0xBCU, 0x7FU, 0x11U, 0xD9U, 0x5CU, 0x41U, 0x1FU, 0x10U, 0x5AU, 0xD8U,
    0x0AU, 0xC1U, 0x31U, 0x88U, 0xA5U, 0xCDU, 0x7BU, 0xBDU, 0x2DU, 0x74U, 0xD0U, 0x12U, 0xB8U, 0xE5U, 0xB4U, 0xB0U,
    0x89U, 0x69U, 0x97U, 0x4AU, 0x0CU, 0x96U, 0x77U, 0x7EU, 0x65U, 0xB9U, 0xF1U, 0x09U, 0xC5U, 0x6EU, 0xC6U, 0x84U,
    0x18U, 0xF0U, 0x7DU, 0xECU, 0x3AU, 0xDCU, 0x4DU, 0x20U, 0x79U, 0xEEU, 0x5FU, 0x3EU, 0xD7U, 0xCBU, 0x39U, 0x48U
};

/** @brief SM4 fixed system parameters. */
static inline CASTLE_CONST uint32_t sm4_fk[4U] = {
    0xA3B1BAC6UL, 0x56AA3350UL, 0x677D9197UL, 0xB27022DCUL
};

/** @brief SM4 key-schedule constants. */
static inline CASTLE_CONST uint32_t sm4_ck[32U] = {
    0x00070E15UL, 0x1C232A31UL, 0x383F464DUL, 0x545B6269UL,
    0x70777E85UL, 0x8C939AA1UL, 0xA8AFB6BDUL, 0xC4CBD2D9UL,
    0xE0E7EEF5UL, 0xFC030A11UL, 0x181F262DUL, 0x343B4249UL,
    0x50575E65UL, 0x6C737A81UL, 0x888F969DUL, 0xA4ABB2B9UL,
    0xC0C7CED5UL, 0xDCE3EAF1UL, 0xF8FF060DUL, 0x141B2229UL,
    0x30373E45UL, 0x4C535A61UL, 0x686F767DUL, 0x848B9299UL,
    0xA0A7AEB5UL, 0xBCC3CAD1UL, 0xD8DFE6EDUL, 0xF4FB0209UL,
    0x10171E25UL, 0x2C333A41UL, 0x484F565DUL, 0x646B7279UL
};

/** @brief Applies the SM4 S-box to each byte of a 32-bit word. */
static uint32_t sm4_tau(uint32_t value) CASTLE_NOEXCEPT
{
    return (static_cast<uint32_t>(sm4_sbox[(value >> 24U) & 0xFFU]) << 24U) |
           (static_cast<uint32_t>(sm4_sbox[(value >> 16U) & 0xFFU]) << 16U) |
           (static_cast<uint32_t>(sm4_sbox[(value >> 8U) & 0xFFU]) << 8U) |
           static_cast<uint32_t>(sm4_sbox[value & 0xFFU]);
}

/** @brief SM4 round transform L(B) = B xor B<<<2 xor B<<<10 xor B<<<18 xor B<<<24. */
static uint32_t sm4_l(uint32_t value) CASTLE_NOEXCEPT
{
        return value ^ castle::rotl32(value, 2U) ^ castle::rotl32(value, 10U) ^
            castle::rotl32(value, 18U) ^ castle::rotl32(value, 24U);
}

/** @brief SM4 key-schedule transform L'(B) = B xor B<<<13 xor B<<<23. */
static uint32_t sm4_l_key(uint32_t value) CASTLE_NOEXCEPT
{
    return value ^ castle::rotl32(value, 13U) ^ castle::rotl32(value, 23U);
}

/** @brief Complete SM4 nonlinear/linear round transform. */
static uint32_t sm4_t(uint32_t value) CASTLE_NOEXCEPT
{
    return sm4_l(sm4_tau(value));
}

/** @brief Complete SM4 key-schedule transform. */
static uint32_t sm4_t_key(uint32_t value) CASTLE_NOEXCEPT
{
    return sm4_l_key(sm4_tau(value));
}

} // namespace detail

/**
 * @brief SM4-128 block cipher.
 *
 * The object owns its 32-word round-key schedule in fixed inline storage.
 * Copying a cipher copies that schedule; destroying an instance wipes it.
 */
class sm4
{
public:
    /** @brief SM4 block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 16U;
    /** @brief SM4 key length in bytes. */
    static CASTLE_CONSTEXPR size_type key_size = 16U;
    /** @brief Number of SM4 rounds. */
    static CASTLE_CONSTEXPR size_type rounds = 32U;

    /** @brief Constructs an instance with an all-zero key. */
    sm4() CASTLE_NOEXCEPT
    {
        const uint8_t zero_key[key_size] = {0U};
        static_cast<void>(set_key(zero_key));
    }

    /**
     * @brief Constructs an instance from a 128-bit key.
     * @param key Key bytes; null selects an all-zero key for consistency with Castle AES.
     */
    explicit sm4(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
    {
        if (key != 0)
        {
            static_cast<void>(set_key(key));
        }
        else
        {
            const uint8_t zero_key[key_size] = {0U};
            static_cast<void>(set_key(zero_key));
        }
    }

    /** @brief Wipes the expanded key schedule. */
    ~sm4() CASTLE_NOEXCEPT
    {
        castle::secure_zero(round_keys_, sizeof(round_keys_));
    }

    /**
     * @brief Replaces the current key and expands the 32 round keys.
     * @param key Exactly 16 key bytes.
     * @return `status::ok`, or `status::invalid_argument` when `key == nullptr`.
     */
    CASTLE_NODISCARD status set_key(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
    {
        if (key == 0)
        {
            return status::invalid_argument;
        }

        uint32_t k[36U];
        k[0] = castle::read_be32(key) ^ detail::sm4_fk[0];
        k[1] = castle::read_be32(key + 4U) ^ detail::sm4_fk[1];
        k[2] = castle::read_be32(key + 8U) ^ detail::sm4_fk[2];
        k[3] = castle::read_be32(key + 12U) ^ detail::sm4_fk[3];

        for (size_type i = 0U; i < rounds; ++i)
        {
            k[i + 4U] = k[i] ^ detail::sm4_t_key(k[i + 1U] ^ k[i + 2U] ^ k[i + 3U] ^ detail::sm4_ck[i]);
            round_keys_[i] = k[i + 4U];
        }

        castle::secure_zero(k, sizeof(k));
        return status::ok;
    }

    /**
     * @brief Encrypts one 16-byte block.
     * @param input Plaintext block.
     * @param output Ciphertext block; may alias `input`.
     * @note Nothing is written when either pointer is null.
     */
    void encrypt_block(CASTLE_CONST uint8_t input[block_size], uint8_t output[block_size])
        CASTLE_CONST CASTLE_NOEXCEPT
    {
        crypt_block(input, output, false);
    }

    /**
     * @brief Decrypts one 16-byte block.
     * @param input Ciphertext block.
     * @param output Plaintext block; may alias `input`.
     * @note Nothing is written when either pointer is null.
     */
    void decrypt_block(CASTLE_CONST uint8_t input[block_size], uint8_t output[block_size])
        CASTLE_CONST CASTLE_NOEXCEPT
    {
        crypt_block(input, output, true);
    }

    /**
     * @brief Applies the SM4-CTR keystream to an arbitrary-length buffer in place.
     * @param data Input/output bytes; may be null only when `size == 0`.
     * @param size Number of bytes to process; a final partial block needs no padding.
     * @param counter_block 16-byte counter block. Its final four bytes are a big-endian
     *        counter and are incremented once per generated block.
     * @return `status::ok`, `status::invalid_argument` for a null required pointer,
     *         or `status::out_of_range` if the counter would wrap.
     * @note `data` and `counter_block` must not overlap. CTR provides no authentication.
     *       Each call discards unused bytes from its final keystream block.
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
        if (block_count > (0xFFFFFFFFU - initial_counter))
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
            increment_ctr_counter(counter_block);
            data += take;
            size -= take;
        }
        castle::secure_zero(keystream, sizeof(keystream));
        return status::ok;
    }

private:
    /** @brief Increments the trailing 32-bit big-endian CTR counter. */
    static void increment_ctr_counter(uint8_t counter_block[block_size]) CASTLE_NOEXCEPT
    {
        castle::write_be32(counter_block + 12U, castle::read_be32(counter_block + 12U) + 1U);
    }

    /** @brief Processes one SM4 block with the selected forward or reverse key order. */
    void crypt_block(CASTLE_CONST uint8_t input[block_size],
                     uint8_t output[block_size],
                     bool decrypt) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (input == 0 || output == 0)
        {
            return;
        }

        uint32_t x[36U];
        x[0] = castle::read_be32(input);
        x[1] = castle::read_be32(input + 4U);
        x[2] = castle::read_be32(input + 8U);
        x[3] = castle::read_be32(input + 12U);

        for (size_type i = 0U; i < rounds; ++i)
        {
            CASTLE_CONST size_type key_index = decrypt ? (rounds - 1U - i) : i;
            x[i + 4U] = x[i] ^ detail::sm4_t(x[i + 1U] ^ x[i + 2U] ^ x[i + 3U] ^ round_keys_[key_index]);
        }

        castle::write_be32(output, x[35U]);
        castle::write_be32(output + 4U, x[34U]);
        castle::write_be32(output + 8U, x[33U]);
        castle::write_be32(output + 12U, x[32U]);
        castle::secure_zero(x, sizeof(x));
    }

    uint32_t round_keys_[rounds];
};

} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_SM4_HPP
