// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file esf_status.hpp
 * @brief UBX-ESF-STATUS (0x10 0x10): external sensor fusion status.
 *
 * Periodic/polled output: a 16-byte header followed by `numSens` 4-byte
 * per-sensor status blocks (`16 + numSens·4`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_STATUS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_STATUS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Sensor fusion filter mode (`fusionMode`). */
enum class esf_fusion_mode : uint8_t
{
    initialization = 0U, ///< Receiver is initializing values required for sensor fusion.
    fusion = 1U,         ///< GNSS and sensor data are used for the navigation solution.
    suspended = 2U,      ///< Sensor fusion temporarily disabled (e.g. invalid sensor data, ferry).
    disabled = 3U        ///< Sensor fusion permanently disabled until receiver reset.
};

/** @brief Sensor calibration status (`sensStatus2` bits 1..0). */
enum class esf_calib_status : uint8_t
{
    not_calibrated = 0U, ///< Sensor is not calibrated.
    calibrating = 1U,    ///< Sensor is calibrating.
    calibrated = 2U,     ///< Sensor is calibrated.
    calibrated_alt = 3U  ///< Sensor is calibrated (spec: `10/11` both mean calibrated).
};

/** @brief Measurement time-tag source (`sensStatus2` bits 3..2). */
enum class esf_time_status : uint8_t
{
    no_data = 0U,        ///< No data.
    first_byte = 1U,     ///< Reception of the first byte used to tag the measurement.
    event_input = 2U,    ///< Event input used to tag the measurement.
    time_tag = 3U        ///< Time tag provided with the data.
};

/** @brief One per-sensor status block of UBX-ESF-STATUS. */
struct esf_status_sensor
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 4U;

    /** @name `sensStatus1` bits @{ */
    static CASTLE_CONSTEXPR uint8_t SENS_STATUS1_TYPE = 0x3FU;
    static CASTLE_CONSTEXPR uint8_t SENS_STATUS1_USED = 0x40U;
    static CASTLE_CONSTEXPR uint8_t SENS_STATUS1_READY = 0x80U;
    /** @} */

    /** @name `sensStatus2` bits @{ */
    static CASTLE_CONSTEXPR uint8_t SENS_STATUS2_CALIB_STATUS = 0x03U;
    static CASTLE_CONSTEXPR uint8_t SENS_STATUS2_TIME_STATUS = 0x0CU;
    /** @} */

    /** @name `faults` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FAULTS_BAD_MEAS = 0x01U;
    static CASTLE_CONSTEXPR uint8_t FAULTS_BAD_TTAG = 0x02U;
    static CASTLE_CONSTEXPR uint8_t FAULTS_MISSING_MEAS = 0x04U;
    static CASTLE_CONSTEXPR uint8_t FAULTS_NOISY_MEAS = 0x08U;
    /** @} */

    uint8_t sens_status1 = 0U; ///< `SENS_STATUS1_*` bits.
    uint8_t sens_status2 = 0U; ///< `SENS_STATUS2_*` bits.
    uint8_t freq = 0U;         ///< Observation frequency [Hz].
    uint8_t faults = 0U;       ///< `FAULTS_*` bits.

    /** @brief Returns the sensor data type (`sensStatus1` bits 5..0). */
    CASTLE_NODISCARD uint8_t type() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(sens_status1 & SENS_STATUS1_TYPE);
    }

    /** @brief Reports whether the sensor data is used for the current fusion solution. */
    CASTLE_NODISCARD bool used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sens_status1 & SENS_STATUS1_USED) != 0U;
    }

    /** @brief Reports whether the sensor is set up but not used for the current solution. */
    CASTLE_NODISCARD bool ready() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sens_status1 & SENS_STATUS1_READY) != 0U;
    }

    CASTLE_NODISCARD esf_calib_status calib_status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<esf_calib_status>(sens_status2 & SENS_STATUS2_CALIB_STATUS);
    }

    /** @brief Reports a calibrated sensor (`calibStatus` 10 or 11). */
    CASTLE_NODISCARD bool calibrated() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sens_status2 & 0x02U) != 0U;
    }

    CASTLE_NODISCARD esf_time_status time_status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<esf_time_status>((sens_status2 & SENS_STATUS2_TIME_STATUS) >> 2U);
    }

    CASTLE_NODISCARD bool bad_meas() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (faults & FAULTS_BAD_MEAS) != 0U;
    }

    CASTLE_NODISCARD bool bad_ttag() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (faults & FAULTS_BAD_TTAG) != 0U;
    }

    CASTLE_NODISCARD bool missing_meas() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (faults & FAULTS_MISSING_MEAS) != 0U;
    }

    CASTLE_NODISCARD bool noisy_meas() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (faults & FAULTS_NOISY_MEAS) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.sens_status1);
        stream.field(self.sens_status2);
        stream.field(self.freq);
        stream.field(self.faults);
    }
};

/** @brief UBX-ESF-STATUS output payload. */
template <castle::size_type MaxSensors = 32U>
struct esf_status
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ESF;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_ESF_STATUS;
    static CASTLE_CONSTEXPR castle::size_type header_length = 16U;
    static CASTLE_CONSTEXPR castle::size_type block_length = esf_status_sensor::wire_size;

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U; ///< Message version (0x02 for this version).
    esf_fusion_mode fusion_mode = esf_fusion_mode::initialization;
    uint8_t num_sens = 0U; ///< On-wire sensor count; may exceed `sensors.size()`.
    castle::container::vector<esf_status_sensor, MaxSensors> sensors{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxSensors` sensor blocks.
     * @return `false` unless the payload length is exactly `16 + numSens·4`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, esf_status& out) CASTLE_NOEXCEPT
    {
        out = esf_status{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_sens, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_sens, block_length))
        {
            out = esf_status{};
            return false;
        }

        detail::read_group(reader, out.num_sens, out.sensors);
        return reader.ok();
    }

    /** @brief Writes the header and every stored sensor (`num_sens` is taken from `sensors.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t wire_count = static_cast<uint8_t>(sensors.size());
        layout_header(*this, wire_count, writer);
        detail::write_group(writer, sensors);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.reserved(7U);
        stream.field(self.fusion_mode);
        stream.reserved(2U);
        stream.field(count);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_STATUS_HPP
