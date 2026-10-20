// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_hw.hpp
 * @brief UBX-MON-HW (0x0a 0x09): hardware status.
 *
 * Periodic/polled output (60 bytes). Deprecated in this protocol version in
 * favour of UBX-MON-HW3 (`mon_hw3.hpp`) and UBX-MON-RF (`mon_rf.hpp`); still
 * emitted by the receiver. Reuses the jamming-state and antenna enumerations
 * of `mon_rf.hpp`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"
#include "castle_ext/protocols/ubx/messages/mon_rf.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-MON-HW output payload. */
struct mon_hw : detail::fixed_message<mon_hw, UBX_CLASS_MON, UBX_ID_MON_HW, 60U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_RTC_CALIB = 0x01U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_SAFE_BOOT = 0x02U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_JAMMING_STATE = 0x0CU;
    static CASTLE_CONSTEXPR uint8_t FLAGS_XTAL_ABSENT = 0x10U;
    /** @} */

    /** @brief Number of physical pins mapped by `vp`. */
    static CASTLE_CONSTEXPR castle::size_type VP_COUNT = 17U;

    uint32_t pin_sel = 0U;   ///< Mask of pins set as peripheral/PIO.
    uint32_t pin_bank = 0U;  ///< Mask of pins set as bank A/B.
    uint32_t pin_dir = 0U;   ///< Mask of pins set as input/output.
    uint32_t pin_val = 0U;   ///< Mask of pin values low/high.
    uint16_t noise_per_ms = 0U; ///< Noise level measured by the GPS core.
    uint16_t agc_cnt = 0U;      ///< AGC monitor (SIGHI xor SIGLO counts, 0..8191).
    mon_antenna_status a_status = mon_antenna_status::init;
    mon_antenna_power a_power = mon_antenna_power::off;
    uint8_t flags = 0U;
    uint32_t used_mask = 0U; ///< Mask of pins used by the virtual pin manager.
    castle::container::array<uint8_t, VP_COUNT> vp{}; ///< Pin mapping for each of the 17 physical pins.
    uint8_t jam_ind = 0U;    ///< CW jamming indicator (0 = none, 255 = strong).
    uint32_t pin_irq = 0U;   ///< Mask of pins using the PIO IRQ.
    uint32_t pull_h = 0U;    ///< Mask of pins using the PIO pull-high resistor.
    uint32_t pull_l = 0U;    ///< Mask of pins using the PIO pull-low resistor.

    CASTLE_NODISCARD bool rtc_calib() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_RTC_CALIB) != 0U;
    }

    CASTLE_NODISCARD bool safe_boot() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SAFE_BOOT) != 0U;
    }

    CASTLE_NODISCARD mon_jamming_state jamming_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<mon_jamming_state>((flags & FLAGS_JAMMING_STATE) >> 2U);
    }

    CASTLE_NODISCARD bool xtal_absent() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_XTAL_ABSENT) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.pin_sel);
        stream.field(self.pin_bank);
        stream.field(self.pin_dir);
        stream.field(self.pin_val);
        stream.field(self.noise_per_ms);
        stream.field(self.agc_cnt);
        stream.field(self.a_status);
        stream.field(self.a_power);
        stream.field(self.flags);
        stream.reserved(1U);
        stream.field(self.used_mask);
        stream.field(self.vp);
        stream.field(self.jam_ind);
        stream.reserved(2U);
        stream.field(self.pin_irq);
        stream.field(self.pull_h);
        stream.field(self.pull_l);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW_HPP
