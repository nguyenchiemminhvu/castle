// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_ver.hpp
 * @brief UBX-MON-VER (0x0a 0x04): receiver and software version.
 *
 * Polled output: `swVersion` CH[30] and `hwVersion` CH[10] followed by
 * `[0..n]` CH[30] extension strings (`40 + [0..n]·30`). The extension count
 * is implied by the payload length. Request it with
 * `encode_poll<mon_ver<>>(buffer, capacity, written)`.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_VER_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_VER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string.hpp"
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

/** @brief UBX-MON-VER output payload. */
template <castle::size_type MaxExtensions = 10U>
struct mon_ver
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_VER;
    static CASTLE_CONSTEXPR castle::size_type sw_version_len = 30U;
    static CASTLE_CONSTEXPR castle::size_type hw_version_len = 10U;
    static CASTLE_CONSTEXPR castle::size_type extension_len = 30U;
    static CASTLE_CONSTEXPR castle::size_type header_length = sw_version_len + hw_version_len;
    static CASTLE_CONSTEXPR castle::size_type block_length = extension_len;

    castle::container::string<sw_version_len> sw_version{}; ///< Software version (NUL padding stripped).
    castle::container::string<hw_version_len> hw_version{}; ///< Hardware version (NUL padding stripped).
    /** @brief Extension strings in wire order; extensions beyond `MaxExtensions` are skipped. */
    castle::container::vector<castle::container::string<extension_len>, MaxExtensions> extensions{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes both version strings and up to `MaxExtensions` extensions.
     * @return `false` unless the payload length is exactly `40 + n·30`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_ver& out) CASTLE_NOEXCEPT
    {
        out = mon_ver{};
        castle::size_type count = 0U;
        if (!matches(raw) || !detail::implied_group_count(raw.payload_length(), header_length, block_length, count))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        reader.text(out.sw_version);
        reader.text(out.hw_version);
        for (castle::size_type index = 0U; index < count; ++index)
        {
            if (out.extensions.full())
            {
                reader.reserved((count - index) * extension_len);
                break;
            }
            castle::container::string<extension_len> extension{};
            reader.text(extension);
            static_cast<void>(out.extensions.push_back(extension));
        }
        if (!reader.ok())
        {
            out = mon_ver{};
            return false;
        }
        return true;
    }

    /** @brief Writes both version strings and every stored extension, NUL-padded to their wire widths. */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.text(sw_version);
        writer.text(hw_version);
        for (CASTLE_CONST castle::container::string<extension_len>& extension : extensions)
        {
            writer.text(extension);
        }
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_VER_HPP
