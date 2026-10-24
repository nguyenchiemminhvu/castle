// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file frame.hpp
 * @brief Platform-neutral classic CAN 2.0A/2.0B frame and its validation.
 *
 * `frame` is a plain, trivially copyable value type (identifier, format, type, DLC and an
 * inline 8-byte payload). Every CAN backend converts it to and from the native structure
 * (`struct can_frame` on SocketCAN, `struct can_frame` on Zephyr), so application code never
 * touches platform headers. CAN FD is out of scope: MCP2515-class controllers are classic-only.
 *
 * @code
 * castle::io::can::frame request(0x123U);
 * request.set_data({0x01U, 0x02U, 0x03U});
 *
 * castle::io::can::frame status_frame(0x18FF0001U, castle::io::can::id_format::extended);
 * status_frame.set_remote(4U);
 * @endcode
 */
#ifndef CASTLE_EXT_IO_CAN_FRAME_HPP
#define CASTLE_EXT_IO_CAN_FRAME_HPP

#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace io
{
namespace can
{

/** @brief Maximum payload length of a classic CAN frame in bytes. */
static CASTLE_INLINE CASTLE_CONSTEXPR size_type max_data_length = 8U;
/** @brief Highest 11-bit (standard) identifier. */
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t max_standard_id = 0x7FFU;
/** @brief Highest 29-bit (extended) identifier. */
static CASTLE_INLINE CASTLE_CONSTEXPR uint32_t max_extended_id = 0x1FFFFFFFU;

/** @brief Identifier format of a frame or filter. */
enum class id_format : uint8_t
{
    standard = 0U, /**< 11-bit identifier (CAN 2.0A). */
    extended       /**< 29-bit identifier (CAN 2.0B). */
};

/** @brief Frame kind: carries payload data or requests it (RTR). */
enum class frame_type : uint8_t
{
    data = 0U, /**< Data frame carrying `dlc` payload bytes. */
    remote     /**< Remote transmission request; `dlc` is the requested length. */
};

/** @brief Returns the highest identifier representable in `format`. */
CASTLE_NODISCARD CASTLE_CONSTEXPR uint32_t max_id(id_format format) CASTLE_NOEXCEPT
{
    return (format == id_format::extended) ? max_extended_id : max_standard_id;
}

/**
 * @brief Classic CAN frame.
 *
 * The payload is always stored as 8 bytes, with bytes beyond `data_length()` kept at zero by the
 * mutators, so frames compare and copy deterministically. Public fields may be assigned directly;
 * `validate_frame()` (called by `basic_bus::send()`) rejects inconsistent values.
 */
struct frame
{
    uint32_t id;
    id_format format;
    frame_type type;
    uint8_t dlc;
    container::array<uint8_t, max_data_length> data;

    /** @brief Creates a standard-format data frame with identifier 0 and no payload. */
    CASTLE_CONSTEXPR frame() CASTLE_NOEXCEPT
        : id(0U)
        , format(id_format::standard)
        , type(frame_type::data)
        , dlc(0U)
        , data{}
    {
    }

    /** @brief Creates an empty data frame with the given identifier and format. */
    explicit CASTLE_CONSTEXPR frame(uint32_t identifier,
                                    id_format id_kind = id_format::standard) CASTLE_NOEXCEPT
        : id(identifier)
        , format(id_kind)
        , type(frame_type::data)
        , dlc(0U)
        , data{}
    {
    }

    /** @brief Returns true for a 29-bit identifier. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_extended() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return format == id_format::extended;
    }

    /** @brief Returns true for a remote transmission request. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_remote() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return type == frame_type::remote;
    }

    /** @brief Returns the number of payload bytes: zero for remote frames, `dlc` (at most 8) otherwise. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type data_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_remote() ? 0U : (dlc > max_data_length ? max_data_length : static_cast<size_type>(dlc));
    }

    /** @brief Returns a read-only view of the valid payload bytes. */
    CASTLE_NODISCARD container::array_view<CASTLE_CONST uint8_t> payload() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return container::array_view<CASTLE_CONST uint8_t>(data.data(), data_length());
    }

    /**
     * @brief Turns the frame into a data frame carrying a copy of `bytes`.
     * @param bytes Source bytes; may be null only when `length` is zero.
     * @param length Number of bytes, at most `max_data_length`.
     * @return `ok`, `invalid_argument` for a null source, or `out_of_range` when `length > 8`.
     *         The frame is unchanged on failure.
     */
    castle::status set_data(CASTLE_CONST void* bytes, size_type length) CASTLE_NOEXCEPT
    {
        if (length > max_data_length)
        {
            return castle::status::out_of_range;
        }
        if (bytes == nullptr && length != 0U)
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST uint8_t* source = static_cast<CASTLE_CONST uint8_t*>(bytes);
        for (size_type i = 0U; i < max_data_length; ++i)
        {
            data[i] = (i < length) ? source[i] : static_cast<uint8_t>(0U);
        }
        type = frame_type::data;
        dlc = static_cast<uint8_t>(length);
        return castle::status::ok;
    }

    /** @brief Turns the frame into a data frame carrying the brace-enclosed bytes. */
    castle::status set_data(container::initializer_list<uint8_t> bytes) CASTLE_NOEXCEPT
    {
        return set_data(bytes.begin(), bytes.size());
    }

    /**
     * @brief Turns the frame into a remote transmission request.
     * @param requested_length Requested payload length, at most `max_data_length`.
     * @return `ok` or `out_of_range`. The frame is unchanged on failure.
     */
    castle::status set_remote(size_type requested_length) CASTLE_NOEXCEPT
    {
        if (requested_length > max_data_length)
        {
            return castle::status::out_of_range;
        }
        for (size_type i = 0U; i < max_data_length; ++i)
        {
            data[i] = 0U;
        }
        type = frame_type::remote;
        dlc = static_cast<uint8_t>(requested_length);
        return castle::status::ok;
    }
};

/** @brief Compares identifier, format, type, DLC and the valid payload bytes. */
CASTLE_NODISCARD CASTLE_INLINE bool operator==(frame CASTLE_CONST& lhs, frame CASTLE_CONST& rhs) CASTLE_NOEXCEPT
{
    if (lhs.id != rhs.id || lhs.format != rhs.format || lhs.type != rhs.type || lhs.dlc != rhs.dlc)
    {
        return false;
    }
    CASTLE_CONST size_type length = lhs.data_length();
    for (size_type i = 0U; i < length; ++i)
    {
        if (lhs.data[i] != rhs.data[i])
        {
            return false;
        }
    }
    return true;
}

/** @brief Negation of `operator==`. */
CASTLE_NODISCARD CASTLE_INLINE bool operator!=(frame CASTLE_CONST& lhs, frame CASTLE_CONST& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Checks that a frame is representable on a classic CAN bus.
 * @return `ok`, or `invalid_argument` for an out-of-range enumerator, a DLC above 8, or an
 *         identifier wider than its format allows.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status validate_frame(frame CASTLE_CONST& value) CASTLE_NOEXCEPT
{
    if ((value.format != id_format::standard && value.format != id_format::extended)
        || (value.type != frame_type::data && value.type != frame_type::remote)
        || value.dlc > max_data_length
        || value.id > max_id(value.format))
    {
        return castle::status::invalid_argument;
    }
    return castle::status::ok;
}

} // namespace can
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_CAN_FRAME_HPP
