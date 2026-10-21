// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file antenna_descriptor.hpp
 * @brief RTCM 3 message 1007: antenna descriptor.
 *
 * Names the antenna model of the reference station (IGS/NGS antenna type, for
 * example `"TRM59800.00     NONE"`) and its setup ID.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_ANTENNA_DESCRIPTOR_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_ANTENNA_DESCRIPTOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string.hpp"
#include "castle/error/status.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{

/** @brief Longest descriptor string RTCM allows in messages 1007 and 1033. */
static CASTLE_CONSTEXPR castle::size_type RTCM_MAX_DESCRIPTOR_LEN = 31U;

/** @brief Descriptor string of messages 1007 and 1033. */
using descriptor_string = castle::container::string<RTCM_MAX_DESCRIPTOR_LEN>;

/** @brief RTCM message 1007. */
struct antenna_descriptor
{
    /** @brief Message number (DF002). */
    static CASTLE_CONSTEXPR uint16_t MESSAGE_TYPE = RTCM_TYPE_ANTENNA_DESCRIPTOR;

    uint16_t station_id = 0U;       ///< DF003: reference station ID (12 bit).
    descriptor_string descriptor{}; ///< DF029/DF030: antenna descriptor, at most 31 characters.
    uint8_t setup_id = 0U;          ///< DF031: antenna setup ID.

    /** @brief Reports whether `type` is 1007. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR bool accepts(uint16_t type) CASTLE_NOEXCEPT
    {
        return type == MESSAGE_TYPE;
    }

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.text(descriptor);
        writer.field(setup_id, 8U);
    }
};

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_ANTENNA_DESCRIPTOR_HPP
