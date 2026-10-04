// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sha512.hpp
 * @brief Streaming SHA-512 hashing primitive.
 */
#ifndef CASTLE_EXT_CRYPTO_HASH_SHA512_HPP
#define CASTLE_EXT_CRYPTO_HASH_SHA512_HPP

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
/** @brief SHA-512 round constants (FIPS 180-4, section 4.2.3). */
static inline CASTLE_CONST uint64_t sha512_k[80U] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL
};
}

/**
 * @brief SHA-512 hash function with a fixed-size streaming state.
 *
 * The message length is tracked as a 128-bit bit count.
 */
class sha512
{
public:
    /** @brief Digest length in bytes. */
    static CASTLE_CONSTEXPR size_type digest_size = 64U;
    /** @brief Internal block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 128U;
    /** @brief Fixed-size digest container. */
    using digest_type = castle::container::array<uint8_t, digest_size>;

    /** @brief Constructs a hasher in its initial state. */
    sha512() CASTLE_NOEXCEPT
    {
        reset();
    }

    /** @brief Restores the initial state, discarding all buffered input. */
    void reset() CASTLE_NOEXCEPT
    {
        state_[0] = 0x6a09e667f3bcc908ULL;
        state_[1] = 0xbb67ae8584caa73bULL;
        state_[2] = 0x3c6ef372fe94f82bULL;
        state_[3] = 0xa54ff53a5f1d36f1ULL;
        state_[4] = 0x510e527fade682d1ULL;
        state_[5] = 0x9b05688c2b3e6c1fULL;
        state_[6] = 0x1f83d9abfb41bd6bULL;
        state_[7] = 0x5be0cd19137e2179ULL;
        bit_count_low_ = 0U;
        bit_count_high_ = 0U;
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

        CASTLE_CONST uint64_t byte_count = static_cast<uint64_t>(size);
        CASTLE_CONST uint64_t bits_low = byte_count << 3U;
        CASTLE_CONST uint64_t old_low = bit_count_low_;
        bit_count_low_ += bits_low;
        bit_count_high_ += byte_count >> 61U;
        if (bit_count_low_ < old_low)
        {
            ++bit_count_high_;
        }

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
        sha512 copy(*this);
        CASTLE_CONST uint64_t original_low = copy.bit_count_low_;
        CASTLE_CONST uint64_t original_high = copy.bit_count_high_;

        copy.buffer_[copy.buffer_size_++] = 0x80U;
        if (copy.buffer_size_ > 112U)
        {
            while (copy.buffer_size_ < block_size)
            {
                copy.buffer_[copy.buffer_size_++] = 0U;
            }
            copy.transform(copy.buffer_);
            copy.buffer_size_ = 0U;
        }
        while (copy.buffer_size_ < 112U)
        {
            copy.buffer_[copy.buffer_size_++] = 0U;
        }
        crypto::detail::store_be64(copy.buffer_ + 112U, original_high);
        crypto::detail::store_be64(copy.buffer_ + 120U, original_low);
        copy.transform(copy.buffer_);

        digest_type result;
        for (size_type i = 0U; i < 8U; ++i)
        {
            crypto::detail::store_be64(&result[8U * i], copy.state_[i]);
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
        sha512 value;
        CASTLE_ASSERT(data != 0 || size == 0U, "castle::crypto::hash::sha512: invalid input");
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
    static CASTLE_CONSTEXPR uint64_t choose(uint64_t e, uint64_t f, uint64_t g) CASTLE_NOEXCEPT
    {
        return (e & f) ^ ((~e) & g);
    }

    /** @brief Maj(a, b, c): bitwise majority of the three inputs. */
    static CASTLE_CONSTEXPR uint64_t majority(uint64_t a, uint64_t b, uint64_t c) CASTLE_NOEXCEPT
    {
        return (a & b) ^ (a & c) ^ (b & c);
    }

    /** @brief Upper-case Sigma0 of FIPS 180-4 (ROTR 28, 34, 39). */
    static CASTLE_CONSTEXPR uint64_t big_sigma0(uint64_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr64(x, 28U) ^ crypto::detail::rotr64(x, 34U) ^ crypto::detail::rotr64(x, 39U);
    }

    /** @brief Upper-case Sigma1 of FIPS 180-4 (ROTR 14, 18, 41). */
    static CASTLE_CONSTEXPR uint64_t big_sigma1(uint64_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr64(x, 14U) ^ crypto::detail::rotr64(x, 18U) ^ crypto::detail::rotr64(x, 41U);
    }

    /** @brief Lower-case sigma0 of FIPS 180-4 (ROTR 1, 8, SHR 7). */
    static CASTLE_CONSTEXPR uint64_t small_sigma0(uint64_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr64(x, 1U) ^ crypto::detail::rotr64(x, 8U) ^ (x >> 7U);
    }

    /** @brief Lower-case sigma1 of FIPS 180-4 (ROTR 19, 61, SHR 6). */
    static CASTLE_CONSTEXPR uint64_t small_sigma1(uint64_t x) CASTLE_NOEXCEPT
    {
        return crypto::detail::rotr64(x, 19U) ^ crypto::detail::rotr64(x, 61U) ^ (x >> 6U);
    }

    /** @brief Processes one 128-byte block and updates the chaining state. */
    void transform(CASTLE_CONST uint8_t* block) CASTLE_NOEXCEPT
    {
        uint64_t w[80U];
        for (size_type i = 0U; i < 16U; ++i)
        {
            w[i] = crypto::detail::load_be64(block + 8U * i);
        }
        for (size_type i = 16U; i < 80U; ++i)
        {
            w[i] = small_sigma1(w[i - 2U]) + w[i - 7U] + small_sigma0(w[i - 15U]) + w[i - 16U];
        }

        uint64_t a = state_[0];
        uint64_t b = state_[1];
        uint64_t c = state_[2];
        uint64_t d = state_[3];
        uint64_t e = state_[4];
        uint64_t f = state_[5];
        uint64_t g = state_[6];
        uint64_t h = state_[7];

        for (size_type i = 0U; i < 80U; ++i)
        {
            CASTLE_CONST uint64_t t1 = h + big_sigma1(e) + choose(e, f, g) + detail::sha512_k[i] + w[i];
            CASTLE_CONST uint64_t t2 = big_sigma0(a) + majority(a, b, c);
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

    uint64_t state_[8U];
    uint64_t bit_count_low_;
    uint64_t bit_count_high_;
    uint8_t buffer_[block_size];
    size_type buffer_size_;
};

} // namespace hash
} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_HASH_SHA512_HPP
