// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sha256.hpp
 * @brief Streaming SHA-256 hashing primitive.
 */
#ifndef CASTLE_EXT_CRYPTO_HASH_SHA256_HPP
#define CASTLE_EXT_CRYPTO_HASH_SHA256_HPP

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

namespace detail
{
/** @brief SHA-256 round constants (FIPS 180-4, section 4.2.2). */
static inline CASTLE_CONST uint32_t sha256_k[64U] = {
    0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,
    0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
    0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,
    0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
    0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,
    0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
    0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,
    0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
    0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,
    0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
    0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,
    0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
    0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,
    0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
    0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,
    0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL
};
}

/**
 * @brief SHA-256 hash function with a fixed-size streaming state.
 */
class sha256
{
public:
    /** @brief Digest length in bytes. */
    static CASTLE_CONSTEXPR size_type digest_size = 32U;
    /** @brief Internal block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 64U;
    /** @brief Fixed-size digest container. */
    using digest_type = castle::container::array<uint8_t, digest_size>;

    /** @brief Constructs a hasher in its initial state. */
    sha256() CASTLE_NOEXCEPT
    {
        reset();
    }

    /** @brief Restores the initial state, discarding all buffered input. */
    void reset() CASTLE_NOEXCEPT
    {
        state_[0] = 0x6a09e667UL;
        state_[1] = 0xbb67ae85UL;
        state_[2] = 0x3c6ef372UL;
        state_[3] = 0xa54ff53aUL;
        state_[4] = 0x510e527fUL;
        state_[5] = 0x9b05688cUL;
        state_[6] = 0x1f83d9abUL;
        state_[7] = 0x5be0cd19UL;
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
            CASTLE_CONST size_type take = size < block_size - buffer_size_ ? size : block_size - buffer_size_;
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
     * @brief Returns the digest of all data fed so far.
     *
     * Padding is applied to a copy, so more data may be added afterwards.
     */
    digest_type digest() CASTLE_CONST CASTLE_NOEXCEPT
    {
        sha256 copy(*this);
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
        crypto::detail::store_be64(copy.buffer_ + 56U, original_bits);
        copy.transform(copy.buffer_);

        digest_type result;
        for (size_type i = 0U; i < 8U; ++i)
        {
            crypto::detail::store_be32(&result[4U * i], copy.state_[i]);
        }
        return result;
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
     * @brief Computes the digest of a complete message in one call.
     * @param data Source bytes; may be null only when `size == 0`.
     * @param size Number of bytes.
     * @return Message digest.
     */
    static digest_type calculate(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        sha256 value;
        CASTLE_ASSERT(data != 0 || size == 0U, "castle::crypto::hash::sha256: invalid input");
        value.update(data, size);
        return value.digest();
    }

    /** @brief Byte-pointer overload of `calculate()` for untyped buffers. */
    static digest_type calculate(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        return calculate(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

private:
    /** @brief Ch(e, f, g): selects bits of `f` where `e` is set, otherwise bits of `g`. */
    static CASTLE_CONSTEXPR uint32_t choose(uint32_t e, uint32_t f, uint32_t g) CASTLE_NOEXCEPT
    {
        return (e & f) ^ ((~e) & g);
    }

    /** @brief Maj(a, b, c): bitwise majority of the three inputs. */
    static CASTLE_CONSTEXPR uint32_t majority(uint32_t a, uint32_t b, uint32_t c) CASTLE_NOEXCEPT
    {
        return (a & b) ^ (a & c) ^ (b & c);
    }

    /** @brief Upper-case Sigma0 of FIPS 180-4 (ROTR 2, 13, 22). */
    static CASTLE_CONSTEXPR uint32_t big_sigma0(uint32_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr32(x, 2U) ^ crypto::detail::rotr32(x, 13U) ^ crypto::detail::rotr32(x, 22U);
    }

    /** @brief Upper-case Sigma1 of FIPS 180-4 (ROTR 6, 11, 25). */
    static CASTLE_CONSTEXPR uint32_t big_sigma1(uint32_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr32(x, 6U) ^ crypto::detail::rotr32(x, 11U) ^ crypto::detail::rotr32(x, 25U);
    }

    /** @brief Lower-case sigma0 of FIPS 180-4 (ROTR 7, 18, SHR 3). */
    static CASTLE_CONSTEXPR uint32_t small_sigma0(uint32_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr32(x, 7U) ^ crypto::detail::rotr32(x, 18U) ^ (x >> 3U);
    }

    /** @brief Lower-case sigma1 of FIPS 180-4 (ROTR 17, 19, SHR 10). */
    static CASTLE_CONSTEXPR uint32_t small_sigma1(uint32_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr32(x, 17U) ^ crypto::detail::rotr32(x, 19U) ^ (x >> 10U);
    }

    /** @brief Processes one 64-byte block and updates the chaining state. */
    void transform(CASTLE_CONST uint8_t* block) CASTLE_NOEXCEPT
    {
        uint32_t w[64U];
        for (size_type i = 0U; i < 16U; ++i) w[i] = crypto::detail::load_be32(block + 4U * i);
        for (size_type i = 16U; i < 64U; ++i)
        {
            w[i] = small_sigma1(w[i - 2U]) + w[i - 7U] + small_sigma0(w[i - 15U]) + w[i - 16U];
        }

        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t e = state_[4];
        uint32_t f = state_[5];
        uint32_t g = state_[6];
        uint32_t h = state_[7];

        for (size_type i = 0U; i < 64U; ++i)
        {
            CASTLE_CONST uint32_t t1 = h + big_sigma1(e) + choose(e, f, g) + detail::sha256_k[i] + w[i];
            CASTLE_CONST uint32_t t2 = big_sigma0(a) + majority(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    uint32_t state_[8U];
    uint64_t bit_count_;
    uint8_t buffer_[block_size];
    size_type buffer_size_;
};

} // namespace hash
} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_HASH_SHA256_HPP
