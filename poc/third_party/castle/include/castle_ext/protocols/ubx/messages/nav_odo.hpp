// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_odo.hpp
 * @brief UBX-NAV-ODO (0x01 0x09): odometer solution.
 *
 * Periodic/polled output of the ground distance travelled since the last
 * odometer reset, the accumulated total and its 1-sigma accuracy.
 *
 * @note UBX-NAV-ODO is not listed in the ZED-F9R interface description
 * (UBX-19056845 R01); the layout follows the generic u-blox protocol
 * definition (version U1, reserved U1[3], iTOW U4, distance U4,
 * totalDistance U4, distanceStd U4).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ODO_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ODO_HPP

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

/** @brief UBX-NAV-ODO output payload. */
struct nav_odo : detail::fixed_message<nav_odo, UBX_CLASS_NAV, UBX_ID_NAV_ODO, 20U>
{
    uint8_t version = 0U;          ///< Message version (0x00 for this version).
    uint32_t i_tow = 0U;           ///< GPS time of week [ms].
    uint32_t distance = 0U;        ///< Ground distance since last reset [m].
    uint32_t total_distance = 0U;  ///< Total cumulative ground distance [m].
    uint32_t distance_std = 0U;    ///< Ground distance accuracy, 1-sigma [m].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.i_tow);
        stream.field(self.distance);
        stream.field(self.total_distance);
        stream.field(self.distance_std);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ODO_HPP
