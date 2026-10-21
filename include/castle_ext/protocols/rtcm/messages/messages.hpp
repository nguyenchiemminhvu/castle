// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file messages.hpp
 * @brief Umbrella header for every RTCM 3.3 message model shipped with Castle.
 *
 * Including this header makes all RTCM input message builders available. For a
 * smaller build, include only the message headers you need.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_MESSAGES_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_MESSAGES_HPP

// Legacy RTK observables (1001-1004, 1009-1012)
#include "castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp"
#include "castle_ext/protocols/rtcm/messages/gps_rtk_observables.hpp"

// Station information (1005, 1006, 1007, 1033, 1230)
#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"
#include "castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp"
#include "castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp"
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

// Multiple Signal Messages (1074-1127)
#include "castle_ext/protocols/rtcm/messages/msm.hpp"

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_MESSAGES_HPP
