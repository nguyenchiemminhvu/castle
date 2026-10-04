// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file md5.hpp
 * @brief Streaming RFC 1321 MD5 message digest.
 *
 * MD5 is provided for protocol compatibility and legacy integrity use only;
 * it is not suitable for modern collision-resistant security applications.
 */
#ifndef CASTLE_EXT_CRYPTO_HASH_MD5_HPP
#define CASTLE_EXT_CRYPTO_HASH_MD5_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/crypto/detail/bytes.hpp"

#include <stdint.h>

namespace castle
{
namespace crypto
{
namespace hash
{

/**
 * @brief MD5 hash function with a fixed-size streaming state.
 */
class md5
{
public:
    /** @brief Digest length in bytes. */
    static CASTLE_CONSTEXPR size_type digest_size = 16U;
    /** @brief Internal block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 64U;
    /** @brief Fixed-size digest container. */
    using digest_type = castle::container::array<uint8_t, digest_size>;

    /** @brief Constructs a hasher in its initial state. */
    md5() CASTLE_NOEXCEPT
    {
        reset();
    }

    /** @brief Restores the initial state, discarding all buffered input. */
    void reset() CASTLE_NOEXCEPT
    {
        state_[0] = 0x67452301UL;
        state_[1] = 0xEFCDAB89UL;
        state_[2] = 0x98BADCFEUL;
        state_[3] = 0x10325476UL;
        bit_count_ = 0U;
        buffer_size_ = 0U;
    }

    /**
     * @brief Feeds a chunk of the message into the hash.
     * @param data Source bytes; may be null only when `size == 0`.
     * @param size Number of bytes.
     * @return `status::ok` or `status::invalid_argument`.
     */
    status update(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        if (data == 0 && size != 0U)
        {
            return status::invalid_argument;
        }

        bit_count_ += static_cast<uint64_t>(size) << 3U;
        while (size != 0U)
        {
            CASTLE_CONST size_type take = (size < block_size - buffer_size_) ? size : block_size - buffer_size_;
            for (size_type i = 0U; i < take; ++i)
            {
                buffer_[buffer_size_ + i] = data[i];
            }
            buffer_size_ += take;
            data += take;
            size -= take;

            if (buffer_size_ == block_size)
            {
                transform(buffer_);
                buffer_size_ = 0U;
            }
        }
        return status::ok;
    }

    /** @brief Byte-pointer overload of `update()` for untyped buffers. */
    status update(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        return update(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

    /**
     * @brief Writes the digest into a caller-owned buffer; streaming state is unchanged.
     * @param output          Destination buffer.
     * @param output_capacity Destination capacity in bytes; must be at least `digest_size`.
     * @return `status::ok`, `status::full`, or `status::invalid_argument`.
     */
    status final(uint8_t* output, size_type output_capacity) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (output == 0 && output_capacity != 0U)
        {
            return status::invalid_argument;
        }
        if (output_capacity < digest_size)
        {
            return status::full;
        }
        if (output == 0)
        {
            return status::invalid_argument;
        }
        digest_type result = digest();
        for (size_type i = 0U; i < digest_size; ++i)
        {
            output[i] = result[i];
        }
        return status::ok;
    }

    /**
     * @brief Returns the digest of all data fed so far.
     *
     * Padding is applied to a copy, so more data may be added afterwards.
     */
    digest_type digest() CASTLE_CONST CASTLE_NOEXCEPT
    {
        md5 copy(*this);
        CASTLE_CONST uint64_t original_bits = copy.bit_count_;
        copy.buffer_[copy.buffer_size_++] = 0x80U;
        if (copy.buffer_size_ > 56U)
        {
            while (copy.buffer_size_ < block_size)
            {
                copy.buffer_[copy.buffer_size_++] = 0U;
            }
            copy.transform(copy.buffer_);
            copy.buffer_size_ = 0U;
        }
        while (copy.buffer_size_ < 56U)
        {
            copy.buffer_[copy.buffer_size_++] = 0U;
        }
        detail::store_le64(copy.buffer_ + 56U, original_bits);
        copy.transform(copy.buffer_);

        digest_type result;
        for (size_type i = 0U; i < 4U; ++i)
        {
            detail::store_le32(&result[4U * i], copy.state_[i]);
        }
        return result;
    }

    /**
     * @brief Computes the digest of a complete message in one call.
     * @param data Source bytes; may be null only when `size == 0`.
     * @param size Number of bytes.
     * @return Message digest.
     */
    static digest_type calculate(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        md5 value;
        CASTLE_ASSERT(data != 0 || size == 0U, "castle::crypto::hash::md5: invalid input");
        value.update(data, size);
        return value.digest();
    }

    /** @brief Byte-pointer overload of `calculate()` for untyped buffers. */
    static digest_type calculate(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        return calculate(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

private:
    /** @brief Rotates a 32-bit value left by `count` bits (1..31). */
    static CASTLE_CONSTEXPR uint32_t left_rotate(uint32_t value, uint32_t count) CASTLE_NOEXCEPT
    {
        return static_cast<uint32_t>((value << count) | (value >> (32U - count)));
    }

    /** @brief Returns the RFC 1321 additive constant for step `i` (0..63). */
    static uint32_t k(size_type i) CASTLE_NOEXCEPT
    {
        static CASTLE_CONST uint32_t values[64U] = {
            0xd76aa478UL, 0xe8c7b756UL, 0x242070dbUL, 0xc1bdceeeUL,
            0xf57c0fafUL, 0x4787c62aUL, 0xa8304613UL, 0xfd469501UL,
            0x698098d8UL, 0x8b44f7afUL, 0xffff5bb1UL, 0x895cd7beUL,
            0x6b901122UL, 0xfd987193UL, 0xa679438eUL, 0x49b40821UL,
            0xf61e2562UL, 0xc040b340UL, 0x265e5a51UL, 0xe9b6c7aaUL,
            0xd62f105dUL, 0x02441453UL, 0xd8a1e681UL, 0xe7d3fbc8UL,
            0x21e1cde6UL, 0xc33707d6UL, 0xf4d50d87UL, 0x455a14edUL,
            0xa9e3e905UL, 0xfcefa3f8UL, 0x676f02d9UL, 0x8d2a4c8aUL,
            0xfffa3942UL, 0x8771f681UL, 0x6d9d6122UL, 0xfde5380cUL,
            0xa4beea44UL, 0x4bdecfa9UL, 0xf6bb4b60UL, 0xbebfbc70UL,
            0x289b7ec6UL, 0xeaa127faUL, 0xd4ef3085UL, 0x04881d05UL,
            0xd9d4d039UL, 0xe6db99e5UL, 0x1fa27cf8UL, 0xc4ac5665UL,
            0xf4292244UL, 0x432aff97UL, 0xab9423a7UL, 0xfc93a039UL,
            0x655b59c3UL, 0x8f0ccc92UL, 0xffeff47dUL, 0x85845dd1UL,
            0x6fa87e4fUL, 0xfe2ce6e0UL, 0xa3014314UL, 0x4e0811a1UL,
            0xf7537e82UL, 0xbd3af235UL, 0x2ad7d2bbUL, 0xeb86d391UL
        };
        return values[i];
    }

    /** @brief Returns the RFC 1321 left-rotation amount for step `i` (0..63). */
    static uint32_t s(size_type i) CASTLE_NOEXCEPT
    {
        static CASTLE_CONST uint8_t values[64U] = {
            7U,12U,17U,22U, 7U,12U,17U,22U, 7U,12U,17U,22U, 7U,12U,17U,22U,
            5U,9U,14U,20U, 5U,9U,14U,20U, 5U,9U,14U,20U, 5U,9U,14U,20U,
            4U,11U,16U,23U, 4U,11U,16U,23U, 4U,11U,16U,23U, 4U,11U,16U,23U,
            6U,10U,15U,21U, 6U,10U,15U,21U, 6U,10U,15U,21U, 6U,10U,15U,21U
        };
        return values[i];
    }

    /** @brief Processes one 64-byte block and updates the chaining state. */
    void transform(CASTLE_CONST uint8_t* block) CASTLE_NOEXCEPT
    {
        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t words[16U];
        for (size_type i = 0U; i < 16U; ++i)
        {
            words[i] = detail::load_le32(block + 4U * i);
        }

        for (size_type i = 0U; i < 64U; ++i)
        {
            uint32_t f;
            size_type g;
            if (i < 16U)
            {
                f = (b & c) | ((~b) & d);
                g = i;
            }
            else if (i < 32U)
            {
                f = (d & b) | ((~d) & c);
                g = (5U * i + 1U) & 15U;
            }
            else if (i < 48U)
            {
                f = b ^ c ^ d;
                g = (3U * i + 5U) & 15U;
            }
            else
            {
                f = c ^ (b | (~d));
                g = (7U * i) & 15U;
            }

            CASTLE_CONST uint32_t temp = d;
            d = c;
            c = b;
            b = b + left_rotate(a + f + k(i) + words[g], s(i));
            a = temp;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
    }

    uint32_t state_[4U];
    uint64_t bit_count_;
    uint8_t buffer_[block_size];
    size_type buffer_size_;
};

} // namespace hash
} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_HASH_MD5_HPP
