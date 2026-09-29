// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_VER_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_VER_HPP

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

/** @brief UBX-MON-VER: software/hardware version and extension strings. */
template <castle::size_type MaxExtensions = 10U>
struct mon_ver
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_VER;
    static CASTLE_CONSTEXPR castle::size_type sw_version_len = 30U;
    static CASTLE_CONSTEXPR castle::size_type hw_version_len = 10U;
    static CASTLE_CONSTEXPR castle::size_type extension_len = 30U;
    static CASTLE_CONSTEXPR castle::size_type min_payload_length = 40U;

    char sw_version[sw_version_len]{};
    char hw_version[hw_version_len]{};
    castle::container::array<castle::container::array<char, extension_len>, MaxExtensions> extensions{};
    castle::size_type extension_count = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_ver& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < min_payload_length)
        {
            return false;
        }

        CASTLE_CONST castle::size_type extra = raw.payload_length() - min_payload_length;
        if ((extra % extension_len) != 0U)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = mon_ver{};
        CASTLE_CONST uint8_t* sw = reader.read_bytes(sw_version_len);
        CASTLE_CONST uint8_t* hw = reader.read_bytes(hw_version_len);
        if (!reader.ok())
        {
            return false;
        }

        copy_field(out.sw_version, sw, sw_version_len);
        copy_field(out.hw_version, hw, hw_version_len);
        while (reader.remaining() >= extension_len)
        {
            CASTLE_CONST uint8_t* extension = reader.read_bytes(extension_len);
            if (out.extension_count < MaxExtensions)
            {
                copy_field(out.extensions[out.extension_count].data(), extension, extension_len);
                ++out.extension_count;
            }
        }

        return reader.ok();
    }

private:
    static void copy_field(char* dst, CASTLE_CONST uint8_t* src, castle::size_type count) CASTLE_NOEXCEPT
    {
        for (castle::size_type index = 0U; index < count; ++index)
        {
            dst[index] = static_cast<char>(src[index]);
        }
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_VER_HPP
