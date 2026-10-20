// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file esf_raw.hpp
 * @brief UBX-ESF-RAW (0x10 0x03): raw sensor measurements.
 *
 * Output: 4 reserved bytes followed by `N` 8-byte blocks (`4 + N·8`), each a
 * data word (accelerometer, gyroscope or temperature reading) plus its
 * sensor time tag. `N` is not carried in the payload; it is implied by the
 * payload length.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_RAW_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_RAW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/esf_data.hpp"
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

/** @brief One sample block of UBX-ESF-RAW. */
struct esf_raw_sample
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 8U;
    /** @brief `dataType` mask (bits 31..24 of the word, after shifting). */
    static CASTLE_CONSTEXPR uint8_t DATA_TYPE_MASK = 0xFFU;

    uint8_t data_type = 0U; ///< `dataType` (0 = no data; 1..255 = data type).
    int32_t data_value = 0; ///< `dataField`, sign-extended 24-bit value.
    uint32_t s_ttag = 0U;   ///< Sensor time tag.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        detail::esf_data_word<DATA_TYPE_MASK>(stream, self.data_type, self.data_value);
        stream.field(self.s_ttag);
    }
};

/** @brief UBX-ESF-RAW output payload. */
template <castle::size_type MaxSamples = 16U>
struct esf_raw
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ESF;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_ESF_RAW;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;
    static CASTLE_CONSTEXPR castle::size_type block_length = esf_raw_sample::wire_size;

    castle::size_type num_samples = 0U; ///< On-wire block count implied by the length; may exceed `samples.size()`.
    castle::container::vector<esf_raw_sample, MaxSamples> samples{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes up to `MaxSamples` sample blocks.
     * @return `false` unless the payload length is `4 + N·8`; `out` is then value-initialised.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, esf_raw& out) CASTLE_NOEXCEPT
    {
        out = esf_raw{};
        if (!matches(raw)
            || !detail::implied_group_count(raw.payload_length(), header_length, block_length, out.num_samples))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        reader.reserved(header_length);
        detail::read_group(reader, out.num_samples, out.samples);
        if (!reader.ok())
        {
            out = esf_raw{};
            return false;
        }
        return true;
    }

    /**
     * @brief Writes the reserved header and every stored sample (`num_samples` is not used).
     * @note A sample whose `data_value` does not fit 24 bits latches `castle::status::out_of_range`.
     */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.reserved(header_length);
        detail::write_group(writer, samples);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_RAW_HPP
