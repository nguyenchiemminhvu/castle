// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPAN_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPAN_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Number of spectrum bins carried per RF block in UBX-MON-SPAN. */
static CASTLE_CONSTEXPR castle::size_type MON_SPAN_SPECTRUM_BINS = 256U;

/** @brief One RF block's spectrum snapshot decoded from UBX-MON-SPAN. */
struct mon_span_rf_block
{
    castle::container::array<uint8_t, MON_SPAN_SPECTRUM_BINS> spectrum{};
    uint32_t span = 0U;
    uint32_t res = 0U;
    uint32_t center = 0U;
    uint8_t pga = 0U;
};

/** @brief UBX-MON-SPAN: per-RF-block spectrum analyzer snapshot. */
template <castle::size_type MaxRfBlocks = 4U>
struct mon_span
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_SPAN;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;
    static CASTLE_CONSTEXPR castle::size_type block_length = 272U;

    uint8_t version = 0U;
    uint8_t num_rf_blocks = 0U;
    castle::container::array<mon_span_rf_block, MaxRfBlocks> rf_blocks{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_span& out) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::size_type payload_len = raw.payload_length();
        if (payload_len < (header_length + block_length))
        {
            return false;
        }

        castle::size_type remaining_bytes = payload_len - header_length;
        if ((remaining_bytes % block_length) != 0U)
        {
            return false;
        }

        castle::size_type block_count = remaining_bytes / block_length;
        payload_reader reader(raw.payload);
        out = mon_span{};
        out.version = reader.read_u8();
        uint8_t declared = reader.read_u8();
        static_cast<void>(declared);
        reader.skip(2U);

        castle::size_type count = (block_count < MaxRfBlocks) ? block_count : MaxRfBlocks;
        out.num_rf_blocks = static_cast<uint8_t>(count);
        for (castle::size_type index = 0U; index < count; ++index)
        {
            for (castle::size_type bin = 0U; bin < MON_SPAN_SPECTRUM_BINS; ++bin)
            {
                out.rf_blocks[index].spectrum[bin] = reader.read_u8();
            }
            out.rf_blocks[index].span = reader.read_u32();
            out.rf_blocks[index].res = reader.read_u32();
            out.rf_blocks[index].center = reader.read_u32();
            out.rf_blocks[index].pga = reader.read_u8();
            reader.skip(3U);
        }
        if (block_count > count)
        {
            reader.skip((block_count - count) * block_length);
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_SPAN_HPP
