// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file xor.hpp
 * @brief Streaming byte-wise XOR checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_XOR_HPP
#define CASTLE_EXT_CHECKSUMS_XOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{
namespace checksums
{

/**
 * @brief Byte-wise XOR checksum with a configurable initial accumulator.
 */
class xor_checksum
{
public:
    /** @brief Checksum value type. */
    using value_type = uint8_t;

    /**
     * @brief Constructs the accumulator.
     * @param initial Starting accumulator value.
     */
    explicit xor_checksum(uint8_t initial = 0U) CASTLE_NOEXCEPT
        : value_(initial)
    {
    }

    /**
     * @brief Restarts the accumulator.
     * @param initial Starting accumulator value.
     */
    void reset(uint8_t initial = 0U) CASTLE_NOEXCEPT
    {
        value_ = initial;
    }

    /**
     * @brief XORs a chunk of the message into the accumulator.
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
            value_ = static_cast<uint8_t>(value_ ^ data[i]);
        }
    }

    /** @brief Byte-pointer overload of `update()` for untyped buffers. */
    void update(CASTLE_CONST void* data, size_type size) CASTLE_NOEXCEPT
    {
        update(static_cast<CASTLE_CONST uint8_t*>(data), size);
    }

    /** @brief Returns the current accumulator value. */
    value_type value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

    /** @brief Alias of `value()`. */
    value_type checksum() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value();
    }

    /**
     * @brief Computes the XOR checksum of a complete message in one call.
     * @param data    Source bytes; a null pointer yields `initial`.
     * @param size    Number of bytes.
     * @param initial Starting accumulator value.
     * @return Final checksum.
     */
    static value_type calculate(CASTLE_CONST uint8_t* data,
                                size_type size,
                                uint8_t initial = 0U) CASTLE_NOEXCEPT
    {
        xor_checksum checksum(initial);
        checksum.update(data, size);
        return checksum.value();
    }

    /** @brief Byte-pointer overload of `calculate()` for untyped buffers. */
    static value_type calculate(CASTLE_CONST void* data,
                                size_type size,
                                uint8_t initial = 0U) CASTLE_NOEXCEPT
    {
        return calculate(static_cast<CASTLE_CONST uint8_t*>(data), size, initial);
    }

private:
    uint8_t value_;
};

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_XOR_HPP
