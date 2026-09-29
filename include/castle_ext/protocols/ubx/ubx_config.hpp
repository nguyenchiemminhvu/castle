// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ubx_config.hpp
 * @brief Header-only UBX CFG-VALSET/VALGET configuration message builders for Castle.
 *
 * This module depends only on `castle_ext/protocols/ubx/ubx.hpp` (not on the
 * parser) and reuses its framing, little-endian, and CFG key-size helpers
 * instead of re-implementing them:
 *  - `config_value` / `config_entry`: an opaque, strongly typed key-value pair;
 *  - `build_valset_frame()`: encodes one UBX-CFG-VALSET write request;
 *  - `build_valget_frame()`: encodes one UBX-CFG-VALGET poll request;
 *  - `parse_valget_response()`: decodes a UBX-CFG-VALGET response payload.
 *
 * No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers
 * are used.
 *
 * @code
 * #include "castle_ext/protocols/ubx/ubx_config.hpp"
 *
 * using namespace castle::protocols::ubx::config;
 *
 * config_entry entries[1] = {
 *     {0x30210001U, config_value(static_cast<uint16_t>(1000U))} // CFG-RATE-MEAS
 * };
 *
 * uint8_t frame[64U];
 * castle::size_type written = 0U;
 * build_valset_frame(
 *     castle::container::array_view<const config_entry>(entries, 1U),
 *     static_cast<uint8_t>(config_layer::ram),
 *     frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_UBX_CONFIG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_UBX_CONFIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace config
{

/**
 * @brief UBX configuration storage layers (bit flags; combine with `operator|`).
 */
enum class config_layer : uint8_t
{
    ram   = 0x01U,
    bbr   = 0x02U,
    flash = 0x04U
};

/**
 * @brief Combines two storage layers into a raw VALSET layer bitmask.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR uint8_t operator|(config_layer lhs, config_layer rhs) CASTLE_NOEXCEPT
{
    return static_cast<uint8_t>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

/**
 * @brief VALGET source layer selector (UBX spec, distinct from `config_layer`'s VALSET bitmask).
 */
static CASTLE_CONSTEXPR uint8_t UBX_CFG_VALGET_LAYER_RAM = 0U;
static CASTLE_CONSTEXPR uint8_t UBX_CFG_VALGET_LAYER_BBR = 1U;
static CASTLE_CONSTEXPR uint8_t UBX_CFG_VALGET_LAYER_FLASH = 2U;
static CASTLE_CONSTEXPR uint8_t UBX_CFG_VALGET_LAYER_DEFAULT = 7U;

/**
 * @brief Opaque, strongly typed storage for one UBX configuration value.
 *
 * The wire byte width is determined by the associated key ID (via
 * `value_byte_size()`), not by this type, so every value is stored as its raw
 * bit pattern in a `uint64_t` for zero-overhead construction and comparison.
 */
struct config_value
{
    uint64_t raw = 0U;

    CASTLE_CONSTEXPR config_value() CASTLE_NOEXCEPT = default;

    CASTLE_CONSTEXPR explicit config_value(bool value) CASTLE_NOEXCEPT
        : raw(value ? 1ULL : 0ULL) {}
    CASTLE_CONSTEXPR explicit config_value(uint8_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(value)) {}
    CASTLE_CONSTEXPR explicit config_value(uint16_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(value)) {}
    CASTLE_CONSTEXPR explicit config_value(uint32_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(value)) {}
    CASTLE_CONSTEXPR explicit config_value(uint64_t value) CASTLE_NOEXCEPT
        : raw(value) {}

    // Signed integers are stored as their two's-complement bit pattern.
    CASTLE_CONSTEXPR explicit config_value(int8_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(static_cast<uint8_t>(value))) {}
    CASTLE_CONSTEXPR explicit config_value(int16_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(static_cast<uint16_t>(value))) {}
    CASTLE_CONSTEXPR explicit config_value(int32_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(static_cast<uint32_t>(value))) {}
    CASTLE_CONSTEXPR explicit config_value(int64_t value) CASTLE_NOEXCEPT
        : raw(static_cast<uint64_t>(value)) {}

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool as_bool() CASTLE_CONST CASTLE_NOEXCEPT { return raw != 0U; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR uint8_t as_u8() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<uint8_t>(raw); }
    CASTLE_NODISCARD CASTLE_CONSTEXPR uint16_t as_u16() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<uint16_t>(raw); }
    CASTLE_NODISCARD CASTLE_CONSTEXPR uint32_t as_u32() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<uint32_t>(raw); }
    CASTLE_NODISCARD CASTLE_CONSTEXPR uint64_t as_u64() CASTLE_CONST CASTLE_NOEXCEPT { return raw; }
    CASTLE_NODISCARD CASTLE_CONSTEXPR int8_t as_i8() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<int8_t>(as_u8()); }
    CASTLE_NODISCARD CASTLE_CONSTEXPR int16_t as_i16() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<int16_t>(as_u16()); }
    CASTLE_NODISCARD CASTLE_CONSTEXPR int32_t as_i32() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<int32_t>(as_u32()); }
    CASTLE_NODISCARD CASTLE_CONSTEXPR int64_t as_i64() CASTLE_CONST CASTLE_NOEXCEPT { return static_cast<int64_t>(raw); }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(CASTLE_CONST config_value& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return raw == other.raw;
    }
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST config_value& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return raw != other.raw;
    }
};

/**
 * @brief One UBX configuration key/value pair.
 */
struct config_entry
{
    uint32_t key_id = 0U;
    config_value value{};
};

/**
 * @brief Writes one `value_size`-byte little-endian value into `output`.
 * @return `false` when `value_size` is not one of the UBX-recognized widths (1/2/4/8).
 */
inline bool write_config_value(
    CASTLE_CONST config_value& value, uint8_t value_size, uint8_t* output) CASTLE_NOEXCEPT
{
    switch (value_size)
    {
        case 1U: output[0U] = value.as_u8(); return true;
        case 2U: castle::write_le16(output, value.as_u16()); return true;
        case 4U: castle::write_le32(output, value.as_u32()); return true;
        case 8U: castle::write_le64(output, value.as_u64()); return true;
        default: return false; // LCOV_EXCL_BR_LINE
    }
}

/**
 * @brief Reads a `value_size`-byte little-endian value from `data`.
 * @return A zeroed `config_value` when `value_size` is not a recognized width.
 */
CASTLE_NODISCARD inline config_value read_config_value(CASTLE_CONST uint8_t* data, uint8_t value_size) CASTLE_NOEXCEPT
{
    switch (value_size)
    {
        case 1U: return config_value(data[0U]);
        case 2U: return config_value(castle::read_le16(data));
        case 4U: return config_value(castle::read_le32(data));
        case 8U: return config_value(castle::read_le64(data));
        default: return config_value();
    }
}

/**
 * @brief Encodes a complete UBX-CFG-VALSET write request into a caller-provided buffer.
 *
 * @tparam MaxPayloadLen Scratch payload capacity; must fit the header plus every entry's key+value.
 * @param entries One or more key-value pairs to write to the target layers.
 * @param layers VALSET layer bitmask (see `config_layer`, combine with `operator|`).
 * @param output Destination frame buffer.
 * @param output_capacity Size of the destination buffer.
 * @param[out] bytes_written Number of bytes generated on success, otherwise zero.
 * @return `castle::status::invalid_argument` for an empty entry list, a null
 * output pointer, or a key with an unrecognized size code; `castle::status::full`
 * when the scratch payload or output buffer is too small; otherwise forwards
 * `encode_frame()`'s result.
 */
template <castle::size_type MaxPayloadLen = UBX_SAFE_MAX_PAYLOAD_LEN>
CASTLE_NODISCARD inline castle::status build_valset_frame(
    castle::container::array_view<CASTLE_CONST config_entry> entries,
    uint8_t layers,
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;

    if ((entries.size() == 0U) || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    castle::container::array<uint8_t, MaxPayloadLen> scratch{};
    castle::size_type pos = 4U; // version + layers + 2 reserved bytes, filled in below

    if (pos > MaxPayloadLen)
    {
        return castle::status::full;
    }

    scratch[0U] = UBX_VALSET_VERSION;
    scratch[1U] = layers;
    scratch[2U] = 0U;
    scratch[3U] = 0U;

    for (castle::size_type index = 0U; index < entries.size(); ++index)
    {
        CASTLE_CONST config_entry& entry = entries[index];
        CASTLE_CONST uint8_t value_size = value_byte_size(entry.key_id);
        if (value_size == 0U)
        {
            return castle::status::invalid_argument;
        }
        if ((pos + 4U + value_size) > MaxPayloadLen)
        {
            return castle::status::full;
        }

        castle::write_le32(&scratch[pos], entry.key_id);
        pos += 4U;
        write_config_value(entry.value, value_size, &scratch[pos]);
        pos += value_size;
    }

    return encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET,
        castle::container::array_view<CASTLE_CONST uint8_t>(scratch.data(), pos),
        output, output_capacity, bytes_written);
}

/**
 * @brief Encodes a complete UBX-CFG-VALGET poll request into a caller-provided buffer.
 *
 * @tparam MaxPayloadLen Scratch payload capacity; must fit the header plus every key ID.
 * @param key_ids Keys to poll from the target.
 * @param layer Source layer to query, e.g. `UBX_CFG_VALGET_LAYER_RAM`.
 * @param position First entry offset for large, chunked responses (0 = start).
 * @param output Destination frame buffer.
 * @param output_capacity Size of the destination buffer.
 * @param[out] bytes_written Number of bytes generated on success, otherwise zero.
 * @return `castle::status::invalid_argument` for an empty key list or a null
 * output pointer; `castle::status::full` when the scratch payload or output
 * buffer is too small; otherwise forwards `encode_frame()`'s result.
 */
template <castle::size_type MaxPayloadLen = UBX_SAFE_MAX_PAYLOAD_LEN>
CASTLE_NODISCARD inline castle::status build_valget_frame(
    castle::container::array_view<CASTLE_CONST uint32_t> key_ids,
    uint8_t layer,
    uint16_t position,
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;

    if ((key_ids.size() == 0U) || (output == nullptr))
    {
        return castle::status::invalid_argument;
    }

    CASTLE_CONST castle::size_type required = 4U + (key_ids.size() * 4U);
    if (required > MaxPayloadLen)
    {
        return castle::status::full;
    }

    castle::container::array<uint8_t, MaxPayloadLen> scratch{};
    scratch[0U] = UBX_VALGET_VERSION_POLL;
    scratch[1U] = layer;
    castle::write_le16(&scratch[2U], position);

    castle::size_type pos = 4U;
    for (castle::size_type index = 0U; index < key_ids.size(); ++index)
    {
        castle::write_le32(&scratch[pos], key_ids[index]);
        pos += 4U;
    }

    return encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALGET,
        castle::container::array_view<CASTLE_CONST uint8_t>(scratch.data(), pos),
        output, output_capacity, bytes_written);
}

/**
 * @brief Decodes a UBX-CFG-VALGET response payload into caller-provided storage.
 *
 * @param payload Response payload (version + layer + position + key/value pairs),
 * e.g. from `message_view::payload` once `message_view::is(UBX_CLASS_CFG, UBX_ID_CFG_VALGET)`.
 * @param entries Caller-provided backing storage for the decoded entries.
 * @param entries_capacity Number of elements available in `entries`.
 * @return Number of entries written to `entries` (clamped to `entries_capacity`).
 * Stops early, without error, at the first malformed or unrecognized key.
 */
CASTLE_NODISCARD inline castle::size_type parse_valget_response(
    castle::container::array_view<CASTLE_CONST uint8_t> payload,
    config_entry* entries,
    castle::size_type entries_capacity) CASTLE_NOEXCEPT
{
    static CASTLE_CONSTEXPR castle::size_type header_size = 4U; // version + layer + 2 position bytes

    if ((payload.size() < header_size) || (entries == nullptr) || (entries_capacity == 0U))
    {
        return 0U;
    }

    castle::size_type pos = header_size;
    castle::size_type count = 0U;

    while (((pos + 4U) <= payload.size()) && (count < entries_capacity))
    {
        CASTLE_CONST uint32_t key_id = castle::read_le32(&payload[pos]);
        CASTLE_CONST uint8_t value_size = value_byte_size(key_id);
        if ((value_size == 0U) || ((pos + 4U + value_size) > payload.size()))
        {
            break;
        }

        entries[count].key_id = key_id;
        entries[count].value = read_config_value(&payload[pos + 4U], value_size);
        ++count;

        pos += 4U + value_size;
    }

    return count;
}

} // namespace config
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_UBX_CONFIG_HPP
