// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rxm_rlm.hpp
 * @brief UBX-RXM-RLM (0x02 0x59): Galileo SAR Return Link Message reports.
 *
 * Output with two layouts told apart by payload byte 1 (`type`): the 16-byte
 * short-RLM report (`type` 0x01) and the 28-byte long-RLM report (`type`
 * 0x02). Byte 0 is the fixed message version (0x00), so the discriminator
 * is byte 1 and `detail::typed_message` (which checks byte 0) does not apply;
 * `basic_rxm_rlm` narrows `matches()` to the exact length and the byte-1 type
 * instead. Decoding re-validates both the version and the type byte.
 * Both layouts are identical apart from the size of `params`, so they share
 * one template.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RLM_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RLM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
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

/** @brief `type` byte values of UBX-RXM-RLM. */
static CASTLE_CONSTEXPR uint8_t RXM_RLM_TYPE_SHORT = 0x01U;
static CASTLE_CONSTEXPR uint8_t RXM_RLM_TYPE_LONG = 0x02U;

/** @brief Message version byte of both layouts in this revision. */
static CASTLE_CONSTEXPR uint8_t RXM_RLM_VERSION = 0x00U;

/**
 * @brief Galileo SAR RLM report layout shared by the short and long variants.
 *
 * @tparam Type Value of payload byte 1.
 * @tparam ParamBytes Size of the `params` field (2 short, 12 long).
 * @tparam PayloadLength Exact payload length (16 short, 28 long).
 */
template <uint8_t Type, castle::size_type ParamBytes, castle::size_type PayloadLength>
struct basic_rxm_rlm
    : detail::fixed_message<basic_rxm_rlm<Type, ParamBytes, PayloadLength>, UBX_CLASS_RXM, UBX_ID_RXM_RLM, PayloadLength>
{
    static_assert(PayloadLength > (13U + ParamBytes), "RLM layout needs at least one trailing reserved byte");

    static CASTLE_CONSTEXPR uint8_t message_type = Type;
    static CASTLE_CONSTEXPR castle::size_type type_offset = 1U;

    /** @brief Mask of the 60 significant bits returned by `beacon_id()`. */
    static CASTLE_CONSTEXPR uint64_t BEACON_ID_MASK = 0x0FFFFFFFFFFFFFFFULL;

    uint8_t sv_id = 0U;                                     ///< Transmitting satellite (see Satellite Numbering).
    castle::container::array<uint8_t, 8U> beacon{};         ///< Beacon identifier, most significant byte first.
    uint8_t message = 0U;                                   ///< Message code (4 bits).
    castle::container::array<uint8_t, ParamBytes> params{}; ///< Parameters, most significant byte first.

    /** @brief Reports whether `raw` has this class/ID, exact length, and `type` byte. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(UBX_CLASS_RXM, UBX_ID_RXM_RLM)
            && (raw.payload_length() == PayloadLength)
            && (raw.payload[type_offset] == Type);
    }

    /** @brief The 60-bit beacon identifier assembled from `beacon` (big-endian on the wire). */
    CASTLE_NODISCARD uint64_t beacon_id() CASTLE_CONST CASTLE_NOEXCEPT
    {
        uint64_t value = 0U;
        for (CASTLE_CONST uint8_t byte : beacon)
        {
            value = (value << 8U) | byte;
        }
        return value & BEACON_ID_MASK;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(RXM_RLM_VERSION);
        stream.constant(Type);
        stream.field(self.sv_id);
        stream.reserved(1U);
        stream.field(self.beacon);
        stream.field(self.message);
        stream.field(self.params);
        stream.reserved(PayloadLength - 13U - ParamBytes);
    }
};

/** @brief UBX-RXM-RLM short-RLM report (16 bytes, 16-bit parameters). */
using rxm_rlm_short = basic_rxm_rlm<RXM_RLM_TYPE_SHORT, 2U, 16U>;

/** @brief UBX-RXM-RLM long-RLM report (28 bytes, 96-bit parameters). */
using rxm_rlm_long = basic_rxm_rlm<RXM_RLM_TYPE_LONG, 12U, 28U>;

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_RLM_HPP
