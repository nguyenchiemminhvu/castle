// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cfg_rst.hpp
 * @brief UBX-CFG-RST (0x06 0x04): reset receiver / clear backup data structures.
 *
 * Command sent by the host. The receiver does not acknowledge it (newer
 * firmware never does; older firmware may be reset before the ACK is sent).
 *
 * @code
 * castle::protocols::ubx::messages::cfg_rst reset = castle::protocols::ubx::messages::cfg_rst::cold_start();
 * uint8_t frame[16U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(reset, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_RST_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_RST_HPP

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

/** @brief Reset type carried in UBX-CFG-RST `resetMode`. */
enum class cfg_rst_mode : uint8_t
{
    hardware_reset = 0x00U,            ///< Hardware reset (watchdog) immediately.
    software_reset = 0x01U,            ///< Controlled software reset.
    software_reset_gnss_only = 0x02U,  ///< Controlled software reset (GNSS only).
    hardware_reset_after_shutdown = 0x04U, ///< Hardware reset (watchdog) after shutdown.
    gnss_stop = 0x08U,                 ///< Controlled GNSS stop.
    gnss_start = 0x09U                 ///< Controlled GNSS start.
};

/** @brief UBX-CFG-RST command payload. */
struct cfg_rst : detail::fixed_message<cfg_rst, UBX_CLASS_CFG, UBX_ID_CFG_RST, 4U>
{
    /** @name `navBbrMask` bits (BBR sections to clear) @{ */
    static CASTLE_CONSTEXPR uint16_t BBR_EPH = 0x0001U;
    static CASTLE_CONSTEXPR uint16_t BBR_ALM = 0x0002U;
    static CASTLE_CONSTEXPR uint16_t BBR_HEALTH = 0x0004U;
    static CASTLE_CONSTEXPR uint16_t BBR_KLOB = 0x0008U;
    static CASTLE_CONSTEXPR uint16_t BBR_POS = 0x0010U;
    static CASTLE_CONSTEXPR uint16_t BBR_CLKD = 0x0020U;
    static CASTLE_CONSTEXPR uint16_t BBR_OSC = 0x0040U;
    static CASTLE_CONSTEXPR uint16_t BBR_UTC = 0x0080U;
    static CASTLE_CONSTEXPR uint16_t BBR_RTC = 0x0100U;
    static CASTLE_CONSTEXPR uint16_t BBR_AOP = 0x8000U;
    /** @} */

    /** @name `navBbrMask` presets @{ */
    static CASTLE_CONSTEXPR uint16_t BBR_HOT_START = 0x0000U;  ///< Keep all BBR data.
    static CASTLE_CONSTEXPR uint16_t BBR_WARM_START = 0x0001U; ///< Clear ephemeris.
    static CASTLE_CONSTEXPR uint16_t BBR_COLD_START = 0xFFFFU; ///< Clear all BBR sections.
    /** @} */

    uint16_t nav_bbr_mask = BBR_HOT_START;
    cfg_rst_mode reset_mode = cfg_rst_mode::software_reset;

    /** @brief Hot start (keep all BBR data) with a controlled software reset. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR cfg_rst hot_start(
        cfg_rst_mode mode = cfg_rst_mode::software_reset) CASTLE_NOEXCEPT
    {
        return make(BBR_HOT_START, mode);
    }

    /** @brief Warm start (clear ephemeris) with a controlled software reset. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR cfg_rst warm_start(
        cfg_rst_mode mode = cfg_rst_mode::software_reset) CASTLE_NOEXCEPT
    {
        return make(BBR_WARM_START, mode);
    }

    /** @brief Cold start (clear all BBR sections) with a controlled software reset. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR cfg_rst cold_start(
        cfg_rst_mode mode = cfg_rst_mode::software_reset) CASTLE_NOEXCEPT
    {
        return make(BBR_COLD_START, mode);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.nav_bbr_mask);
        stream.field(self.reset_mode);
        stream.reserved(1U);
    }

private:
    static CASTLE_CONSTEXPR cfg_rst make(uint16_t mask, cfg_rst_mode mode) CASTLE_NOEXCEPT
    {
        cfg_rst message{};
        message.nav_bbr_mask = mask;
        message.reset_mode = mode;
        return message;
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_RST_HPP
