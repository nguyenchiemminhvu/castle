// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_eoe.hpp
 * @brief UBX-NAV-EOE (0x01 0x61): end-of-epoch marker.
 *
 * Periodic output sent after all enabled NAV class (and NMEA) messages of a
 * navigation epoch, so a host can collect one epoch's messages as a unit.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_EOE_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_EOE_HPP

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

/** @brief UBX-NAV-EOE output payload. */
struct nav_eoe : detail::fixed_message<nav_eoe, UBX_CLASS_NAV, UBX_ID_NAV_EOE, 4U>
{
    uint32_t i_tow = 0U; ///< GPS time of week of the finished epoch [ms].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_EOE_HPP
