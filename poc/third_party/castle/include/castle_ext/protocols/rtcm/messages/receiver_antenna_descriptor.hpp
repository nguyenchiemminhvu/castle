// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file receiver_antenna_descriptor.hpp
 * @brief RTCM 3 message 1033: receiver and antenna descriptors.
 *
 * Extends message 1007 with the antenna serial number and the receiver type,
 * firmware version and serial number of the reference station.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_RECEIVER_ANTENNA_DESCRIPTOR_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_RECEIVER_ANTENNA_DESCRIPTOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"
#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{

/** @brief RTCM message 1033. */
struct receiver_antenna_descriptor
{
    /** @brief Message number (DF002). */
    static CASTLE_CONSTEXPR uint16_t MESSAGE_TYPE = RTCM_TYPE_RECEIVER_ANTENNA;

    uint16_t station_id = 0U;           ///< DF003: reference station ID (12 bit).
    descriptor_string antenna{};        ///< DF029/DF030: antenna descriptor.
    uint8_t antenna_setup_id = 0U;      ///< DF031: antenna setup ID.
    descriptor_string antenna_serial{}; ///< DF032/DF033: antenna serial number.
    descriptor_string receiver_type{};  ///< DF227/DF228: receiver type descriptor.
    descriptor_string firmware{};       ///< DF229/DF230: receiver firmware version.
    descriptor_string receiver_serial{};///< DF231/DF232: receiver serial number.

    /** @brief Reports whether `type` is 1033. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR bool accepts(uint16_t type) CASTLE_NOEXCEPT
    {
        return type == MESSAGE_TYPE;
    }

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.text(antenna);
        writer.field(antenna_setup_id, 8U);
        writer.text(antenna_serial);
        writer.text(receiver_type);
        writer.text(firmware);
        writer.text(receiver_serial);
    }
};

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_RECEIVER_ANTENNA_DESCRIPTOR_HPP
