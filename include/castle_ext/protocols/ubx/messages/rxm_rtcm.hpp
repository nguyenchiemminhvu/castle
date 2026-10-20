// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rxm_rtcm.hpp
 * @brief UBX-RXM-RTCM (0x02 0x32): RTCM input status.
 *
 * Output emitted once per successfully parsed RTCM input message, whether or
 * not the receiver supports that RTCM message.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RTCM_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RTCM_HPP

#include "castle/core/compiler.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief `msgUsed` (bits 2..1 of `flags`): whether the receiver used the RTCM message. */
enum class rxm_rtcm_msg_used : uint8_t
{
    unknown = 0U,  ///< Do not know.
    not_used = 1U, ///< Not used.
    used = 2U      ///< Used successfully by the receiver.
};

/** @brief UBX-RXM-RTCM output payload. */
struct rxm_rtcm : detail::fixed_message<rxm_rtcm, UBX_CLASS_RXM, UBX_ID_RXM_RTCM, 8U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_CRC_FAILED = 0x01U; ///< `crcFailed`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_MSG_USED = 0x06U;   ///< `msgUsed`.
    /** @} */

    /** @brief `refStation` value reported for RTCM messages without a DF003 field. */
    static CASTLE_CONSTEXPR uint16_t REF_STATION_NOT_REPORTED = 0xFFFFU;

    uint8_t version = 0U;      ///< Message version (0x02 for this version).
    uint8_t flags = 0U;        ///< Status flags, see `FLAGS_*`.
    uint16_t sub_type = 0U;    ///< Subtype of u-blox proprietary RTCM message 4072.
    uint16_t ref_station = 0U; ///< Reference station ID, or `REF_STATION_NOT_REPORTED`.
    uint16_t msg_type = 0U;    ///< RTCM message type.

    /** @brief `true` when the RTCM CRC failed (`ref_station` / `msg_type` may then be corrupt). */
    CASTLE_NODISCARD bool crc_failed() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_CRC_FAILED) != 0U;
    }

    CASTLE_NODISCARD rxm_rtcm_msg_used msg_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<rxm_rtcm_msg_used>((flags & FLAGS_MSG_USED) >> 1U);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(self.flags);
        stream.field(self.sub_type);
        stream.field(self.ref_station);
        stream.field(self.msg_type);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RTCM_HPP
