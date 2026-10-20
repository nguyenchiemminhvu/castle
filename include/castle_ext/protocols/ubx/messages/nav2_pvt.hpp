// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav2_pvt.hpp
 * @brief UBX-NAV2-PVT (0x29 0x07): secondary-output navigation position velocity time solution.
 *
 * Same 92-byte payload as UBX-NAV-PVT; see `basic_nav_pvt` in nav_pvt.hpp.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_PVT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_PVT_HPP

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-NAV2-PVT (0x29 0x07). */
using nav2_pvt = basic_nav_pvt<UBX_CLASS_NAV2, UBX_ID_NAV2_PVT>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV2_PVT_HPP
