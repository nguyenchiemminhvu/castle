// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rxm_rawx.hpp
 * @brief UBX-RXM-RAWX (0x02 0x15): multi-GNSS raw measurements.
 *
 * Periodic/polled output: a 16-byte header followed by `numMeas` 32-byte
 * measurement blocks (`16 + numMeas·32`). Carries pseudorange, carrier phase,
 * Doppler and signal-quality data for RINEX 3 observation generation.
 * `rcvTow`, `prMes`, `cpMes` (R8) and `doMes` (R4) are IEEE-754 values on the
 * wire and are stored as `double` / `float`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RAWX_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RAWX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
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

/** @brief One measurement block of UBX-RXM-RAWX. */
struct rxm_rawx_meas
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 32U;

    /** @name `prStdev` / `cpStdev` / `doStdev` bits @{ */
    static CASTLE_CONSTEXPR uint8_t STDEV_INDEX = 0x0FU; ///< `prStd` / `cpStd` / `doStd`.
    /** @} */

    /** @brief `cpStd` value marking the carrier phase standard deviation as invalid. */
    static CASTLE_CONSTEXPR uint8_t CP_STD_INVALID = 0x0FU;

    /** @name `trkStat` bits @{ */
    static CASTLE_CONSTEXPR uint8_t TRK_STAT_PR_VALID = 0x01U;      ///< `prValid`.
    static CASTLE_CONSTEXPR uint8_t TRK_STAT_CP_VALID = 0x02U;      ///< `cpValid`.
    static CASTLE_CONSTEXPR uint8_t TRK_STAT_HALF_CYC = 0x04U;      ///< `halfCyc`: half cycle valid.
    static CASTLE_CONSTEXPR uint8_t TRK_STAT_SUB_HALF_CYC = 0x08U;  ///< `subHalfCyc`: half cycle subtracted.
    /** @} */

    double pr_mes = 0.0;    ///< Pseudorange measurement [m].
    double cp_mes = 0.0;    ///< Carrier phase measurement [cycles].
    float do_mes = 0.0F;    ///< Doppler measurement, positive for approaching satellites [Hz].
    uint8_t gnss_id = 0U;   ///< GNSS identifier (see Satellite Numbering).
    uint8_t sv_id = 0U;     ///< Satellite identifier (see Satellite Numbering).
    uint8_t sig_id = 0U;    ///< Signal identifier (see Signal Identifiers).
    uint8_t freq_id = 0U;   ///< GLONASS frequency slot + 7 (0..13).
    uint16_t locktime = 0U; ///< Carrier phase locktime counter, at most 64500 [ms].
    uint8_t cno = 0U;       ///< Carrier-to-noise density ratio [dBHz].
    uint8_t pr_stdev = 0U;  ///< Pseudorange standard deviation index, `0.01·2^n` [m] (bits 3..0).
    uint8_t cp_stdev = 0U;  ///< Carrier phase standard deviation, `0.004·n` [cycles] (bits 3..0).
    uint8_t do_stdev = 0U;  ///< Doppler standard deviation index, `0.002·2^n` [Hz] (bits 3..0).
    uint8_t trk_stat = 0U;  ///< Tracking status, see `TRK_STAT_*`.

    CASTLE_NODISCARD uint8_t pr_std() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(pr_stdev & STDEV_INDEX);
    }

    CASTLE_NODISCARD uint8_t cp_std() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(cp_stdev & STDEV_INDEX);
    }

    CASTLE_NODISCARD uint8_t do_std() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(do_stdev & STDEV_INDEX);
    }

    /** @brief Estimated pseudorange standard deviation [m] (`0.01·2^prStd`). */
    CASTLE_NODISCARD double pr_std_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(1U << pr_std()) * castle::math::pow10< -2, double>();
    }

    /** @brief Estimated carrier phase standard deviation [cycles] (`0.004·cpStd`). */
    CASTLE_NODISCARD double cp_std_cycles() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(cp_std()) * 4.0 * castle::math::pow10< -3, double>();
    }

    /** @brief `false` when `cpStd` holds the invalid marker 0x0F. */
    CASTLE_NODISCARD bool cp_std_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return cp_std() != CP_STD_INVALID;
    }

    /** @brief Estimated Doppler standard deviation [Hz] (`0.002·2^doStd`). */
    CASTLE_NODISCARD double do_std_hz() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(1U << do_std()) * 2.0 * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD bool pr_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (trk_stat & TRK_STAT_PR_VALID) != 0U;
    }

    CASTLE_NODISCARD bool cp_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (trk_stat & TRK_STAT_CP_VALID) != 0U;
    }

    CASTLE_NODISCARD bool half_cyc() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (trk_stat & TRK_STAT_HALF_CYC) != 0U;
    }

    CASTLE_NODISCARD bool sub_half_cyc() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (trk_stat & TRK_STAT_SUB_HALF_CYC) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.pr_mes);
        stream.field(self.cp_mes);
        stream.field(self.do_mes);
        stream.field(self.gnss_id);
        stream.field(self.sv_id);
        stream.field(self.sig_id);
        stream.field(self.freq_id);
        stream.field(self.locktime);
        stream.field(self.cno);
        stream.field(self.pr_stdev);
        stream.field(self.cp_stdev);
        stream.field(self.do_stdev);
        stream.field(self.trk_stat);
        stream.reserved(1U);
    }
};

/** @brief UBX-RXM-RAWX output payload. */
template <castle::size_type MaxMeas = 64U>
struct rxm_rawx
{
    static_assert(MaxMeas <= 255U, "numMeas is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_RXM;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_RXM_RAWX;
    static CASTLE_CONSTEXPR castle::size_type header_length = 16U;
    static CASTLE_CONSTEXPR castle::size_type block_length = rxm_rawx_meas::wire_size;

    /** @name `recStat` bits @{ */
    static CASTLE_CONSTEXPR uint8_t REC_STAT_LEAP_SEC = 0x01U;  ///< `leapSec`: leap seconds determined.
    static CASTLE_CONSTEXPR uint8_t REC_STAT_CLK_RESET = 0x02U; ///< `clkReset`: clock reset applied.
    /** @} */

    double rcv_tow = 0.0;   ///< Measurement time of week in receiver local time [s].
    uint16_t week = 0U;     ///< GPS week number in receiver local time [weeks].
    int8_t leap_s = 0;      ///< GPS leap seconds (GPS-UTC) [s].
    uint8_t num_meas = 0U;  ///< On-wire measurement count; may exceed `meas.size()`.
    uint8_t rec_stat = 0U;  ///< Receiver tracking status, see `REC_STAT_*`.
    uint8_t version = 0U;   ///< Message version (0x01 for this version).
    castle::container::vector<rxm_rawx_meas, MaxMeas> meas{};

    CASTLE_NODISCARD bool leap_sec() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (rec_stat & REC_STAT_LEAP_SEC) != 0U;
    }

    CASTLE_NODISCARD bool clk_reset() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (rec_stat & REC_STAT_CLK_RESET) != 0U;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxMeas` measurement blocks.
     * @return `false` unless the payload length is exactly `16 + numMeas·32`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, rxm_rawx& out) CASTLE_NOEXCEPT
    {
        out = rxm_rawx{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_meas, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_meas, block_length))
        {
            out = rxm_rawx{};
            return false;
        }

        detail::read_group(reader, out.num_meas, out.meas);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`numMeas` is taken from `meas.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(meas.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, meas);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.rcv_tow);
        stream.field(self.week);
        stream.field(self.leap_s);
        stream.field(count);
        stream.field(self.rec_stat);
        stream.field(self.version);
        stream.reserved(2U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RAWX_HPP
