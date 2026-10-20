// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cfg_valget.hpp
 * @brief UBX-CFG-VALGET (0x06 0x8b): get configuration items.
 *
 * Two layouts share the class/ID and are told apart by the version byte:
 *  - `cfg_valget_poll` (version 0): poll request sent by the host, naming a
 *    source layer, a skip `position` and up to 64 key IDs (complete keys or
 *    group / all-groups wild cards);
 *  - `cfg_valget` (version 1): polled response from the receiver carrying up
 *    to 64 key/value pairs (`cfgData`), each value as wide as its key's size code.
 *
 * @code
 * castle::protocols::ubx::messages::cfg_valget_poll<> poll{};
 * poll.layer = castle::protocols::ubx::config::UBX_CFG_VALGET_LAYER_RAM;
 * static_cast<void>(poll.keys.push_back(0x30210001U)); // CFG-RATE-MEAS
 * uint8_t frame[16U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(poll, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALGET_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALGET_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/ubx_config.hpp"
#include "castle_ext/protocols/ubx/messages/detail/cfg_data.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief One UBX-CFG-VALGET response key/value pair. */
struct cfg_valget_entry
{
    uint32_t key_id = 0U; ///< Configuration key ID.
    uint64_t value = 0U;  ///< Raw value, zero-extended from `value_byte_size(key_id)` bytes.
};

/** @brief UBX-CFG-VALGET poll request (version 0). */
template <castle::size_type MaxKeys = CFG_MAX_KEYS>
struct cfg_valget_poll : detail::cfg_message<cfg_valget_poll<MaxKeys>, UBX_ID_CFG_VALGET, 0x00U>
{
    uint8_t layer = config::UBX_CFG_VALGET_LAYER_RAM; ///< Source layer: 0 RAM, 1 BBR, 2 Flash, 7 Default.
    uint16_t position = 0U;                           ///< Key values to skip before building the response.
    castle::container::vector<uint32_t, MaxKeys> keys{}; ///< Key IDs (or wild cards) to retrieve.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.layer);
        stream.field(self.position);
        detail::cfg_keys(stream, self.keys);
    }
};

/** @brief UBX-CFG-VALGET polled response (version 1). */
template <castle::size_type MaxEntries = CFG_MAX_KEYS>
struct cfg_valget : detail::cfg_message<cfg_valget<MaxEntries>, UBX_ID_CFG_VALGET, 0x01U>
{
    uint8_t layer = config::UBX_CFG_VALGET_LAYER_RAM; ///< Layer the values were read from: 0 RAM, 1 BBR, 2 Flash, 7 Default.
    uint16_t position = 0U;                           ///< Items skipped before this message (mirrors the request).
    castle::container::vector<cfg_valget_entry, MaxEntries> entries{}; ///< Decoded key/value pairs in wire order.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.layer);
        stream.field(self.position);
        detail::cfg_data(stream, self.entries);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALGET_HPP
