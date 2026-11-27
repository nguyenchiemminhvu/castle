#pragma once

#include <stdint.h>
#include "castle/core/status.hpp"
#include "castle/dsa/string.hpp"
#include "castle_ext/data/data_pool.hpp"
#include "castle_ext/data/data_slot.hpp"

namespace gnss
{

enum class LocationSource : uint8_t
{
    ubx_pvt = 0U,
    nmea_gga = 1U,
    nmea_rmc = 2U
};

struct LocationData
{
    double latitude_deg = 0.0;       // Consumer coordinate: GCJ-02 in China, WGS-84 elsewhere.
    double longitude_deg = 0.0;
    double wgs84_latitude_deg = 0.0;
    double wgs84_longitude_deg = 0.0;
    double altitude_m = 0.0;
    uint32_t horizontal_accuracy_mm = 0U;
    uint8_t fix_type = 0U;
    uint8_t satellites_used = 0U;
    LocationSource source = LocationSource::ubx_pvt;
    bool valid = false;
    bool shifted_to_gcj02 = false;
};

struct TimeData
{
    uint16_t year = 0U;
    uint8_t month = 0U;
    uint8_t day = 0U;
    uint8_t hour = 0U;
    uint8_t minute = 0U;
    uint8_t second = 0U;
    int32_t nanoseconds = 0;
    bool valid = false;
};

struct SatelliteData
{
    uint8_t used = 0U;
    uint16_t position_dop_x100 = 0U;
};

// Separate heartbeat avoids a later GGA latest-value update masking a valid
// PVT notification before the HAL health observer consumes its inbox.
struct PvtHeartbeat
{
    uint32_t sequence = 0U;
};

struct HalStatus
{
    bool uart_open = false;
    bool no_bytes_fault = false;
    bool no_valid_pvt_fault = false;
    uint8_t recovery_level = 0U;
    uint32_t rx_bytes = 0U;
    castle::status last_error = castle::status::ok;
};

struct Waypoint
{
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    uint32_t monotonic_ms = 0U;
};

using Pool = castle_ext::data::data_pool<8U,
    castle_ext::data::data_slot<LocationData, 4U>,
    castle_ext::data::data_slot<TimeData, 4U>,
    castle_ext::data::data_slot<SatelliteData, 4U>,
    castle_ext::data::data_slot<PvtHeartbeat, 4U>,
    castle_ext::data::data_slot<HalStatus, 4U>>;

struct GnssTransConfig
{
    castle::dsa::string<128U> uart_path{ "/dev/ttyS1" };
    castle::dsa::string<128U> config_path{ "/etc/ubx_config.cfg" };
    uint32_t baud_rate = 2000000U;
    bool simulation = false;

    castle::status init() noexcept
    {
        return castle::status::ok;
    }
    castle::status deinit() noexcept
    {
        return castle::status::ok;
    }
    castle::status suspend() noexcept
    {
        return castle::status::ok;
    }
    castle::status resume() noexcept
    {
        return castle::status::ok;
    }
    castle::status shutdown() noexcept
    {
        return castle::status::ok;
    }
};

struct DataStore
{
    Pool pool{};
    Pool& get() noexcept
    {
        return pool;
    }
    castle::status init() noexcept
    {
        return castle::status::ok;
    }
    castle::status deinit() noexcept
    {
        return castle::status::ok;
    }
    castle::status suspend() noexcept
    {
        return castle::status::ok;
    }
    castle::status resume() noexcept
    {
        return castle::status::ok;
    }
    castle::status shutdown() noexcept
    {
        return castle::status::ok;
    }
};

} // namespace gnss
