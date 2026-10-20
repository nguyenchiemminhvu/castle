// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_rf.hpp
 * @brief UBX-MON-RF (0x0a 0x38): RF information per RF block.
 *
 * Periodic/polled output: a 4-byte header followed by `nBlocks` 24-byte RF
 * blocks (`4 + nBlocks·24`), one per band supported by the receiver.
 *
 * The jamming-state and antenna-supervisor enumerations defined here are
 * shared with the deprecated UBX-MON-HW (`mon_hw.hpp`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RF_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Output of the jamming/interference monitor (UBX-MON-RF / UBX-MON-HW `jammingState`). */
enum class mon_jamming_state : uint8_t
{
    unknown = 0U,  ///< Unknown or feature disabled.
    ok = 1U,       ///< No significant jamming.
    warning = 2U,  ///< Interference visible but fix OK.
    critical = 3U  ///< Interference visible and no fix.
};

/** @brief Antenna supervisor state machine status (`antStatus` / `aStatus`). */
enum class mon_antenna_status : uint8_t
{
    init = 0x00U,
    dont_know = 0x01U,
    ok = 0x02U,
    short_circuit = 0x03U, ///< Spec `SHORT`.
    open = 0x04U
};

/** @brief Antenna power status (`antPower` / `aPower`). */
enum class mon_antenna_power : uint8_t
{
    off = 0x00U,
    on = 0x01U,
    dont_know = 0x02U
};

/** @brief One RF block of UBX-MON-RF. */
struct mon_rf_block
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 24U;

    /** @name `blockId` values @{ */
    static CASTLE_CONSTEXPR uint8_t BLOCK_ID_L1 = 0U;    ///< L1 band.
    static CASTLE_CONSTEXPR uint8_t BLOCK_ID_L2_L5 = 1U; ///< L2 or L5 band, depending on product configuration.
    /** @} */

    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_JAMMING_STATE = 0x03U;
    /** @} */

    uint8_t block_id = BLOCK_ID_L1;
    uint8_t flags = 0U;
    mon_antenna_status ant_status = mon_antenna_status::init;
    mon_antenna_power ant_power = mon_antenna_power::off;
    uint32_t post_status = 0U;   ///< POST status word.
    uint16_t noise_per_ms = 0U;  ///< Noise level measured by the GPS core.
    uint16_t agc_cnt = 0U;       ///< AGC monitor (SIGHI xor SIGLO counts, 0..8191).
    uint8_t jam_ind = 0U;        ///< CW jamming indicator (0 = none, 255 = strong).
    int8_t ofs_i = 0;            ///< I-part imbalance (-128..127).
    uint8_t mag_i = 0U;          ///< I-part magnitude (0..255).
    int8_t ofs_q = 0;            ///< Q-part imbalance (-128..127).
    uint8_t mag_q = 0U;          ///< Q-part magnitude (0..255).

    CASTLE_NODISCARD mon_jamming_state jamming_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<mon_jamming_state>(flags & FLAGS_JAMMING_STATE);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.block_id);
        stream.field(self.flags);
        stream.field(self.ant_status);
        stream.field(self.ant_power);
        stream.field(self.post_status);
        stream.reserved(4U);
        stream.field(self.noise_per_ms);
        stream.field(self.agc_cnt);
        stream.field(self.jam_ind);
        stream.field(self.ofs_i);
        stream.field(self.mag_i);
        stream.field(self.ofs_q);
        stream.field(self.mag_q);
        stream.reserved(3U);
    }
};

/** @brief UBX-MON-RF output payload. */
template <castle::size_type MaxBlocks = 2U>
struct mon_rf
{
    static_assert(MaxBlocks <= 255U, "nBlocks is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_RF;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;
    static CASTLE_CONSTEXPR castle::size_type block_length = mon_rf_block::wire_size;

    uint8_t version = 0U;   ///< Message version (0x00).
    uint8_t n_blocks = 0U;  ///< On-wire block count; may exceed `blocks.size()`.
    castle::container::vector<mon_rf_block, MaxBlocks> blocks{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxBlocks` RF blocks.
     * @return `false` unless the payload length is exactly `4 + nBlocks·24`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_rf& out) CASTLE_NOEXCEPT
    {
        out = mon_rf{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.n_blocks, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.n_blocks, block_length))
        {
            out = mon_rf{};
            return false;
        }

        detail::read_group(reader, out.n_blocks, out.blocks);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`nBlocks` is taken from `blocks.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(blocks.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, blocks);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(count);
        stream.reserved(2U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_RF_HPP
