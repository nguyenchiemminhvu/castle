// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file frame.hpp
 * @brief Fixed-capacity, hardware-independent representation of CAN frames.
 *
 * The type models Classical CAN and ISO CAN FD data frames, plus Classical CAN
 * remote, error, and overload frame kinds. It does not emulate a CAN controller
 * or serialize physical-layer bit timing, arbitration, ACK, or error signaling.
 */
#ifndef CASTLE_EXT_PROTOCOLS_CAN_FRAME_HPP
#define CASTLE_EXT_PROTOCOLS_CAN_FRAME_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array_view.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace can
{

static CASTLE_CONSTEXPR castle::size_type CAN_STANDARD_IDENTIFIER_BITS = 11U;
static CASTLE_CONSTEXPR castle::size_type CAN_EXTENDED_IDENTIFIER_BITS = 29U;
static CASTLE_CONSTEXPR uint32_t CAN_STANDARD_IDENTIFIER_MAX = 0x7FFU;
static CASTLE_CONSTEXPR uint32_t CAN_EXTENDED_IDENTIFIER_MAX = 0x1FFFFFFFU;
static CASTLE_CONSTEXPR castle::size_type CAN_CLASSICAL_MAX_DATA_LENGTH = 8U;
static CASTLE_CONSTEXPR castle::size_type CAN_FD_MAX_DATA_LENGTH = 64U;

/** @brief Identifier width used in the arbitration field. */
enum class identifier_format : uint8_t
{
    standard = 0U, /**< CAN 2.0A, 11-bit identifier. */
    extended = 1U  /**< CAN 2.0B, 29-bit identifier. */
};

/** @brief Logical CAN frame kind. */
enum class frame_kind : uint8_t
{
    data = 0U,
    remote = 1U,
    error = 2U,
    overload = 3U
};

/** @brief CAN data-link frame format. */
enum class frame_format : uint8_t
{
    classical = 0U, /**< Classical CAN 2.0, at most 8 data bytes. */
    fd = 1U         /**< ISO CAN FD, at most 64 data bytes. */
};

/**
 * @brief Tests whether a payload byte count can be represented by a CAN FD DLC.
 * @param length Payload length in bytes.
 * @return True for 0..8, 12, 16, 20, 24, 32, 48, or 64 bytes.
 */
CASTLE_CONSTEXPR inline bool is_valid_fd_data_length(castle::size_type length) CASTLE_NOEXCEPT
{
    return (length <= 8U)
        || (length == 12U)
        || (length == 16U)
        || (length == 20U)
        || (length == 24U)
        || (length == 32U)
        || (length == 48U)
        || (length == 64U);
}

/**
 * @brief Maps a CAN FD payload byte count to its 4-bit DLC value.
 * @param length Payload length in bytes.
 * @return DLC 0..15, or 0xFF when the length is not representable.
 */
CASTLE_CONSTEXPR inline uint8_t fd_data_length_code(castle::size_type length) CASTLE_NOEXCEPT
{
    return (length <= 8U) ? static_cast<uint8_t>(length)
        : (length == 12U) ? 9U
        : (length == 16U) ? 10U
        : (length == 20U) ? 11U
        : (length == 24U) ? 12U
        : (length == 32U) ? 13U
        : (length == 48U) ? 14U
        : (length == 64U) ? 15U
        : 0xFFU;
}

/**
 * @brief Fixed-capacity CAN frame value.
 *
 * `data_length` is the number of payload bytes for data frames and the requested
 * data length (DLC value 0..8) for remote frames. For error/overload signals it
 * must be zero. CAN FD payload lengths follow the discrete DLC mapping.
 *
 * A true `error_state_indicator` means the CAN FD sender was error-passive;
 * false means error-active. Error and overload frames are controller-generated
 * signaling events, not ordinary application messages.
 */
struct frame
{
    uint32_t identifier = 0U;
    identifier_format id_format = identifier_format::standard;
    frame_kind kind = frame_kind::data;
    frame_format format = frame_format::classical;
    bool bit_rate_switch = false;
    bool error_state_indicator = false;
    uint8_t data_length = 0U;
    uint8_t data[CAN_FD_MAX_DATA_LENGTH] = {};

    /** @brief Returns whether all identifier, type, flag, and length fields are valid. */
    CASTLE_NODISCARD bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if ((id_format != identifier_format::standard)
            && (id_format != identifier_format::extended))
        {
            return false;
        }
        if ((kind != frame_kind::data) && (kind != frame_kind::remote)
            && (kind != frame_kind::error) && (kind != frame_kind::overload))
        {
            return false;
        }
        if ((format != frame_format::classical) && (format != frame_format::fd))
        {
            return false;
        }
        if ((id_format == identifier_format::standard && identifier > CAN_STANDARD_IDENTIFIER_MAX)
            || (id_format == identifier_format::extended && identifier > CAN_EXTENDED_IDENTIFIER_MAX))
        {
            return false;
        }

        if (kind == frame_kind::error || kind == frame_kind::overload)
        {
            return (format == frame_format::classical)
                && (data_length == 0U)
                && !bit_rate_switch
                && !error_state_indicator;
        }

        if (kind == frame_kind::remote)
        {
            return (format == frame_format::classical)
                && (data_length <= CAN_CLASSICAL_MAX_DATA_LENGTH)
                && !bit_rate_switch
                && !error_state_indicator;
        }

        if (format == frame_format::classical)
        {
            return (data_length <= CAN_CLASSICAL_MAX_DATA_LENGTH)
                && !bit_rate_switch
                && !error_state_indicator;
        }

        return is_valid_fd_data_length(data_length);
    }

    /** @brief Returns true for an FD data frame. */
    CASTLE_NODISCARD bool is_fd() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return format == frame_format::fd;
    }

    /** @brief Returns true when this frame carries application data bytes. */
    CASTLE_NODISCARD bool has_data() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return kind == frame_kind::data && data_length != 0U; // LCOV_EXCL_BR_LINE
    }

    /** @brief Returns the 4-bit data length code, or 0xFF for an invalid frame. */
    CASTLE_NODISCARD uint8_t data_length_code() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid() || (kind != frame_kind::data && kind != frame_kind::remote))
        {
            return 0xFFU;
        }
        if (kind == frame_kind::remote || format == frame_format::classical)
        {
            return data_length;
        }
        return fd_data_length_code(data_length);
    }
};

/**
 * @brief Creates a validated data frame without dynamic allocation.
 * @param identifier Arbitration identifier.
 * @param id_format Standard (11-bit) or extended (29-bit) identifier.
 * @param format Classical CAN or CAN FD.
 * @param payload Source bytes; may be null only when `length == 0`.
 * @param length Payload byte count.
 * @param output Destination frame, modified only on success.
 * @param bit_rate_switch Enable the CAN FD faster data phase.
 * @param error_state_indicator Set the FD ESI bit (error-passive sender).
 */
inline castle::status make_data_frame(
    uint32_t identifier,
    identifier_format id_format,
    frame_format format,
    CASTLE_CONST uint8_t* payload,
    castle::size_type length,
    frame& output,
    bool bit_rate_switch = false,
    bool error_state_indicator = false) CASTLE_NOEXCEPT
{
    if ((length != 0U && payload == nullptr) || length > CAN_FD_MAX_DATA_LENGTH)
    {
        return castle::status::invalid_argument;
    }

    frame candidate{};
    candidate.identifier = identifier;
    candidate.id_format = id_format;
    candidate.kind = frame_kind::data;
    candidate.format = format;
    candidate.bit_rate_switch = bit_rate_switch;
    candidate.error_state_indicator = error_state_indicator;
    candidate.data_length = static_cast<uint8_t>(length);
    // Reject invalid lengths, identifiers, formats, or flags before reading
    // the caller's payload memory. This also prevents over-reading a short
    // payload buffer when, for example, a Classical CAN length exceeds 8.
    if (!candidate.valid())
    {
        return castle::status::invalid_argument;
    }
    for (castle::size_type index = 0U; index < length; ++index)
    {
        candidate.data[index] = payload[index];
    }
    output = candidate;
    return castle::status::ok;
}

/**
 * @brief Creates a data frame from a non-owning Castle byte view.
 * @note The payload is copied into the frame's inline storage; the view need
 * only remain valid for the duration of this call.
 */
inline castle::status make_data_frame(
    uint32_t identifier,
    identifier_format id_format,
    frame_format format,
    castle::container::array_view<CASTLE_CONST uint8_t> payload,
    frame& output,
    bool bit_rate_switch = false,
    bool error_state_indicator = false) CASTLE_NOEXCEPT
{
    return make_data_frame(identifier, id_format, format, payload.data(), payload.size(),
        output, bit_rate_switch, error_state_indicator);
}

/** @brief Creates a Classical CAN remote request with the requested DLC (0..8). */
inline castle::status make_remote_frame(
    uint32_t identifier,
    identifier_format id_format,
    uint8_t requested_length,
    frame& output) CASTLE_NOEXCEPT
{
    frame candidate{};
    candidate.identifier = identifier;
    candidate.id_format = id_format;
    candidate.kind = frame_kind::remote;
    candidate.format = frame_format::classical;
    candidate.data_length = requested_length;
    if (!candidate.valid())
    {
        return castle::status::invalid_argument;
    }
    output = candidate;
    return castle::status::ok;
}

/** @brief Creates a controller-level Classical CAN error signaling event value. */
inline castle::status make_error_frame(
    uint32_t identifier,
    identifier_format id_format,
    frame& output) CASTLE_NOEXCEPT
{
    frame candidate{};
    candidate.identifier = identifier;
    candidate.id_format = id_format;
    candidate.kind = frame_kind::error;

    // LCOV_EXCL_START
    if (!candidate.valid())
    {
        return castle::status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    output = candidate;
    return castle::status::ok;
}

/** @brief Creates a controller-level Classical CAN overload signaling event value. */
inline castle::status make_overload_frame(
    uint32_t identifier,
    identifier_format id_format,
    frame& output) CASTLE_NOEXCEPT
{
    frame candidate{};
    candidate.identifier = identifier;
    candidate.id_format = id_format;
    candidate.kind = frame_kind::overload;

    // LCOV_EXCL_START
    if (!candidate.valid())
    {
        return castle::status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    output = candidate;
    return castle::status::ok;
}

} // namespace can
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_CAN_FRAME_HPP
