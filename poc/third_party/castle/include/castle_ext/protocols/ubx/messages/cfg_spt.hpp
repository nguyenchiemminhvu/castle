// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cfg_spt.hpp
 * @brief UBX-CFG-SPT (0x06 0x64): configure and start a sensor production test.
 *
 * Get/set message. The host sends it to run the built-in self-test of a
 * directly connected sensor; results are reported in UBX-MON-SPT. Only
 * supported when a sensor is directly connected to the receiver.
 *
 * @code
 * castle::protocols::ubx::messages::cfg_spt test{};
 * test.sensor_id = 3U; // see UBX-MON-SPT for sensor IDs
 * uint8_t frame[20U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(test, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_SPT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_SPT_HPP

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

/** @brief UBX-CFG-SPT payload. */
struct cfg_spt : detail::fixed_message<cfg_spt, UBX_CLASS_CFG, UBX_ID_CFG_SPT, 12U>
{
    /** @brief Message version (payload byte 0) defined by this spec revision. */
    static CASTLE_CONSTEXPR uint8_t version = 0x00U;

    uint16_t sensor_id = 0U; ///< ID of the sensor to test (see UBX-MON-SPT).

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(version);
        stream.reserved(1U);
        stream.field(self.sensor_id);
        stream.reserved(8U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_SPT_HPP
