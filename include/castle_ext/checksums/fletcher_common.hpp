// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fletcher_common.hpp
 * @brief Width-generic streaming Fletcher checksum shared by Fletcher-16/32/64.
 */
#ifndef CASTLE_EXT_CHECKSUMS_FLETCHER_COMMON_HPP
#define CASTLE_EXT_CHECKSUMS_FLETCHER_COMMON_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <limits.h>
#include <stdint.h>

namespace castle
{
namespace checksums
{
namespace detail
{

/**
 * @brief Fletcher checksum over half-width blocks with modulo (2^(W/2) - 1) sums.
 *
 * Input bytes are grouped into blocks of `sizeof(UInt) / 2` bytes, so 16/32/64-bit
 * checksums consume 8/16/32-bit blocks. A trailing partial block is zero-padded
 * when the value is read; the internal state is not modified by `value()`.
 *
 * @tparam UInt         Checksum type: an unsigned integer of 2, 4 or 8 bytes.
 * @tparam LittleEndian True to assemble multi-byte blocks least-significant byte first.
 *                      Irrelevant for 16-bit checksums (one byte per block).
 */
template <typename UInt, bool LittleEndian = true>
class basic_fletcher
{
    static_assert(meta::is_unsigned<UInt>::value, "Fletcher checksum type must be unsigned");
    static_assert(sizeof(UInt) == 2U || sizeof(UInt) == 4U || sizeof(UInt) == 8U,
                  "Fletcher checksum type must be 16, 32 or 64 bits wide");

    static CASTLE_CONSTEXPR size_type block_size = sizeof(UInt) / 2U;
    static CASTLE_CONSTEXPR size_type half_bits = block_size * CHAR_BIT;
    static CASTLE_CONSTEXPR UInt modulus =
        static_cast<UInt>((static_cast<UInt>(1U) << half_bits) - static_cast<UInt>(1U));

    /** @brief Reduces `value` modulo `modulus`. */
    // Inputs are < 2 * modulus, so a single conditional subtraction reduces them.
    static CASTLE_CONSTEXPR UInt reduce(UInt value) CASTLE_NOEXCEPT
    {
        return (value >= modulus) ? static_cast<UInt>(value - modulus) : value;
    }

    /**
     * @brief Adds one complete block to the two running sums.
     * @param[in,out] sum1 First-order sum.
     * @param[in,out] sum2 Second-order (sum of sums) accumulator.
     * @param block Assembled block value.
     */
    static void accumulate(UInt& sum1, UInt& sum2, UInt block) CASTLE_NOEXCEPT
    {
        sum1 = reduce(static_cast<UInt>(sum1 + block));
        sum2 = reduce(static_cast<UInt>(sum2 + sum1));
    }

    /** @brief Appends one byte to the pending block and accumulates it once the block is full. */
    void push_byte(uint8_t byte) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (LittleEndian)
        {
            pending_ = static_cast<UInt>(pending_ | (static_cast<UInt>(byte) << (pending_size_ * CHAR_BIT)));
        }
        else
        {
            pending_ = static_cast<UInt>((pending_ << CHAR_BIT) | static_cast<UInt>(byte));
        }

        ++pending_size_;
        if (pending_size_ == block_size)
        {
            accumulate(sum1_, sum2_, pending_);
            pending_ = static_cast<UInt>(0);
            pending_size_ = 0U;
        }
    }

public:
    /** @brief Checksum value type. */
    using value_type = UInt;

    /** @brief Constructs an engine with both sums cleared. */
    basic_fletcher() CASTLE_NOEXCEPT
        : sum1_(0U), sum2_(0U), pending_(0U), pending_size_(0U)
    {
    }

    /** @brief Clears the sums and any pending partial block. */
    void reset() CASTLE_NOEXCEPT
    {
        sum1_ = static_cast<UInt>(0);
        sum2_ = static_cast<UInt>(0);
        pending_ = static_cast<UInt>(0);
        pending_size_ = 0U;
    }

    /**
     * @brief Feeds a chunk of the message into the checksum.
     * @param data Source bytes; a null pointer is ignored.
     * @param size Number of bytes to process.
     */
    void update(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        if (data == 0)
        {
            return;
        }

        for (size_type i = 0U; i < size; ++i)
        {
            push_byte(data[i]);
        }
    }

    /** @brief Byte-pointer overload of `update()` for untyped buffers. */
    void update(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        update(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

    /**
     * @brief Returns the checksum as `(sum2 << W/2) | sum1`.
     *
     * A pending partial block is zero-padded on a copy of the sums, so the
     * stream can continue to be updated afterwards.
     */
    value_type value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        UInt sum1 = sum1_;
        UInt sum2 = sum2_;

        if (pending_size_ != 0U)
        {
            UInt block = pending_;
            CASTLE_IF_CONSTEXPR (!LittleEndian)
            {
                block = static_cast<UInt>(block << ((block_size - pending_size_) * CHAR_BIT));
            }
            accumulate(sum1, sum2, block);
        }

        return static_cast<value_type>(static_cast<UInt>(sum2 << half_bits) | sum1);
    }

    /** @brief Alias of `value()`. */
    value_type checksum() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value();
    }

    /**
     * @brief Computes the checksum of a complete message in one call.
     * @param data Source bytes; a null pointer yields the checksum of an empty message.
     * @param size Number of bytes.
     * @return Final checksum.
     */
    static value_type calculate(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        basic_fletcher fletcher;
        fletcher.update(data, size);
        return fletcher.value();
    }

    /** @brief Byte-pointer overload of `calculate()` for untyped buffers. */
    static value_type calculate(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        return calculate(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

private:
    UInt sum1_;
    UInt sum2_;
    UInt pending_;
    size_type pending_size_;
};

} // namespace detail
} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_FLETCHER_COMMON_HPP
