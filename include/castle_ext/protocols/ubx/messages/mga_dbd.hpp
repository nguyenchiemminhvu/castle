// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mga_dbd.hpp
 * @brief UBX-MGA-DBD (0x13 0x80): navigation database dump entry.
 *
 * Input/output: 12 reserved bytes followed by a free-length, firmware-specific
 * `data` section (`12 + n`). The receiver emits these entries in response to
 * the empty-payload poll request, which needs no struct:
 *
 * @code
 * uint8_t poll[8U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_poll<
 *     castle::protocols::ubx::messages::mga_dbd<>>(poll, sizeof(poll), written);
 * @endcode
 *
 * The receiver ends the dump with a UBX-MGA-ACK whose `msgPayloadStart` holds
 * the number of entries sent. Entries are only meant to be sent back to the
 * receiver that produced them.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_DBD_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_DBD_HPP

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

/**
 * @brief UBX-MGA-DBD database entry.
 *
 * @tparam MaxData Stored `data` capacity in bytes. The spec caps the whole
 * payload at 164 bytes (firmware 2.01 onwards), so the default never truncates.
 */
template <castle::size_type MaxData = 164U>
struct mga_dbd
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MGA;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MGA_DBD;
    static CASTLE_CONSTEXPR castle::size_type header_length = 12U;

    /** @brief Firmware-specific entry bytes (at most `MaxData`; the rest is skipped). */
    castle::container::vector<uint8_t, MaxData> data{};

    /**
     * @brief Reports whether `raw` is a database entry: MGA-DBD with at least
     * the 12-byte header. The empty poll request does not match.
     */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id) && (raw.payload_length() >= header_length);
    }

    /**
     * @brief Decodes the entry; `data` keeps the first `MaxData` bytes.
     * @return `false` for another class/ID or a payload shorter than 12 bytes.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mga_dbd& out) CASTLE_NOEXCEPT
    {
        out = mga_dbd{};
        if (!matches(raw))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        reader.reserved(header_length);
        detail::read_tail(reader, out.data);
        if (!reader.ok())
        {
            out = mga_dbd{};
            return false;
        }
        return true;
    }

    /** @brief Writes the 12 reserved bytes and every stored `data` byte. */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.reserved(header_length);
        detail::write_tail(writer, data);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MGA_DBD_HPP
