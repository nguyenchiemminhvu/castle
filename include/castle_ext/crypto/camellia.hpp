// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file camellia.hpp
 * @brief Heap-free Camellia-128, Camellia-192, and Camellia-256 block cipher.
 *
 * Camellia uses a 128-bit block and supports 128-, 192-, and 256-bit keys.
 * This header exposes raw block encryption/decryption and CTR processing;
 * authentication is deliberately left to a separate construction.
 */
#ifndef CASTLE_EXT_CRYPTO_CAMELLIA_HPP
#define CASTLE_EXT_CRYPTO_CAMELLIA_HPP

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

/** @brief Camellia S-box 1 from RFC 3713, section 2.4.1. */
static inline CASTLE_CONST uint8_t camellia_sbox1[256U] = {
    112U,130U, 44U,236U,179U, 39U,192U,229U,228U,133U, 87U, 53U,234U, 12U,174U, 65U,
     35U,239U,107U,147U, 69U, 25U,165U, 33U,237U, 14U, 79U, 78U, 29U,101U,146U,189U,
    134U,184U,175U,143U,124U,235U, 31U,206U, 62U, 48U,220U, 95U, 94U,197U, 11U, 26U,
    166U,225U, 57U,202U,213U, 71U, 93U, 61U,217U,  1U, 90U,214U, 81U, 86U,108U, 77U,
    139U, 13U,154U,102U,251U,204U,176U, 45U,116U, 18U, 43U, 32U,240U,177U,132U,153U,
    223U, 76U,203U,194U, 52U,126U,118U,  5U,109U,183U,169U, 49U,209U, 23U,  4U,215U,
     20U, 88U, 58U, 97U,222U, 27U, 17U, 28U, 50U, 15U,156U, 22U, 83U, 24U,242U, 34U,
    254U, 68U,207U,178U,195U,181U,122U,145U, 36U,  8U,232U,168U, 96U,252U,105U, 80U,
    170U,208U,160U,125U,161U,137U, 98U,151U, 84U, 91U, 30U,149U,224U,255U,100U,210U,
     16U,196U,  0U, 72U,163U,247U,117U,219U,138U,  3U,230U,218U,  9U, 63U,221U,148U,
    135U, 92U,131U,  2U,205U, 74U,144U, 51U,115U,103U,246U,243U,157U,127U,191U,226U,
     82U,155U,216U, 38U,200U, 55U,198U, 59U,129U,150U,111U, 75U, 19U,190U, 99U, 46U,
    233U,121U,167U,140U,159U,110U,188U,142U, 41U,245U,249U,182U, 47U,253U,180U, 89U,
    120U,152U,  6U,106U,231U, 70U,113U,186U,212U, 37U,171U, 66U,136U,162U,141U,250U,
    114U,  7U,185U, 85U,248U,238U,172U, 10U, 54U, 73U, 42U,104U, 60U, 56U,241U,164U,
     64U, 40U,211U,123U,187U,201U, 67U,193U, 21U,227U,173U,244U,119U,199U,128U,158U
};

/** @brief Camellia key-schedule sigma constants from RFC 3713. */
static inline CASTLE_CONST uint64_t camellia_sigma[6U] = {
    0xA09E667F3BCC908BULL,
    0xB67AE8584CAA73B2ULL,
    0xC6EF372FE94F82BEULL,
    0x54FF53A5F1D36F1CULL,
    0x10E527FADE682D1DULL,
    0xB05688C2B3E6C1FDULL
};

/** @brief 8-bit left rotation used to derive Camellia S-boxes 2 and 3. */
static CASTLE_CONSTEXPR uint8_t camellia_rotl8(uint8_t value, uint32_t count) CASTLE_NOEXCEPT
{
    return static_cast<uint8_t>((static_cast<uint32_t>(value) << count) |
                                (static_cast<uint32_t>(value) >> (8U - count)));
}

/** @brief 128-bit left rotation represented as two 64-bit halves. */
static void camellia_rotl128(uint64_t hi, uint64_t lo, uint32_t count,
                             uint64_t& out_hi, uint64_t& out_lo) CASTLE_NOEXCEPT
{
    count %= 128U;
    if (count == 0U)
    {
        out_hi = hi;
        out_lo = lo;
    }
    else if (count < 64U)
    {
        out_hi = (hi << count) | (lo >> (64U - count));
        out_lo = (lo << count) | (hi >> (64U - count));
    }
    else
    {
        CASTLE_CONST uint32_t shift = count - 64U;
        if (shift == 0U)
        {
            out_hi = lo;
            out_lo = hi;
        }
        else
        {
            out_hi = (lo << shift) | (hi >> (64U - shift));
            out_lo = (hi << shift) | (lo >> (64U - shift));
        }
    }
}

/** @brief Camellia F-function. */
static uint64_t camellia_f(uint64_t input, uint64_t key) CASTLE_NOEXCEPT
{
    CASTLE_CONST uint64_t x = input ^ key;
    const uint8_t t1 = camellia_sbox1[(x >> 56U) & 0xFFU];
    const uint8_t t2 = camellia_rotl8(camellia_sbox1[(x >> 48U) & 0xFFU], 1U);
    const uint8_t t3 = camellia_rotl8(camellia_sbox1[(x >> 40U) & 0xFFU], 7U);
    const uint8_t t4_index = static_cast<uint8_t>((((x >> 32U) & 0xFFU) << 1U) | (((x >> 32U) & 0xFFU) >> 7U));
    const uint8_t t4 = camellia_sbox1[t4_index];
    const uint8_t t5 = camellia_rotl8(camellia_sbox1[(x >> 24U) & 0xFFU], 1U);
    const uint8_t t6 = camellia_rotl8(camellia_sbox1[(x >> 16U) & 0xFFU], 7U);
    const uint8_t t7_index = static_cast<uint8_t>((((x >> 8U) & 0xFFU) << 1U) | (((x >> 8U) & 0xFFU) >> 7U));
    const uint8_t t7 = camellia_sbox1[t7_index];
    const uint8_t t8 = camellia_sbox1[x & 0xFFU];

    const uint8_t y1 = static_cast<uint8_t>(t1 ^ t3 ^ t4 ^ t6 ^ t7 ^ t8);
    const uint8_t y2 = static_cast<uint8_t>(t1 ^ t2 ^ t4 ^ t5 ^ t7 ^ t8);
    const uint8_t y3 = static_cast<uint8_t>(t1 ^ t2 ^ t3 ^ t5 ^ t6 ^ t8);
    const uint8_t y4 = static_cast<uint8_t>(t2 ^ t3 ^ t4 ^ t5 ^ t6 ^ t7);
    const uint8_t y5 = static_cast<uint8_t>(t1 ^ t2 ^ t6 ^ t7 ^ t8);
    const uint8_t y6 = static_cast<uint8_t>(t2 ^ t3 ^ t5 ^ t7 ^ t8);
    const uint8_t y7 = static_cast<uint8_t>(t3 ^ t4 ^ t5 ^ t6 ^ t8);
    const uint8_t y8 = static_cast<uint8_t>(t1 ^ t4 ^ t5 ^ t6 ^ t7);

    return (static_cast<uint64_t>(y1) << 56U) |
           (static_cast<uint64_t>(y2) << 48U) |
           (static_cast<uint64_t>(y3) << 40U) |
           (static_cast<uint64_t>(y4) << 32U) |
           (static_cast<uint64_t>(y5) << 24U) |
           (static_cast<uint64_t>(y6) << 16U) |
           (static_cast<uint64_t>(y7) << 8U) |
           static_cast<uint64_t>(y8);
}

/** @brief Camellia FL-function. */
static uint64_t camellia_fl(uint64_t input, uint64_t key) CASTLE_NOEXCEPT
{
    uint32_t x1 = static_cast<uint32_t>(input >> 32U);
    uint32_t x2 = static_cast<uint32_t>(input);
    const uint32_t k1 = static_cast<uint32_t>(key >> 32U);
    const uint32_t k2 = static_cast<uint32_t>(key);
    x2 ^= castle::rotl32(x1 & k1, 1U);
    x1 ^= x2 | k2;
    return (static_cast<uint64_t>(x1) << 32U) | x2;
}

/** @brief Camellia FLINV-function. */
static uint64_t camellia_flinv(uint64_t input, uint64_t key) CASTLE_NOEXCEPT
{
    uint32_t y1 = static_cast<uint32_t>(input >> 32U);
    uint32_t y2 = static_cast<uint32_t>(input);
    const uint32_t k1 = static_cast<uint32_t>(key >> 32U);
    const uint32_t k2 = static_cast<uint32_t>(key);
    y1 ^= y2 | k2;
    y2 ^= castle::rotl32(y1 & k1, 1U);
    return (static_cast<uint64_t>(y1) << 32U) | y2;
}

} // namespace detail

/**
 * @brief Camellia block cipher with compile-time selected key size.
 * @tparam KeyBits One of 128, 192, or 256.
 */
template <size_type KeyBits>
class camellia
{
    static_assert(KeyBits == 128U || KeyBits == 192U || KeyBits == 256U,
                  "castle::crypto::camellia supports 128, 192, or 256-bit keys");

public:
    /** @brief Block length in bytes. */
    static CASTLE_CONSTEXPR size_type block_size = 16U;
    /** @brief Key length in bytes. */
    static CASTLE_CONSTEXPR size_type key_size = KeyBits / 8U;
    /** @brief Number of Feistel rounds. */
    static CASTLE_CONSTEXPR size_type rounds = (KeyBits == 128U) ? 18U : 24U;

    /** @brief Constructs an instance with an all-zero key. */
    camellia() CASTLE_NOEXCEPT
    {
        const uint8_t zero_key[key_size] = {0U};
        static_cast<void>(set_key(zero_key));
    }

    /**
     * @brief Constructs an instance from a key.
     * @param key Key bytes; null selects an all-zero key for consistency with Castle AES.
     */
    explicit camellia(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
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
    ~camellia() CASTLE_NOEXCEPT
    {
        castle::secure_zero(k_, sizeof(k_));
        castle::secure_zero(kw_, sizeof(kw_));
        castle::secure_zero(ke_, sizeof(ke_));
    }

    /**
     * @brief Replaces the current key and computes all Camellia subkeys.
     * @param key Exactly `key_size` bytes.
     * @return `status::ok`, or `status::invalid_argument` when `key == nullptr`.
     */
    CASTLE_NODISCARD status set_key(CASTLE_CONST uint8_t* key) CASTLE_NOEXCEPT
    {
        if (key == 0)
        {
            return status::invalid_argument;
        }

        const uint64_t kl_hi = castle::read_be64(key);
        const uint64_t kl_lo = castle::read_be64(key + 8U);
        uint64_t kr_hi = 0ULL;
        uint64_t kr_lo = 0ULL;

        if (KeyBits == 192U)
        {
            kr_lo = castle::read_be64(key + 16U);
            kr_hi = kr_lo;
            kr_lo = ~kr_lo;
        }
        else if (KeyBits == 256U)
        {
            kr_hi = castle::read_be64(key + 16U);
            kr_lo = castle::read_be64(key + 24U);
        }

        uint64_t ka_hi;
        uint64_t ka_lo;
        uint64_t kb_hi;
        uint64_t kb_lo;
        make_intermediate_keys(kl_hi, kl_lo, kr_hi, kr_lo, ka_hi, ka_lo, kb_hi, kb_lo);

        kw_[0] = kl_hi;
        kw_[1] = kl_lo;

        uint64_t hi;
        uint64_t lo;
        if (KeyBits == 128U)
        {
            set_pair_from_rotation(ka_hi, ka_lo, 0U, k_[0], k_[1]);
            set_pair_from_rotation(kl_hi, kl_lo, 15U, k_[2], k_[3]);
            set_pair_from_rotation(ka_hi, ka_lo, 15U, k_[4], k_[5]);
            set_pair_from_rotation(ka_hi, ka_lo, 30U, ke_[0], ke_[1]);
            set_pair_from_rotation(kl_hi, kl_lo, 45U, k_[6], k_[7]);
            set_pair_from_rotation(ka_hi, ka_lo, 45U, k_[8], k_[9]);
            set_pair_from_rotation(kl_hi, kl_lo, 60U, hi, lo);
            k_[9] = lo;
            set_pair_from_rotation(ka_hi, ka_lo, 60U, k_[10], k_[11]);
            set_pair_from_rotation(kl_hi, kl_lo, 77U, ke_[2], ke_[3]);
            set_pair_from_rotation(kl_hi, kl_lo, 94U, k_[12], k_[13]);
            set_pair_from_rotation(ka_hi, ka_lo, 94U, k_[14], k_[15]);
            set_pair_from_rotation(kl_hi, kl_lo, 111U, k_[16], k_[17]);
            set_pair_from_rotation(ka_hi, ka_lo, 111U, kw_[2], kw_[3]);
            for (size_type i = 18U; i < 24U; ++i) k_[i] = 0ULL;
            for (size_type i = 4U; i < 6U; ++i) ke_[i] = 0ULL;
        }
        else
        {
            set_pair_from_rotation(kb_hi, kb_lo, 0U, k_[0], k_[1]);
            set_pair_from_rotation(kr_hi, kr_lo, 15U, k_[2], k_[3]);
            set_pair_from_rotation(ka_hi, ka_lo, 15U, k_[4], k_[5]);
            set_pair_from_rotation(kr_hi, kr_lo, 30U, ke_[0], ke_[1]);
            set_pair_from_rotation(kb_hi, kb_lo, 30U, k_[6], k_[7]);
            set_pair_from_rotation(kl_hi, kl_lo, 45U, k_[8], k_[9]);
            set_pair_from_rotation(ka_hi, ka_lo, 45U, k_[10], k_[11]);
            set_pair_from_rotation(kl_hi, kl_lo, 60U, ke_[2], ke_[3]);
            set_pair_from_rotation(kr_hi, kr_lo, 60U, k_[12], k_[13]);
            set_pair_from_rotation(kb_hi, kb_lo, 60U, k_[14], k_[15]);
            set_pair_from_rotation(kl_hi, kl_lo, 77U, k_[16], k_[17]);
            set_pair_from_rotation(ka_hi, ka_lo, 77U, ke_[4], ke_[5]);
            set_pair_from_rotation(kr_hi, kr_lo, 94U, k_[18], k_[19]);
            set_pair_from_rotation(ka_hi, ka_lo, 94U, k_[20], k_[21]);
            set_pair_from_rotation(kl_hi, kl_lo, 111U, k_[22], k_[23]);
            set_pair_from_rotation(kb_hi, kb_lo, 111U, kw_[2], kw_[3]);
        }

        return status::ok;
    }

    /**
     * @brief Encrypts one 16-byte block.
     * @param input Plaintext block.
     * @param output Ciphertext block; may alias `input`.
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
     */
    void decrypt_block(CASTLE_CONST uint8_t input[block_size], uint8_t output[block_size])
        CASTLE_CONST CASTLE_NOEXCEPT
    {
        crypt_block(input, output, true);
    }

    /**
     * @brief Applies the Camellia-CTR keystream to an arbitrary-length buffer in place.
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

    static void set_pair_from_rotation(uint64_t hi, uint64_t lo, uint32_t count,
                                       uint64_t& out_hi, uint64_t& out_lo) CASTLE_NOEXCEPT
    {
        detail::camellia_rotl128(hi, lo, count, out_hi, out_lo);
    }

    static void make_intermediate_keys(uint64_t kl_hi, uint64_t kl_lo,
                                       uint64_t kr_hi, uint64_t kr_lo,
                                       uint64_t& ka_hi, uint64_t& ka_lo,
                                       uint64_t& kb_hi, uint64_t& kb_lo) CASTLE_NOEXCEPT
    {
        uint64_t d1 = kl_hi ^ kr_hi;
        uint64_t d2 = kl_lo ^ kr_lo;
        d2 ^= detail::camellia_f(d1, detail::camellia_sigma[0]);
        d1 ^= detail::camellia_f(d2, detail::camellia_sigma[1]);
        d1 ^= kl_hi;
        d2 ^= kl_lo;
        d2 ^= detail::camellia_f(d1, detail::camellia_sigma[2]);
        d1 ^= detail::camellia_f(d2, detail::camellia_sigma[3]);
        ka_hi = d1;
        ka_lo = d2;

        d1 = ka_hi ^ kr_hi;
        d2 = ka_lo ^ kr_lo;
        d2 ^= detail::camellia_f(d1, detail::camellia_sigma[4]);
        d1 ^= detail::camellia_f(d2, detail::camellia_sigma[5]);
        kb_hi = d1;
        kb_lo = d2;
    }

    void crypt_block(CASTLE_CONST uint8_t input[block_size],
                     uint8_t output[block_size],
                     bool decrypt) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (input == 0 || output == 0)
        {
            return;
        }

        uint64_t d1 = castle::read_be64(input);
        uint64_t d2 = castle::read_be64(input + 8U);

        // Decryption uses the same data path with the complete subkey order reversed.
        const uint64_t whitening1 = decrypt ? kw_[2] : kw_[0];
        const uint64_t whitening2 = decrypt ? kw_[3] : kw_[1];
        d1 ^= whitening1;
        d2 ^= whitening2;

        const size_type stage_count = rounds / 6U;
        for (size_type stage = 0U; stage < stage_count; ++stage)
        {
            const size_type stage_round_base = stage * 6U;
            for (size_type round = 0U; round < 6U; ++round)
            {
                const size_type logical_round = stage_round_base + round;
                const size_type key_index = decrypt ? (rounds - 1U - logical_round) : logical_round;
                if ((round & 1U) == 0U)
                {
                    d2 ^= detail::camellia_f(d1, k_[key_index]);
                }
                else
                {
                    d1 ^= detail::camellia_f(d2, k_[key_index]);
                }
            }

            if (stage + 1U < stage_count)
            {
                const size_type fl_stage = decrypt ? (stage_count - 2U - stage) : stage;
                if (decrypt)
                {
                    d1 = detail::camellia_fl(d1, ke_[2U * fl_stage + 1U]);
                    d2 = detail::camellia_flinv(d2, ke_[2U * fl_stage]);
                }
                else
                {
                    d1 = detail::camellia_fl(d1, ke_[2U * fl_stage]);
                    d2 = detail::camellia_flinv(d2, ke_[2U * fl_stage + 1U]);
                }
            }
        }

        if (decrypt)
        {
            d1 ^= kw_[1];
            d2 ^= kw_[0];
        }
        else
        {
            d2 ^= kw_[2];
            d1 ^= kw_[3];
        }

        castle::write_be64(output, d2);
        castle::write_be64(output + 8U, d1);
    }

    uint64_t kw_[4U];
    uint64_t k_[24U];
    uint64_t ke_[6U];
};

} // namespace crypto
} // namespace castle

#endif // CASTLE_EXT_CRYPTO_CAMELLIA_HPP
