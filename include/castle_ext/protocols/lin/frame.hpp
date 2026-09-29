// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file frame.hpp
 * @brief Fixed-capacity LIN headers and response frames.
 *
 * This header models the logical LIN data-link fields. Break generation,
 * synchronization timing, UART framing, and the physical transceiver remain
 * the responsibility of a target-specific adapter.
 */
#ifndef CASTLE_EXT_PROTOCOLS_LIN_FRAME_HPP
#define CASTLE_EXT_PROTOCOLS_LIN_FRAME_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace lin
{

static CASTLE_CONSTEXPR uint8_t LIN_SYNC_BYTE = 0x55U;
static CASTLE_CONSTEXPR castle::size_type LIN_IDENTIFIER_BITS = 6U;
static CASTLE_CONSTEXPR uint8_t LIN_IDENTIFIER_MAX = 0x3FU;
static CASTLE_CONSTEXPR uint8_t LIN_DIAGNOSTIC_REQUEST_IDENTIFIER = 0x3CU;
static CASTLE_CONSTEXPR uint8_t LIN_DIAGNOSTIC_RESPONSE_IDENTIFIER = 0x3DU;
static CASTLE_CONSTEXPR uint8_t LIN_RESERVED_IDENTIFIER_FIRST = 0x3EU;
static CASTLE_CONSTEXPR castle::size_type LIN_MAX_DATA_LENGTH = 8U;
static CASTLE_CONSTEXPR uint8_t LIN_INVALID_PROTECTED_IDENTIFIER = 0xFFU;

/** @brief Checksum mode selected by the network description. */
enum class checksum_type : uint8_t
{
    classic = 0U,  /**< Checksum covers data bytes only. */
    enhanced = 1U  /**< Checksum covers the protected identifier and data bytes. */
};

/** @brief Identifies the node responsible for supplying a frame response. */
enum class response_owner : uint8_t
{
    master = 0U,
    slave = 1U
};

/** @brief Returns whether an identifier is usable by LIN 2.x framing. */
CASTLE_CONSTEXPR inline bool is_valid_identifier(uint32_t identifier) CASTLE_NOEXCEPT
{
    return identifier < LIN_RESERVED_IDENTIFIER_FIRST;
}

/**
 * @brief Calculates the parity-protected identifier (PID) for a LIN frame ID.
 * @return PID byte, or `LIN_INVALID_PROTECTED_IDENTIFIER` for reserved IDs.
 */
CASTLE_CONSTEXPR inline uint8_t calculate_protected_identifier(uint32_t identifier) CASTLE_NOEXCEPT
{
    if (!is_valid_identifier(identifier))
    {
        return LIN_INVALID_PROTECTED_IDENTIFIER;
    }

    const uint8_t id0 = static_cast<uint8_t>((identifier >> 0U) & 1U);
    const uint8_t id1 = static_cast<uint8_t>((identifier >> 1U) & 1U);
    const uint8_t id2 = static_cast<uint8_t>((identifier >> 2U) & 1U);
    const uint8_t id3 = static_cast<uint8_t>((identifier >> 3U) & 1U);
    const uint8_t id4 = static_cast<uint8_t>((identifier >> 4U) & 1U);
    const uint8_t id5 = static_cast<uint8_t>((identifier >> 5U) & 1U);
    const uint8_t p0 = static_cast<uint8_t>(id0 ^ id1 ^ id2 ^ id4);
    const uint8_t p1 = static_cast<uint8_t>(!(id1 ^ id3 ^ id4 ^ id5));
    return static_cast<uint8_t>(static_cast<uint8_t>(identifier) | (p0 << 6U) | (p1 << 7U));
}

/** @brief Tests a received PID for legal ID range and correct parity bits. */
CASTLE_CONSTEXPR inline bool is_valid_protected_identifier(uint8_t pid) CASTLE_NOEXCEPT
{
    const uint8_t identifier = static_cast<uint8_t>(pid & 0x3FU);
    return is_valid_identifier(identifier)
        && calculate_protected_identifier(identifier) == pid;
}

/** @brief Logical LIN header; the UART adapter supplies break and sync fields. */
struct header
{
    uint8_t identifier = 0U;
    uint8_t protected_identifier = calculate_protected_identifier(0U);

    CASTLE_NODISCARD bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_valid_identifier(identifier)
            && protected_identifier == calculate_protected_identifier(identifier);
    }
};

/**
 * @brief Complete, fixed-capacity LIN response including its checksum.
 *
 * `valid()` checks field shape and PID parity. Use `validate_checksum()` from
 * `checksum.hpp` to verify the response checksum as well.
 */
struct frame
{
    uint8_t identifier = 0U;
    uint8_t protected_identifier = calculate_protected_identifier(0U);
    uint8_t data_length = 1U;
    uint8_t data[LIN_MAX_DATA_LENGTH] = {};
    uint8_t checksum = 0U;
    checksum_type checksum_mode = checksum_type::enhanced;

    /** @brief Tests identifier, PID, payload length, and checksum-mode shape. */
    CASTLE_NODISCARD bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!is_valid_identifier(identifier)
            || protected_identifier != calculate_protected_identifier(identifier))
        {
            return false;
        }
        if (data_length == 0U || data_length > LIN_MAX_DATA_LENGTH)
        {
            return false;
        }
        if (identifier >= LIN_DIAGNOSTIC_REQUEST_IDENTIFIER
            && data_length != LIN_MAX_DATA_LENGTH)
        {
            return false;
        }
        if (checksum_mode != checksum_type::classic
            && checksum_mode != checksum_type::enhanced)
        {
            return false;
        }
        if (identifier >= LIN_DIAGNOSTIC_REQUEST_IDENTIFIER
            && checksum_mode != checksum_type::classic)
        {
            return false;
        }
        return true;
    }
};

/** @brief Creates a validated logical header; output is unchanged on failure. */
inline castle::status make_header(uint32_t identifier, header& output) CASTLE_NOEXCEPT
{
    if (!is_valid_identifier(identifier))
    {
        return castle::status::invalid_argument;
    }
    header candidate{};
    candidate.identifier = static_cast<uint8_t>(identifier);
    candidate.protected_identifier = calculate_protected_identifier(identifier);
    output = candidate;
    return castle::status::ok;
}

/** @brief Decodes a PID byte into a header; output is unchanged on failure. */
inline castle::status decode_header(uint8_t pid, header& output) CASTLE_NOEXCEPT
{
    if (!is_valid_protected_identifier(pid))
    {
        return castle::status::invalid_argument;
    }
    header candidate{};
    candidate.identifier = static_cast<uint8_t>(pid & 0x3FU);
    candidate.protected_identifier = pid;
    output = candidate;
    return castle::status::ok;
}

} // namespace lin
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_LIN_FRAME_HPP
