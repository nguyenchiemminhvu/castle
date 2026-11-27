// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav2_timegps.hpp
 * @brief UBX-NAV2-TIMEGPS (0x29 0x20): secondary-output GPS time solution.
 *
 * Periodic/polled output of the secondary navigation output. Its payload is
 * identical to UBX-NAV-TIMEGPS, so the type is an alias of
 * `basic_nav_timegps` bound to the NAV2 class.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_TIMEGPS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_TIMEGPS_HPP

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV2-TIMEGPS. */
using nav2_timegps = basic_nav_timegps<UBX_CLASS_NAV2, UBX_ID_NAV2_TIMEGPS>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_TIMEGPS_HPP
