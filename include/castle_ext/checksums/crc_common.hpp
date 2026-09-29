// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc_common.hpp
 * @brief Width-generic, table-free streaming CRC engine shared by CRC-8/16/32/64.
 *
 * Concrete algorithms (crc8, crc16, crc32, crc64) are aliases of
 * `detail::basic_crc` with the Rocksoft-model parameters filled in.
 */
#ifndef CASTLE_EXT_CHECKSUMS_CRC_COMMON_HPP
#define CASTLE_EXT_CHECKSUMS_CRC_COMMON_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{
namespace checksums
{
namespace detail
{

/**
 * @brief Bitwise streaming CRC parameterised by the Rocksoft model.
 *
 * @tparam UInt         Unsigned register type; its width defines the CRC width.
 * @tparam Polynomial   Generator polynomial in normal (non-reflected) form.
 * @tparam Initial      Initial register value.
 * @tparam FinalXor     Value XOR-ed into the result when finalizing.
 * @tparam ReflectInput True to process each input byte LSB first.
 * @tparam ReflectOutput True to reflect the register before the final XOR.
 */
template <typename UInt,
          UInt Polynomial,
          UInt Initial,
          UInt FinalXor,
          bool ReflectInput,
          bool ReflectOutput>
class basic_crc
{
    static_assert(sizeof(UInt) >= sizeof(uint8_t), "CRC state must be at least one byte");

    static CASTLE_CONST size_type width = sizeof(UInt) * 8U;

    /** @brief Reverses the bit order of `value` across the full CRC width. */
    static CASTLE_CONSTEXPR UInt reflect(UInt value) CASTLE_NOEXCEPT
    {
        UInt result = static_cast<UInt>(0);
        for (size_type i = 0U; i < width; ++i)
        {
            result = static_cast<UInt>((result << 1U) | (value & static_cast<UInt>(1U)));
            value = static_cast<UInt>(value >> 1U);
        }
        return result;
    }

    /** @brief Returns `Polynomial` in reflected form, used by the LSB-first path. */
    static CASTLE_CONSTEXPR UInt reflected_polynomial() CASTLE_NOEXCEPT
    {
        return reflect(Polynomial);
    }

    /**
     * @brief Folds one input byte into a CRC register.
     * @param crc   Current register value.
     * @param value Input byte.
     * @return Updated register value.
     */
    static UInt update_byte(UInt crc, uint8_t value) CASTLE_NOEXCEPT
    {
        if (ReflectInput)
        {
            crc = static_cast<UInt>(crc ^ static_cast<UInt>(value));
            CASTLE_CONST UInt polynomial = reflected_polynomial();
            for (size_type i = 0U; i < 8U; ++i)
            {
                if ((crc & static_cast<UInt>(1U)) != static_cast<UInt>(0))
                {
                    crc = static_cast<UInt>((crc >> 1U) ^ polynomial);
                }
                else
                {
                    crc = static_cast<UInt>(crc >> 1U);
                }
            }
        }
        else
        {
            crc = static_cast<UInt>(crc ^ (static_cast<UInt>(value) << (width - 8U)));
            for (size_type i = 0U; i < 8U; ++i)
            {
                if ((crc & (static_cast<UInt>(1U) << (width - 1U))) != static_cast<UInt>(0))
                {
                    crc = static_cast<UInt>((crc << 1U) ^ Polynomial);
                }
                else
                {
                    crc = static_cast<UInt>(crc << 1U);
                }
            }
        }
        return crc;
    }

    /** @brief Applies output reflection (if it differs from input reflection) and the final XOR. */
    static UInt finalize(UInt crc) CASTLE_NOEXCEPT
    {
        if (ReflectInput != ReflectOutput)
        {
            crc = reflect(crc);
        }
        return static_cast<UInt>(crc ^ FinalXor);
    }

public:
    /** @brief Checksum value type. */
    using value_type = UInt;

    /** @brief Constructs an engine holding the initial register value. */
    basic_crc() CASTLE_NOEXCEPT
        : state_(Initial)
    {
    }

    /** @brief Restores the initial register value. */
    void reset() CASTLE_NOEXCEPT
    {
        state_ = Initial;
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
            state_ = update_byte(state_, data[i]);
        }
    }

    /** @brief Byte-pointer overload of `update()` for untyped buffers. */
    void update(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        update(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

    /** @brief Returns the finalized checksum; the running state is not modified. */
    value_type value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return finalize(state_);
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
        basic_crc crc;
        crc.update(data, size);
        return crc.value();
    }

    /** @brief Byte-pointer overload of `calculate()` for untyped buffers. */
    static value_type calculate(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        return calculate(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

private:
    value_type state_;
};

} // namespace detail
} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_CRC_COMMON_HPP
