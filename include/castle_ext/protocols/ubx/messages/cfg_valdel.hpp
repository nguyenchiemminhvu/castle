// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cfg_valdel.hpp
 * @brief UBX-CFG-VALDEL (0x06 0x8c): delete configuration item values.
 *
 * Set message sent by the host to delete saved values from the BBR and/or
 * Flash layer (reverting them to defaults once those layers are loaded into
 * RAM); the receiver answers with UBX-ACK-ACK or UBX-ACK-NAK. Two layouts
 * share the class/ID and are told apart by the version byte:
 *  - `cfg_valdel` (version 0): applied immediately;
 *  - `cfg_valdel_transaction` (version 1): adds a `transaction` byte.
 *
 * Both carry up to 64 `U4` key IDs.
 *
 * @code
 * castle::protocols::ubx::messages::cfg_valdel<> del{};
 * del.layers = castle::protocols::ubx::messages::cfg_valdel<>::LAYERS_FLASH;
 * static_cast<void>(del.keys.push_back(0x30210001U)); // CFG-RATE-MEAS
 * uint8_t frame[16U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(del, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALDEL_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALDEL_HPP

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

/** @brief `layers` bits (X1) of UBX-CFG-VALDEL; values match `config::config_layer`. */
struct cfg_valdel_layers
{
    static CASTLE_CONSTEXPR uint8_t LAYERS_BBR = static_cast<uint8_t>(config::config_layer::bbr);     ///< bit 1.
    static CASTLE_CONSTEXPR uint8_t LAYERS_FLASH = static_cast<uint8_t>(config::config_layer::flash); ///< bit 2.

    uint8_t layers = LAYERS_BBR | LAYERS_FLASH; ///< Layers the values are deleted from.

    CASTLE_NODISCARD bool bbr() CASTLE_CONST CASTLE_NOEXCEPT { return (layers & LAYERS_BBR) != 0U; }
    CASTLE_NODISCARD bool flash() CASTLE_CONST CASTLE_NOEXCEPT { return (layers & LAYERS_FLASH) != 0U; }
};

/** @brief UBX-CFG-VALDEL without transaction (version 0). */
template <castle::size_type MaxKeys = CFG_MAX_KEYS>
struct cfg_valdel
    : detail::cfg_message<cfg_valdel<MaxKeys>, UBX_ID_CFG_VALDEL, 0x00U>
    , cfg_valdel_layers
{
    castle::container::vector<uint32_t, MaxKeys> keys{}; ///< Key IDs to delete.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.layers);
        stream.reserved(2U);
        detail::cfg_keys(stream, self.keys);
    }
};

/** @brief UBX-CFG-VALDEL with transaction (version 1). */
template <castle::size_type MaxKeys = CFG_MAX_KEYS>
struct cfg_valdel_transaction
    : detail::cfg_message<cfg_valdel_transaction<MaxKeys>, UBX_ID_CFG_VALDEL, 0x01U>
    , cfg_valdel_layers
    , cfg_transaction_field
{
    castle::container::vector<uint32_t, MaxKeys> keys{}; ///< Key IDs to delete (may be empty).

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.layers);
        stream.field(self.transaction);
        stream.reserved(1U);
        detail::cfg_keys(stream, self.keys);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALDEL_HPP
