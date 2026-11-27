// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_rxr.hpp
 * @brief UBX-MON-RXR (0x0a 0x21): receiver status information.
 *
 * Output (1 byte), sent when the receiver changes from or to backup mode.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RXR_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RXR_HPP

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

/** @brief UBX-MON-RXR output payload. */
struct mon_rxr : detail::fixed_message<mon_rxr, UBX_CLASS_MON, UBX_ID_MON_RXR, 1U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_AWAKE = 0x01U;
    /** @} */

    uint8_t flags = 0U;

    /** @brief `true` when the receiver is awake (not in backup mode). */
    CASTLE_NODISCARD bool awake() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_AWAKE) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.flags);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RXR_HPP
