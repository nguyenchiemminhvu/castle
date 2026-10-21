// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file station_arp.hpp
 * @brief RTCM 3 messages 1005 and 1006: stationary RTK reference station ARP.
 *
 * Both messages carry the antenna reference point (ARP) of the base station in
 * ECEF coordinates (ITRF) plus the satellite systems it serves. Message 1006
 * appends the antenna height above the marker. This is the position a rover
 * (for example a u-blox ZED-F9R) needs to apply the observation corrections.
 * Each message number is its own type: `station_arp` (1005) and
 * `station_arp_height` (1006).
 *
 * Coordinates are kept as the raw 0.0001 m integers sent on the wire;
 * `set_position_m()` / `x_m()` convert from and to metres.
 *
 * @code
 * castle::protocols::rtcm::messages::station_arp_height station{};
 * station.station_id = 7U;
 * station.gps = true;
 * station.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
 * station.set_antenna_height_m(1.5);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_STATION_ARP_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_STATION_ARP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/math/round.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{

/**
 * @brief RTCM message 1005 (ARP) or 1006 (ARP with antenna height).
 *
 * @tparam Type `RTCM_TYPE_STATION_ARP` or `RTCM_TYPE_STATION_ARP_HEIGHT`.
 */
template <uint16_t Type>
struct station_arp_message
{
    static_assert((Type == RTCM_TYPE_STATION_ARP) || (Type == RTCM_TYPE_STATION_ARP_HEIGHT),
                  "message type must be 1005 or 1006");

    /** @brief Message number (DF002). */
    static CASTLE_CONSTEXPR uint16_t MESSAGE_TYPE = Type;
    /** @brief The message carries the antenna height (1006). */
    static CASTLE_CONSTEXPR bool HAS_ANTENNA_HEIGHT = (Type == RTCM_TYPE_STATION_ARP_HEIGHT);

    /** @brief Resolution of the ECEF coordinates and of the antenna height, in metres. */
    static CASTLE_CONSTEXPR double RESOLUTION_M = 0.0001;

    uint16_t station_id = 0U;                      ///< DF003: reference station ID (12 bit).
    uint8_t itrf_year = 0U;                        ///< DF021 (6 bit): ITRF realization year.
    bool gps = false;                              ///< DF022: GPS service supported.
    bool glonass = false;                          ///< DF023: GLONASS service supported.
    bool galileo = false;                          ///< DF024: Galileo service supported.
    bool reference_station_indicator = false;      ///< DF141: false = physical station, true = non-physical (network) station.
    int64_t ecef_x = 0;                            ///< DF025 (38 bit signed): ARP ECEF X, 0.0001 m.
    bool single_receiver_oscillator = false;       ///< DF142: all observations use one oscillator.
    int64_t ecef_y = 0;                            ///< DF026 (38 bit signed): ARP ECEF Y, 0.0001 m.
    uint8_t quarter_cycle_indicator = 0U;          ///< DF364 (2 bit): 0 = unspecified, 1 = correct, 2 = correct for L1 C/A, 3 = reserved.
    int64_t ecef_z = 0;                            ///< DF027 (38 bit signed): ARP ECEF Z, 0.0001 m.
    uint16_t antenna_height = 0U;                  ///< DF028 (1006 only, 16 bit): antenna height, 0.0001 m.

    /** @brief Reports whether `type` is this message's number. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR bool accepts(uint16_t type) CASTLE_NOEXCEPT
    {
        return type == MESSAGE_TYPE;
    }

    /** @brief Stores the ARP position given in metres, rounded to 0.0001 m. */
    void set_position_m(double x, double y, double z) CASTLE_NOEXCEPT
    {
        ecef_x = to_units(x);
        ecef_y = to_units(y);
        ecef_z = to_units(z);
    }

    /** @brief Stores the antenna height given in metres (1006 only), rounded to 0.0001 m and clamped to 0..6.5535 m. */
    void set_antenna_height_m(double height) CASTLE_NOEXCEPT
    {
        static_assert(HAS_ANTENNA_HEIGHT, "message 1005 carries no antenna height");
        CASTLE_CONST int64_t units = to_units(height);
        antenna_height = static_cast<uint16_t>((units < 0) ? 0 : ((units > 0xFFFF) ? 0xFFFF : units));
    }

    CASTLE_NODISCARD double x_m() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<double>(ecef_x) * RESOLUTION_M; }
    CASTLE_NODISCARD double y_m() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<double>(ecef_y) * RESOLUTION_M; }
    CASTLE_NODISCARD double z_m() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<double>(ecef_z) * RESOLUTION_M; }
    CASTLE_NODISCARD double antenna_height_m() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<double>(antenna_height) * RESOLUTION_M; }

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.field(itrf_year, 6U);
        writer.field(gps);
        writer.field(glonass);
        writer.field(galileo);
        writer.field(reference_station_indicator);
        writer.field(ecef_x, 38U);
        writer.field(single_receiver_oscillator);
        writer.reserved(1U);
        writer.field(ecef_y, 38U);
        writer.field(quarter_cycle_indicator, 2U);
        writer.field(ecef_z, 38U);
        if (HAS_ANTENNA_HEIGHT)
        {
            writer.field(antenna_height, 16U);
        }
    }

private:
    /** @brief Converts metres to 0.0001 m units, rounding to nearest and saturating far outside the 38-bit range; NaN becomes 0. */
    static int64_t to_units(double meters) CASTLE_NOEXCEPT
    {
        CASTLE_CONSTEXPR double limit = 1.0e15; // Far beyond 2^37 units; the writer then reports out_of_range.
        double scaled = meters / RESOLUTION_M;
        if (!(scaled == scaled)) // NaN would make the integer conversion undefined.
        {
            return 0;
        }
        scaled = (scaled > limit) ? limit : ((scaled < -limit) ? -limit : scaled);
        return castle::math::round_half_away<int64_t>(scaled);
    }
};

/** @name Message 1005/1006 aliases @{ */
using station_arp = station_arp_message<RTCM_TYPE_STATION_ARP>;
using station_arp_height = station_arp_message<RTCM_TYPE_STATION_ARP_HEIGHT>;
/** @} */

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_STATION_ARP_HPP
