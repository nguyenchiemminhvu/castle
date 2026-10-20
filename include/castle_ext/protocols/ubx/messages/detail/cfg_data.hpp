// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cfg_data.hpp
 * @brief Shared base and payload helpers for the variable-length UBX-CFG-VAL* messages.
 *
 * UBX-CFG-VALSET, -VALGET and -VALDEL share one shape: a 4-byte header whose
 * first byte is the message version (which also selects the layout), followed
 * by either a list of `U4` key IDs or `cfgData` (key ID + value whose width is
 * encoded in the key, see `value_byte_size()`). This header provides:
 *  - `cfg_message`: CRTP base deriving `matches()`, `decode()` and `encode()`
 *    from one `layout()`, exactly like `fixed_message`, but for a variable
 *    length payload discriminated by its version byte;
 *  - `cfg_keys()` / `cfg_data()`: layout verbs for the two body kinds;
 *  - `cfg_transaction_action` / `cfg_transaction_field`: the `transaction`
 *    byte shared by the version 1 VALSET and VALDEL layouts.
 *
 * @code
 * template <castle::size_type MaxKeys>
 * struct cfg_valdel : detail::cfg_message<cfg_valdel<MaxKeys>, UBX_ID_CFG_VALDEL, 0x00U>
 * {
 *     uint8_t layers = 0U;
 *     castle::container::vector<uint32_t, MaxKeys> keys{};
 *
 *     template <typename Self, typename Stream>
 *     static void layout(Self& self, Stream& stream) noexcept
 *     {
 *         stream.field(self.layers);   // byte 1 (byte 0 = version, written by the base)
 *         stream.reserved(2U);
 *         detail::cfg_keys(stream, self.keys);
 *     }
 * };
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_CFG_DATA_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_CFG_DATA_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/ubx_config.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Maximum keys (VALGET/VALDEL) or key/value pairs (VALSET, VALGET response) per message. */
static CASTLE_CONSTEXPR castle::size_type CFG_MAX_KEYS = 64U;

/** @brief `transaction.action` of the version 1 UBX-CFG-VALSET / UBX-CFG-VALDEL layouts. */
enum class cfg_transaction_action : uint8_t
{
    transactionless = 0U, ///< Apply immediately; cancels any started transaction.
    start = 1U,           ///< (Re)start a transaction, dropping non-applied messages.
    ongoing = 2U,         ///< Add to the started transaction.
    apply = 3U            ///< Apply and end the transaction.
};

/** @brief `transaction` byte (X1) of the version 1 VALSET / VALDEL layouts. */
struct cfg_transaction_field
{
    static CASTLE_CONSTEXPR uint8_t TRANSACTION_ACTION = 0x03U; ///< bits 1..0.

    uint8_t transaction = 0U;

    /** @brief Returns `transaction` bits 1..0. */
    CASTLE_NODISCARD cfg_transaction_action action() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<cfg_transaction_action>(transaction & TRANSACTION_ACTION);
    }

    /** @brief Replaces `transaction` bits 1..0, keeping the other bits. */
    void set_action(cfg_transaction_action value) CASTLE_NOEXCEPT
    {
        transaction = static_cast<uint8_t>(
            (transaction & static_cast<uint8_t>(~TRANSACTION_ACTION)) | static_cast<uint8_t>(value));
    }
};

namespace detail
{

/**
 * @brief Variable-length UBX-CFG message base selected by its version byte.
 *
 * `Derived::layout()` describes payload bytes 1..end; the base reads and
 * writes the version byte 0 itself, so the version is stated once (the
 * template argument).
 *
 * @tparam Derived Message type providing `template <class Self, class Stream> static void layout(Self&, Stream&)`.
 * @tparam MsgId UBX-CFG message ID byte.
 * @tparam Version Payload byte 0 identifying this layout.
 */
template <typename Derived, uint8_t MsgId, uint8_t Version>
struct cfg_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_CFG;
    static CASTLE_CONSTEXPR uint8_t msg_id = MsgId;
    static CASTLE_CONSTEXPR uint8_t version = Version;
    static CASTLE_CONSTEXPR castle::size_type header_length = 4U;

    /**
     * @brief Reports whether `raw` has this class/ID, at least the 4-byte
     * header, and this layout's version byte.
     */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(UBX_CLASS_CFG, MsgId)
            && (raw.payload_length() >= header_length)
            && (raw.payload[0U] == Version);
    }

    /**
     * @brief Decodes the header and body into `out`.
     * @return `false` when `matches()` fails or the body is malformed (a
     * partial key, a key with an unknown value size, a truncated value);
     * `out` is then value-initialised.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, Derived& out) CASTLE_NOEXCEPT
    {
        out = Derived{};
        if (!matches(raw))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        reader.constant(Version);
        Derived::layout(out, reader);
        if (!reader.ok())
        {
            out = Derived{};
            return false;
        }
        return true;
    }

    /** @brief Writes the payload; see `encode_message()`. */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.constant(Version);
        Derived::layout(static_cast<CASTLE_CONST Derived&>(*this), writer);
    }
};

/**
 * @brief Decodes every remaining `U4` key ID into `keys` (cleared first).
 *
 * Keys beyond `N` are consumed but not stored. A trailing partial key latches
 * a reader failure.
 */
template <castle::size_type N>
CASTLE_INLINE void cfg_keys(payload_reader& reader, castle::container::vector<uint32_t, N>& keys) CASTLE_NOEXCEPT
{
    static_assert(N <= CFG_MAX_KEYS, "UBX-CFG messages carry at most 64 keys");
    keys.clear();
    while (reader.ok() && (reader.remaining() > 0U))
    {
        CASTLE_CONST uint32_t key = reader.read_u32();
        if (reader.ok() && !keys.full())
        {
            static_cast<void>(keys.push_back(key));
        }
    }
}

/** @brief Encodes every stored key ID. */
template <castle::size_type N>
CASTLE_INLINE void cfg_keys(payload_writer& writer, CASTLE_CONST castle::container::vector<uint32_t, N>& keys) CASTLE_NOEXCEPT
{
    static_assert(N <= CFG_MAX_KEYS, "UBX-CFG messages carry at most 64 keys");
    for (CASTLE_CONST uint32_t key : keys)
    {
        writer.write(key);
    }
}

/** @brief Reads one `size`-byte (1, 2, 4 or 8) little-endian configuration value. */
CASTLE_NODISCARD CASTLE_INLINE uint64_t read_cfg_value(payload_reader& reader, uint8_t size) CASTLE_NOEXCEPT
{
    if (size == 1U)
    {
        return reader.read_u8();
    }
    if (size == 2U)
    {
        return reader.read_u16();
    }
    if (size == 4U)
    {
        return reader.read_u32();
    }
    return reader.read_u64();
}

/** @brief Raw bit pattern of a configuration value held as `uint64_t`. */
CASTLE_NODISCARD CASTLE_CONSTEXPR uint64_t cfg_raw_value(uint64_t value) CASTLE_NOEXCEPT
{
    return value;
}

/** @brief Raw bit pattern of a configuration value held as `config::config_value`. */
CASTLE_NODISCARD CASTLE_CONSTEXPR uint64_t cfg_raw_value(CASTLE_CONST config::config_value& value) CASTLE_NOEXCEPT
{
    return value.raw;
}

/**
 * @brief Decodes every remaining `cfgData` key/value pair into `entries` (cleared first).
 *
 * `Entry` exposes `uint32_t key_id` and a `value` constructible from the raw
 * `uint64_t` (`config::config_entry`, `cfg_valget_entry`). The value width
 * follows the key's size code; an unknown size code or a truncated pair
 * latches a reader failure. Pairs beyond `N` are consumed but not stored.
 */
template <typename Entry, castle::size_type N>
CASTLE_INLINE void cfg_data(payload_reader& reader, castle::container::vector<Entry, N>& entries) CASTLE_NOEXCEPT
{
    static_assert(N <= CFG_MAX_KEYS, "UBX-CFG messages carry at most 64 key/value pairs");
    entries.clear();
    while (reader.ok() && (reader.remaining() > 0U))
    {
        Entry entry{};
        entry.key_id = reader.read_u32();
        CASTLE_CONST uint8_t size = value_byte_size(entry.key_id);
        if (size == 0U)
        {
            reader.fail();
            return;
        }
        entry.value = static_cast<decltype(entry.value)>(read_cfg_value(reader, size));
        if (reader.ok() && !entries.full())
        {
            static_cast<void>(entries.push_back(entry));
        }
    }
}

/**
 * @brief Encodes every stored key/value pair, each value `value_byte_size(key)` bytes wide.
 * @note A key with an unknown size code latches `castle::status::invalid_argument`.
 */
template <typename Entry, castle::size_type N>
CASTLE_INLINE void cfg_data(payload_writer& writer, CASTLE_CONST castle::container::vector<Entry, N>& entries) CASTLE_NOEXCEPT
{
    static_assert(N <= CFG_MAX_KEYS, "UBX-CFG messages carry at most 64 key/value pairs");
    for (CASTLE_CONST Entry& entry : entries)
    {
        CASTLE_CONST uint8_t size = value_byte_size(entry.key_id);
        if (size == 0U)
        {
            writer.fail(castle::status::invalid_argument);
            return;
        }
        writer.write(entry.key_id);
        writer.write_uint(cfg_raw_value(entry.value), size);
    }
}

} // namespace detail
} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_CFG_DATA_HPP
