// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file chacha20.hpp
 * @brief RFC 8439 ChaCha20 stream cipher.
 */
#ifndef CASTLE_EXT_CRYPTO_CHACHA20_HPP
#define CASTLE_EXT_CRYPTO_CHACHA20_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/crypto/detail/bytes.hpp"

#include <stdint.h>

namespace castle
{
namespace crypto
{

/**
 * @brief Heap-free, incremental ChaCha20 stream cipher.
 *
 * The implementation uses the RFC 8439 32-byte key, 12-byte nonce, and
 * 32-bit block counter layout. It can process arbitrarily sized chunks of a
 * message without requiring the caller to align updates to 64-byte blocks.
 */
class chacha20
{
public:
    /** @brief Key length in bytes. */
    static CASTLE_CONSTEXPR size_type key_size = 32U;
    /** @brief Nonce length in bytes. */
    static CASTLE_CONSTEXPR size_type nonce_size = 12U;
    /** @brief Keystream block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 64U;

    /**
     * @brief Constructs a cipher from a key, nonce and initial block counter.
     * @param key     32-byte key.
     * @param nonce   12-byte nonce.
     * @param counter Initial block counter.
     * @note A null `key` or `nonce` selects an all-zero key and nonce.
     */
    chacha20(CASTLE_CONST uint8_t* key, CASTLE_CONST uint8_t* nonce, uint32_t counter = 0U) CASTLE_NOEXCEPT
    {
        if (key != 0 && nonce != 0)
        {
            reset(key, nonce, counter);
        }
        else
        {
            uint8_t zero_key[key_size] = {0U};
            uint8_t zero_nonce[nonce_size] = {0U};
            reset(zero_key, zero_nonce, counter);
        }
    }

    /** @brief Constructs a cipher with an all-zero key and nonce; call `reset()` before real use. */
    chacha20() CASTLE_NOEXCEPT
    {
        uint8_t zero_key[key_size] = {0U};
        uint8_t zero_nonce[nonce_size] = {0U};
        reset(zero_key, zero_nonce, 0U);
    }

    /**
     * @brief Re-keys the cipher and discards any unused keystream.
     * @param key     32-byte key.
     * @param nonce   12-byte nonce.
     * @param counter Initial block counter.
     * @return `status::ok` or `status::invalid_argument` for a null key or nonce.
     */
    status reset(CASTLE_CONST uint8_t* key, CASTLE_CONST uint8_t* nonce, uint32_t counter = 0U) CASTLE_NOEXCEPT
    {
        if (key == 0 || nonce == 0)
        {
            return status::invalid_argument;
        }
        state_[0] = 0x61707865UL;
        state_[1] = 0x3320646eUL;
        state_[2] = 0x79622d32UL;
        state_[3] = 0x6b206574UL;
        for (size_type i = 0U; i < 8U; ++i)
        {
            state_[4U + i] = detail::load_le32(key + 4U * i);
        }
        state_[12] = counter;
        state_[13] = detail::load_le32(nonce);
        state_[14] = detail::load_le32(nonce + 4U);
        state_[15] = detail::load_le32(nonce + 8U);
        keystream_size_ = 0U;
        return status::ok;
    }

    /**
     * @brief Repositions the stream at a block boundary, discarding unused keystream.
     * @param counter Block counter of the next 64-byte block.
     * @return Always `status::ok`.
     */
    status set_counter(uint32_t counter) CASTLE_NOEXCEPT
    {
        state_[12] = counter;
        keystream_size_ = 0U;
        return status::ok;
    }

    /** @brief Returns the block counter that will be used for the next generated block. */
    uint32_t counter() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_[12];
    }

    /**
     * @brief XORs data with the keystream in place; the same call encrypts and decrypts.
     * @param data Input/output buffer; may be null only when `size == 0`.
     * @param size Number of bytes to process.
     */
    void crypt(uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        if (data == 0 && size != 0U)
        {
            return;
        }
        while (size != 0U)
        {
            if (keystream_size_ == 0U)
            {
                generate_block();
            }
            CASTLE_CONST size_type available = block_size - keystream_size_;
            CASTLE_CONST size_type take = size < available ? size : available;
            for (size_type i = 0U; i < take; ++i)
            {
                data[i] = static_cast<uint8_t>(data[i] ^ keystream_[keystream_size_ + i]);
            }
            data += take;
            size -= take;
            keystream_size_ += take;
            if (keystream_size_ == block_size)
            {
                keystream_size_ = 0U;
            }
        }
    }

private:
    /** @brief Applies the RFC 8439 quarter round to four state words in place. */
    static void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) CASTLE_NOEXCEPT
    {
        a += b; d ^= a; d = detail::rotl32(d, 16U);
        c += d; b ^= c; b = detail::rotl32(b, 12U);
        a += b; d ^= a; d = detail::rotl32(d, 8U);
        c += d; b ^= c; b = detail::rotl32(b, 7U);
    }

    /** @brief Produces the next 64-byte keystream block and advances the block counter. */
    void generate_block() CASTLE_NOEXCEPT
    {
        uint32_t working[16U];
        for (size_type i = 0U; i < 16U; ++i)
        {
            working[i] = state_[i];
        }

        for (size_type i = 0U; i < 10U; ++i)
        {
            quarter_round(working[0], working[4], working[8], working[12]);
            quarter_round(working[1], working[5], working[9], working[13]);
            quarter_round(working[2], working[6], working[10], working[14]);
            quarter_round(working[3], working[7], working[11], working[15]);
            quarter_round(working[0], working[5], working[10], working[15]);
            quarter_round(working[1], working[6], working[11], working[12]);
            quarter_round(working[2], working[7], working[8], working[13]);
            quarter_round(working[3], working[4], working[9], working[14]);
        }

        for (size_type i = 0U; i < 16U; ++i)
        {
            detail::store_le32(keystream_ + 4U * i, working[i] + state_[i]);
        }
        ++state_[12];
        keystream_size_ = 0U;
    }

    uint32_t state_[16U];
    uint8_t keystream_[block_size];
    size_type keystream_size_;
};

} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_CHACHA20_HPP
