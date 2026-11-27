// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_hw2.hpp
 * @brief UBX-MON-HW2 (0x0a 0x0b): extended hardware status.
 *
 * Periodic/polled output (28 bytes): RF front-end I/Q imbalance and
 * magnitude, low-level configuration source and POST status. Deprecated in
 * this protocol version in favour of UBX-MON-HW3 and UBX-MON-RF.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW2_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW2_HPP

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

/** @brief Source of the low-level configuration (`cfgSource`). */
enum class mon_hw2_cfg_source : uint8_t
{
    flash_image = 102U, ///< 'f'.
    otp = 111U,         ///< 'o'.
    config_pins = 112U, ///< 'p'.
    rom = 114U          ///< 'r'.
};

/** @brief UBX-MON-HW2 output payload. */
struct mon_hw2 : detail::fixed_message<mon_hw2, UBX_CLASS_MON, UBX_ID_MON_HW2, 28U>
{
    int8_t ofs_i = 0;   ///< I-part imbalance (-128..127).
    uint8_t mag_i = 0U; ///< I-part magnitude (0..255).
    int8_t ofs_q = 0;   ///< Q-part imbalance (-128..127).
    uint8_t mag_q = 0U; ///< Q-part magnitude (0..255).
    mon_hw2_cfg_source cfg_source = mon_hw2_cfg_source::rom;
    uint32_t low_lev_cfg = 0U; ///< Low-level configuration (obsolete for protocol > 15.00).
    uint32_t post_status = 0U; ///< POST status word.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.ofs_i);
        stream.field(self.mag_i);
        stream.field(self.ofs_q);
        stream.field(self.mag_q);
        stream.field(self.cfg_source);
        stream.reserved(3U);
        stream.field(self.low_lev_cfg);
        stream.reserved(8U);
        stream.field(self.post_status);
        stream.reserved(4U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW2_HPP
