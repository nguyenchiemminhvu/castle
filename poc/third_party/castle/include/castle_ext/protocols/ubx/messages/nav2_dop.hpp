// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav2_dop.hpp
 * @brief UBX-NAV2-DOP (0x29 0x04): secondary-output dilution of precision.
 *
 * Same 18-byte payload as UBX-NAV-DOP; see `basic_nav_dop` in nav_dop.hpp.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_DOP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_DOP_HPP

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV2-DOP (0x29 0x04). */
using nav2_dop = basic_nav_dop<UBX_CLASS_NAV2, UBX_ID_NAV2_DOP>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_DOP_HPP
