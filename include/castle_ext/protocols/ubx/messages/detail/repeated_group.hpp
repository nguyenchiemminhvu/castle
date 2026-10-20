// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file repeated_group.hpp
 * @brief Shared decoding/encoding of UBX "repeated group" payload sections.
 *
 * Many UBX payloads are a fixed header followed by `N` equally sized blocks
 * (`8 + numSvs·12`, `4 + nBlocks·24`, ...). The two recurring steps live here
 * once:
 *  - `has_group_length()` / `implied_group_count()` validate the payload size;
 *  - `read_group()` / `write_group()` walk the blocks with the same
 *    `payload_reader` / `payload_writer` layout verbs used by
 *    `fixed_message`.
 *
 * A block type describes its wire format exactly like a fixed message:
 *
 * @code
 * struct nav_geofence_fence
 * {
 *     static constexpr castle::size_type wire_size = 2U;
 *     uint8_t state = 0U;
 *     uint8_t id = 0U;
 *
 *     template <typename Self, typename Stream>
 *     static void layout(Self& self, Stream& stream) noexcept
 *     {
 *         stream.field(self.state);
 *         stream.field(self.id);
 *     }
 * };
 * @endcode
 *
 * Decoded blocks land in a fixed-capacity `castle::container::vector`, whose
 * `size()` is the number actually stored. Blocks beyond its capacity are
 * skipped, so the reader cursor always ends on the next field and the
 * message's spec-named count field still reports the on-wire count.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_REPEATED_GROUP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_REPEATED_GROUP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{
namespace detail
{

/**
 * @brief Reports whether `payload_length == header_length + count * block_size`.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool has_group_length(
    castle::size_type payload_length,
    castle::size_type header_length,
    castle::size_type count,
    castle::size_type block_size) CASTLE_NOEXCEPT
{
    return payload_length == (header_length + (count * block_size));
}

/**
 * @brief Computes the block count implied by the payload length.
 *
 * Used by groups whose count is not carried in a header field
 * (e.g. UBX-MON-IO, UBX-ESF-RAW, UBX-MON-VER extensions).
 *
 * @param[out] count_out Number of whole blocks after `header_length`; zero on failure.
 * @return `false` when the payload is shorter than the header or the remainder
 * is not a whole number of blocks.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool implied_group_count(
    castle::size_type payload_length,
    castle::size_type header_length,
    castle::size_type block_size,
    castle::size_type& count_out) CASTLE_NOEXCEPT
{
    count_out = 0U;
    if ((payload_length < header_length) || (((payload_length - header_length) % block_size) != 0U))
    {
        return false;
    }
    count_out = (payload_length - header_length) / block_size;
    return true;
}

/**
 * @brief Decodes `wire_count` consecutive blocks into `out` (cleared first).
 *
 * Blocks beyond `out.capacity()` are skipped, so the cursor always advances by
 * exactly `wire_count * Block::wire_size` bytes. Read failures latch into
 * `reader.ok()`, which the calling decoder checks once.
 *
 * @note Complexity: O(wire_count).
 */
template <typename Block, castle::size_type N>
CASTLE_INLINE void read_group(
    payload_reader& reader,
    castle::size_type wire_count,
    castle::container::vector<Block, N>& out) CASTLE_NOEXCEPT
{
    out.clear();
    for (castle::size_type index = 0U; index < wire_count; ++index)
    {
        if (out.full())
        {
            reader.skip((wire_count - index) * Block::wire_size);
            return;
        }

        Block block{};
        Block::layout(block, reader);
        static_cast<void>(out.push_back(block));
    }
}

/**
 * @brief Encodes every block stored in `blocks`, in order.
 * @note Complexity: O(blocks.size()).
 */
template <typename Block, castle::size_type N>
CASTLE_INLINE void write_group(
    payload_writer& writer,
    CASTLE_CONST castle::container::vector<Block, N>& blocks) CASTLE_NOEXCEPT
{
    for (CASTLE_CONST Block& block : blocks)
    {
        Block::layout(block, writer);
    }
}

/**
 * @brief Stores up to `N` bytes of a free-length trailing byte section (e.g.
 * UBX-MGA-DBD `data`, UBX-INF `str`) and skips the rest.
 */
template <typename T, castle::size_type N>
CASTLE_INLINE void read_tail(payload_reader& reader, castle::container::vector<T, N>& out) CASTLE_NOEXCEPT
{
    static_assert(sizeof(T) == 1U, "read_tail stores byte-sized elements only");
    out.clear();
    while ((reader.remaining() > 0U) && !out.full())
    {
        T value{};
        reader.read(value);
        static_cast<void>(out.push_back(value));
    }
    reader.skip(reader.remaining());
}

/** @brief Encodes every byte stored in `bytes`. */
template <typename T, castle::size_type N>
CASTLE_INLINE void write_tail(payload_writer& writer, CASTLE_CONST castle::container::vector<T, N>& bytes) CASTLE_NOEXCEPT
{
    for (CASTLE_CONST T& value : bytes)
    {
        writer.write(value);
    }
}

} // namespace detail
} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_REPEATED_GROUP_HPP
