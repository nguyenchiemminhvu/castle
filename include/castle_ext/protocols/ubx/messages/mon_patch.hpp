// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_patch.hpp
 * @brief UBX-MON-PATCH (0x0a 0x27): installed patches.
 *
 * Polled output: a 4-byte header followed by `nEntries` 16-byte patch blocks
 * (`4 + nEntries·16`). Only patches that are installed and enabled are reported.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_PATCH_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_PATCH_HPP

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

/** @brief Patch storage location (`patchInfo` bits 2..1). */
enum class mon_patch_location : uint8_t
{
    efuse = 0U,
    rom = 1U,
    bbr = 2U,
    file_system = 3U
};

/** @brief One patch block of UBX-MON-PATCH. */
struct mon_patch_entry
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 16U;

    /** @name `patchInfo` bits @{ */
    static CASTLE_CONSTEXPR uint32_t PATCH_INFO_ACTIVATED = 0x00000001U;
    static CASTLE_CONSTEXPR uint32_t PATCH_INFO_LOCATION = 0x00000006U;
    static CASTLE_CONSTEXPR uint32_t PATCH_INFO_LOCATION_SHIFT = 1U;
    /** @} */

    uint32_t patch_info = 0U;    ///< Patch status bitfield; see `PATCH_INFO_*`.
    uint32_t comparator = 0U;    ///< Number of the comparator.
    uint32_t patch_address = 0U; ///< Address targeted by the patch.
    uint32_t patch_data = 0U;    ///< Data inserted at `patch_address`.

    /** @brief Reports whether the patch is active. */
    CASTLE_NODISCARD bool activated() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (patch_info & PATCH_INFO_ACTIVATED) != 0U;
    }

    /** @brief Returns where the patch is stored. */
    CASTLE_NODISCARD mon_patch_location location() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<mon_patch_location>((patch_info & PATCH_INFO_LOCATION) >> PATCH_INFO_LOCATION_SHIFT);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.patch_info);
        stream.field(self.comparator);
        stream.field(self.patch_address);
        stream.field(self.patch_data);
    }
};

/** @brief UBX-MON-PATCH output payload. */
template <castle::size_type MaxEntries = 8U>
struct mon_patch
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_PATCH;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;
    static CASTLE_CONSTEXPR castle::size_type block_length = mon_patch_entry::wire_size;

    uint16_t version = 0U;   ///< Message version (0x0001 for this version).
    uint16_t n_entries = 0U; ///< On-wire patch count; may exceed `entries.size()`.
    castle::container::vector<mon_patch_entry, MaxEntries> entries{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxEntries` patch blocks.
     * @return `false` unless the payload length is exactly `4 + nEntries·16`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_patch& out) CASTLE_NOEXCEPT
    {
        out = mon_patch{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.n_entries, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.n_entries, block_length))
        {
            out = mon_patch{};
            return false;
        }

        detail::read_group(reader, out.n_entries, out.entries);
        if (!reader.ok())
        {
            out = mon_patch{};
            return false;
        }
        return true;
    }

    /** @brief Writes the header and every stored entry (`n_entries` is taken from `entries.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint16_t count = static_cast<uint16_t>(entries.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, entries);
    }

private:
    /** @brief Header layout; the count is passed separately so `encode()` need not copy the message. */
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& n_entries, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(n_entries);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_PATCH_HPP
