// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file checksum.hpp
 * @brief LIN classic/enhanced checksum calculation and frame construction.
 */
#ifndef CASTLE_EXT_PROTOCOLS_LIN_CHECKSUM_HPP
#define CASTLE_EXT_PROTOCOLS_LIN_CHECKSUM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array_view.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/lin/frame.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace lin
{

/** @brief Resolves the mandated classic checksum for diagnostic frame IDs. */
CASTLE_CONSTEXPR inline checksum_type effective_checksum_type(
    uint32_t identifier, checksum_type requested) CASTLE_NOEXCEPT
{
    return identifier == LIN_DIAGNOSTIC_REQUEST_IDENTIFIER
        || identifier == LIN_DIAGNOSTIC_RESPONSE_IDENTIFIER
        ? checksum_type::classic : requested;
}

/**
 * @brief Calculates the LIN checksum for a response payload.
 * @param identifier Six-bit LIN frame identifier.
 * @param data Payload bytes (must not be null for non-empty payloads).
 * @param length Payload length (1..8; diagnostic IDs require 8 bytes).
 * @param type Requested checksum model; diagnostic IDs always use classic.
 * @param output Receives the checksum on success and is unchanged on failure.
 * @return `ok`, or `invalid_argument` for malformed input.
 *
 * The carry is folded back into the low byte after each addition, then the
 * resulting eight-bit sum is inverted. Complexity is O(length), with O(1)
 * auxiliary storage.
 */
inline castle::status calculate_checksum(
    uint32_t identifier,
    CASTLE_CONST uint8_t* data,
    castle::size_type length,
    checksum_type type,
    uint8_t& output) CASTLE_NOEXCEPT
{
    if (!is_valid_identifier(identifier)
        || (length != 0U && data == nullptr)
        || length == 0U
        || length > LIN_MAX_DATA_LENGTH
        || ((identifier == LIN_DIAGNOSTIC_REQUEST_IDENTIFIER
             || identifier == LIN_DIAGNOSTIC_RESPONSE_IDENTIFIER)
            && length != LIN_MAX_DATA_LENGTH)
        || (type != checksum_type::classic && type != checksum_type::enhanced))
    {
        return castle::status::invalid_argument;
    }

    const checksum_type mode = effective_checksum_type(identifier, type);
    uint16_t sum = 0U;
    if (mode == checksum_type::enhanced)
    {
        sum = calculate_protected_identifier(identifier);
    }

    for (castle::size_type index = 0U; index < length; ++index)
    {
        sum = static_cast<uint16_t>(sum + data[index]);
        if (sum > 0xFFU)
        {
            sum = static_cast<uint16_t>((sum & 0xFFU) + 1U);
        }
    }

    output = static_cast<uint8_t>(~static_cast<uint8_t>(sum));
    return castle::status::ok;
}

/** @brief Calculates a checksum from a non-owning Castle byte view. */
inline castle::status calculate_checksum(
    uint32_t identifier,
    castle::container::array_view<CASTLE_CONST uint8_t> data,
    checksum_type type,
    uint8_t& output) CASTLE_NOEXCEPT
{
    return calculate_checksum(identifier, data.data(), data.size(), type, output);
}

/** @brief Calculates a frame's checksum after validating its structural fields. */
inline castle::status calculate_checksum(CASTLE_CONST frame& value, uint8_t& output) CASTLE_NOEXCEPT
{
    if (!value.valid())
    {
        return castle::status::invalid_argument;
    }
    return calculate_checksum(value.identifier, value.data, value.data_length,
        value.checksum_mode, output);
}

/** @brief Returns true only when the frame structure and checksum are valid. */
inline bool validate_checksum(CASTLE_CONST frame& value) CASTLE_NOEXCEPT
{
    if (!value.valid())
    {
        return false;
    }
    uint8_t expected = 0U;
    return castle::succeeded(calculate_checksum(value, expected))
        && expected == value.checksum;
}

/**
 * @brief Constructs a complete response frame, copying its payload inline.
 * @param identifier LIN frame identifier (0..61; 62 and 63 are reserved).
 * @param data Payload source, which may not be null.
 * @param length Payload length 1..8; diagnostic frames require exactly 8.
 * @param type Requested checksum model; diagnostic identifiers are forced to classic.
 * @param output Destination modified only when construction succeeds.
 */
inline castle::status make_frame(
    uint32_t identifier,
    CASTLE_CONST uint8_t* data,
    castle::size_type length,
    checksum_type type,
    frame& output) CASTLE_NOEXCEPT
{
    if (!is_valid_identifier(identifier)
        || data == nullptr
        || length == 0U
        || length > LIN_MAX_DATA_LENGTH
        || ((identifier == LIN_DIAGNOSTIC_REQUEST_IDENTIFIER
             || identifier == LIN_DIAGNOSTIC_RESPONSE_IDENTIFIER)
            && length != LIN_MAX_DATA_LENGTH)
        || (type != checksum_type::classic && type != checksum_type::enhanced))
    {
        return castle::status::invalid_argument;
    }

    frame candidate{};
    candidate.identifier = static_cast<uint8_t>(identifier);
    candidate.protected_identifier = calculate_protected_identifier(identifier);
    candidate.data_length = static_cast<uint8_t>(length);
    candidate.checksum_mode = effective_checksum_type(identifier, type);
    for (castle::size_type index = 0U; index < length; ++index)
    {
        candidate.data[index] = data[index];
    }

    const castle::status result = calculate_checksum(identifier, candidate.data,
        length, candidate.checksum_mode, candidate.checksum);
    if (!castle::succeeded(result) || !candidate.valid())
    {
        return castle::status::invalid_argument;
    }
    output = candidate;
    return castle::status::ok;
}

/** @brief Constructs a response frame from a non-owning Castle byte view. */
inline castle::status make_frame(
    uint32_t identifier,
    castle::container::array_view<CASTLE_CONST uint8_t> data,
    checksum_type type,
    frame& output) CASTLE_NOEXCEPT
{
    return make_frame(identifier, data.data(), data.size(), type, output);
}

} // namespace lin
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_LIN_CHECKSUM_HPP
