// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_gnss.hpp
 * @brief UBX-MON-GNSS (0x0a 0x28): major GNSS selection.
 *
 * Polled output (8 bytes). The `supported`, `defaultGnss` and `enabled`
 * bitmasks share one bit assignment (GPS, GLONASS, BeiDou, Galileo), exposed
 * as the `GNSS_*` masks. Poll with `encode_poll<mon_gnss>()`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_GNSS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_GNSS_HPP

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

/** @brief UBX-MON-GNSS output payload. */
struct mon_gnss : detail::fixed_message<mon_gnss, UBX_CLASS_MON, UBX_ID_MON_GNSS, 8U>
{
    /** @name Bits of `supported` / `default_gnss` / `enabled` @{ */
    static CASTLE_CONSTEXPR uint8_t GNSS_GPS = 0x01U;
    static CASTLE_CONSTEXPR uint8_t GNSS_GLONASS = 0x02U;
    static CASTLE_CONSTEXPR uint8_t GNSS_BEIDOU = 0x04U;
    static CASTLE_CONSTEXPR uint8_t GNSS_GALILEO = 0x08U;
    /** @} */

    uint8_t version = 0x01U;  ///< Message version (0x01).
    uint8_t supported = 0U;   ///< Major GNSS supported by this receiver.
    uint8_t default_gnss = 0U; ///< Default major GNSS selection.
    uint8_t enabled = 0U;     ///< Currently enabled major GNSS selection.
    uint8_t simultaneous = 0U; ///< Maximum number of concurrent major GNSS.

    /** @brief `true` when every bit of `gnss_mask` (`GNSS_*`) is set in `supported`. */
    CASTLE_NODISCARD bool is_supported(uint8_t gnss_mask) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (supported & gnss_mask) == gnss_mask;
    }

    /** @brief `true` when every bit of `gnss_mask` (`GNSS_*`) is set in `default_gnss`. */
    CASTLE_NODISCARD bool is_default(uint8_t gnss_mask) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (default_gnss & gnss_mask) == gnss_mask;
    }

    /** @brief `true` when every bit of `gnss_mask` (`GNSS_*`) is set in `enabled`. */
    CASTLE_NODISCARD bool is_enabled(uint8_t gnss_mask) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (enabled & gnss_mask) == gnss_mask;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(self.supported);
        stream.field(self.default_gnss);
        stream.field(self.enabled);
        stream.field(self.simultaneous);
        stream.reserved(3U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_GNSS_HPP
