// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_span.hpp
 * @brief UBX-MON-SPAN (0x0a 0x31): signal characteristics (basic spectrum analyzer).
 *
 * Periodic/polled output: a 4-byte header followed by `numRfBlocks` 272-byte
 * RF blocks (`4 + numRfBlocks·272`), one spectrum per RF path.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPAN_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPAN_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
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

/** @brief Number of spectrum bins carried per RF block in UBX-MON-SPAN. */
static CASTLE_CONSTEXPR castle::size_type MON_SPAN_SPECTRUM_BINS = 256U;

/** @brief One RF block of UBX-MON-SPAN. */
struct mon_span_rf_block
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 272U;

    castle::container::array<uint8_t, MON_SPAN_SPECTRUM_BINS> spectrum{}; ///< Amplitude per bin [dB].
    uint32_t span = 0U;   ///< Spectrum span [Hz].
    uint32_t res = 0U;    ///< Resolution of the spectrum (bin width) [Hz].
    uint32_t center = 0U; ///< Center of the spectrum span [Hz].
    uint8_t pga = 0U;     ///< Programmable gain amplifier gain [dB].

    /**
     * @brief Returns the frequency of spectrum bin `bin` (0..255) [Hz]:
     * `center + span * (bin - 128) / 256`, as given by the spec.
     */
    CASTLE_NODISCARD double bin_frequency_hz(castle::size_type bin) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST double offset = static_cast<double>(bin) - static_cast<double>(MON_SPAN_SPECTRUM_BINS / 2U);
        return static_cast<double>(center)
            + ((static_cast<double>(span) * offset) / static_cast<double>(MON_SPAN_SPECTRUM_BINS));
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.spectrum);
        stream.field(self.span);
        stream.field(self.res);
        stream.field(self.center);
        stream.field(self.pga);
        stream.reserved(3U);
    }
};

/** @brief UBX-MON-SPAN output payload. */
template <castle::size_type MaxRfBlocks = 4U>
struct mon_span
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_SPAN;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;
    static CASTLE_CONSTEXPR castle::size_type block_length = mon_span_rf_block::wire_size;

    uint8_t version = 0U;       ///< Message version (0x00 for this version).
    uint8_t num_rf_blocks = 0U; ///< On-wire RF block count; may exceed `rf_blocks.size()`.
    castle::container::vector<mon_span_rf_block, MaxRfBlocks> rf_blocks{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxRfBlocks` RF blocks.
     * @return `false` unless the payload length is exactly `4 + numRfBlocks·272`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_span& out) CASTLE_NOEXCEPT
    {
        out = mon_span{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_rf_blocks, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_rf_blocks, block_length))
        {
            out = mon_span{};
            return false;
        }

        detail::read_group(reader, out.num_rf_blocks, out.rf_blocks);
        if (!reader.ok())
        {
            out = mon_span{};
            return false;
        }
        return true;
    }

    /** @brief Writes the header and every stored RF block (the count is taken from `rf_blocks.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(rf_blocks.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, rf_blocks);
    }

private:
    /** @brief Header layout; the count is passed separately so `encode()` need not copy the message. */
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& num_rf_blocks, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(num_rf_blocks);
        stream.reserved(2U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPAN_HPP
