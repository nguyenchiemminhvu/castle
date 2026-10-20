// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_qzss.hpp
 * @brief UBX-MGA-QZSS (0x13 0x05): QZSS assistance data (ephemeris, almanac, health).
 *
 * Input messages sent by the host. The ephemeris and almanac layouts are
 * bit-for-bit the GPS ones, so they reuse `basic_mga_gps_eph` /
 * `basic_mga_gps_alm` from `mga_gps.hpp` instantiated on `UBX_ID_MGA_QZSS`.
 * Only the health layout (5 SVs instead of 32) is QZSS-specific. The `type`
 * and version bytes are the shared `MGA_GPS_TYPE_*` / `MGA_GPS_VERSION`
 * constants.
 *
 * @code
 * castle::protocols::ubx::messages::mga_qzss_health health{};
 * health.health_code[0U] = 0x01U;
 * uint8_t frame[24U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(health, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_QZSS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_QZSS_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/typed_message.hpp"
#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-MGA-QZSS-EPH (same layout as UBX-MGA-GPS-EPH). */
using mga_qzss_eph = basic_mga_gps_eph<UBX_ID_MGA_QZSS>;

/** @brief UBX-MGA-QZSS-ALM (same layout as UBX-MGA-GPS-ALM). */
using mga_qzss_alm = basic_mga_gps_alm<UBX_ID_MGA_QZSS>;

/** @brief UBX-MGA-QZSS-HEALTH: health codes for QZSS SVs 1-5 (12 bytes). */
struct mga_qzss_health
    : detail::typed_message<mga_qzss_health, UBX_CLASS_MGA, UBX_ID_MGA_QZSS, MGA_GPS_TYPE_HEALTH, 12U>
{
    static CASTLE_CONSTEXPR castle::size_type NUM_SVS = 5U;

    /** @brief The 6 LSBs of each `health_code` entry carry the health code. */
    static CASTLE_CONSTEXPR uint8_t HEALTH_CODE_MASK = 0x3FU;

    /** @brief `health_code[i]` is the health of QZSS SV `i + 1`. */
    castle::container::array<uint8_t, NUM_SVS> health_code{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(MGA_GPS_TYPE_HEALTH);
        stream.constant(MGA_GPS_VERSION);
        stream.reserved(2U);
        stream.field(self.health_code);
        stream.reserved(3U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_QZSS_HPP
