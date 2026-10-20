// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav2_eell.hpp
 * @brief UBX-NAV2-EELL (0x29 0x3D): secondary-output position error ellipse parameters.
 *
 * Same 16-byte payload as UBX-NAV-EELL; see `basic_nav_eell` in nav_eell.hpp.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_EELL_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_EELL_HPP

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV2-EELL (0x29 0x3D). */
using nav2_eell = basic_nav_eell<UBX_CLASS_NAV2, UBX_ID_NAV2_EELL>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_EELL_HPP
