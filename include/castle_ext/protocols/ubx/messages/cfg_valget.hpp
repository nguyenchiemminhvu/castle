// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALGET_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALGET_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/ubx_config.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief One decoded UBX-CFG-VALGET key/value pair. */
struct cfg_valget_entry
{
    uint32_t key_id = 0U;
    uint64_t value = 0U;
};

/** @brief UBX-CFG-VALGET: configuration key/value poll response. */
template <castle::size_type MaxEntries = 32U>
struct cfg_valget
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_CFG;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_CFG_VALGET;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;

    uint8_t version = 0U;
    uint8_t layer = 0U;
    uint16_t position = 0U;
    castle::container::array<cfg_valget_entry, MaxEntries> entries{};
    castle::size_type entry_count = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static uint8_t value_size(uint32_t key) CASTLE_NOEXCEPT
    {
        return value_byte_size(key);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, cfg_valget& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < header_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = cfg_valget{};
        out.version = reader.read_u8();
        out.layer = reader.read_u8();
        out.position = reader.read_u16();
        while (reader.remaining() >= 4U)
        {
            uint32_t key = reader.read_u32();
            uint8_t size = value_size(key);
            if (size == 0U)
            {
                break;
            }
            if (reader.remaining() < size)
            {
                return false;
            }

            uint64_t value = 0U;
            switch (size)
            {
                case 1U: value = reader.read_u8(); break;
                case 2U: value = reader.read_u16(); break;
                case 4U: value = reader.read_u32(); break;
                case 8U: value = reader.read_u64(); break;
                default: return false; // LCOV_EXCL_BR_LINE
            }

            if (out.entry_count < MaxEntries)
            {
                out.entries[out.entry_count].key_id = key;
                out.entries[out.entry_count].value = value;
                ++out.entry_count;
            }
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_CFG_VALGET_HPP
