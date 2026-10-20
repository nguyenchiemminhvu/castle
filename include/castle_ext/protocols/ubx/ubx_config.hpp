// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ubx_config.hpp
 * @brief Shared UBX configuration vocabulary: storage layers, typed values and key/value entries.
 *
 * These types are consumed by the CFG message models, which own all wire
 * encoding and decoding:
 *  - `messages::cfg_valset` / `cfg_valset_transaction` (UBX-CFG-VALSET);
 *  - `messages::cfg_valget_poll` / `cfg_valget` (UBX-CFG-VALGET request/response);
 *  - `messages::cfg_valdel` / `cfg_valdel_transaction` (UBX-CFG-VALDEL).
 *
 * A value's wire width is never stored here; it follows the key's size code
 * (`castle::protocols::ubx::value_byte_size()`).
 *
 * No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers
 * are used.
 *
 * @code
 * #include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"
 *
 * namespace ubx = castle::protocols::ubx;
 *
 * ubx::messages::cfg_valset<> set{};
 * set.layers = ubx::config::config_layer::ram | ubx::config::config_layer::flash;
 * static_cast<void>(set.entries.push_back(
 *     ubx::config::config_entry{0x30210001U, ubx::config::config_value(static_cast<uint16_t>(1000U))}));
 *
 * uint8_t frame[64U];
 * castle::size_type written = 0U;
 * castle::status result = ubx::encode_message(set, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_UBX_CONFIG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_UBX_CONFIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

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

    CASTLE_CONSTEXPR config_value() CASTLE_NOEXCEPT CASTLE_DEFAULT;

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

} // namespace config
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_UBX_CONFIG_HPP
