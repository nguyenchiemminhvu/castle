// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cfg_valset.hpp
 * @brief UBX-CFG-VALSET (0x06 0x8a): set configuration item values.
 *
 * Set message sent by the host; the receiver answers with UBX-ACK-ACK or
 * UBX-ACK-NAK. Two layouts share the class/ID and are told apart by the
 * version byte:
 *  - `cfg_valset` (version 0): applied immediately;
 *  - `cfg_valset_transaction` (version 1): adds a `transaction` byte so
 *    several messages can be applied atomically.
 *
 * Both carry up to 64 key/value pairs (`cfgData`); each value is encoded with
 * the width given by its key's size code (`value_byte_size()`). Encoding a
 * key with an unknown size code fails with `castle::status::invalid_argument`.
 *
 * @code
 * castle::protocols::ubx::messages::cfg_valset<> set{};
 * set.layers = castle::protocols::ubx::messages::cfg_valset<>::LAYERS_RAM;
 * castle::protocols::ubx::config::config_entry rate{};
 * rate.key_id = 0x30210001U; // CFG-RATE-MEAS (U2)
 * rate.value = castle::protocols::ubx::config::config_value(static_cast<uint16_t>(100U));
 * static_cast<void>(set.entries.push_back(rate));
 * uint8_t frame[32U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(set, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALSET_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALSET_HPP

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

/** @brief `layers` bits (X1) of UBX-CFG-VALSET; values match `config::config_layer`. */
struct cfg_valset_layers
{
    static CASTLE_CONSTEXPR uint8_t LAYERS_RAM = static_cast<uint8_t>(config::config_layer::ram);     ///< bit 0.
    static CASTLE_CONSTEXPR uint8_t LAYERS_BBR = static_cast<uint8_t>(config::config_layer::bbr);     ///< bit 1.
    static CASTLE_CONSTEXPR uint8_t LAYERS_FLASH = static_cast<uint8_t>(config::config_layer::flash); ///< bit 2.

    uint8_t layers = LAYERS_RAM; ///< Layers the values are written to.

    CASTLE_NODISCARD bool ram() CASTLE_CONST CASTLE_NOEXCEPT { return (layers & LAYERS_RAM) != 0U; }
    CASTLE_NODISCARD bool bbr() CASTLE_CONST CASTLE_NOEXCEPT { return (layers & LAYERS_BBR) != 0U; }
    CASTLE_NODISCARD bool flash() CASTLE_CONST CASTLE_NOEXCEPT { return (layers & LAYERS_FLASH) != 0U; }
};

/** @brief UBX-CFG-VALSET without transaction (version 0). */
template <castle::size_type MaxEntries = CFG_MAX_KEYS>
struct cfg_valset
    : detail::cfg_message<cfg_valset<MaxEntries>, UBX_ID_CFG_VALSET, 0x00U>
    , cfg_valset_layers
{
    castle::container::vector<config::config_entry, MaxEntries> entries{}; ///< Key/value pairs in wire order.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.layers);
        stream.reserved(2U);
        detail::cfg_data(stream, self.entries);
    }
};

/** @brief UBX-CFG-VALSET with transaction (version 1). */
template <castle::size_type MaxEntries = CFG_MAX_KEYS>
struct cfg_valset_transaction
    : detail::cfg_message<cfg_valset_transaction<MaxEntries>, UBX_ID_CFG_VALSET, 0x01U>
    , cfg_valset_layers
    , cfg_transaction_field
{
    castle::container::vector<config::config_entry, MaxEntries> entries{}; ///< Key/value pairs in wire order (may be empty).

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.layers);
        stream.field(self.transaction);
        stream.reserved(1U);
        detail::cfg_data(stream, self.entries);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALSET_HPP
