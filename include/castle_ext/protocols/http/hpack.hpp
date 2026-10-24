// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_HPACK_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_HPACK_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/http/types.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace http
{

namespace detail
{
struct hpack_static_field { char const* name; char const* value; };
inline CASTLE_CONSTEXPR hpack_static_field hpack_static_table[61] =
{
    {":authority", ""}, {":method", "GET"}, {":method", "POST"}, {":path", "/"},
    {":path", "/index.html"}, {":scheme", "http"}, {":scheme", "https"},
    {":status", "200"}, {":status", "204"}, {":status", "206"}, {":status", "304"},
    {":status", "400"}, {":status", "404"}, {":status", "500"},
    {"accept-charset", ""}, {"accept-encoding", "gzip, deflate"}, {"accept-language", ""},
    {"accept-ranges", ""}, {"accept", ""}, {"access-control-allow-origin", ""},
    {"age", ""}, {"allow", ""}, {"authorization", ""}, {"cache-control", ""},
    {"content-disposition", ""}, {"content-encoding", ""}, {"content-language", ""},
    {"content-length", ""}, {"content-location", ""}, {"content-range", ""},
    {"content-type", ""}, {"cookie", ""}, {"date", ""}, {"etag", ""}, {"expect", ""},
    {"expires", ""}, {"from", ""}, {"host", ""}, {"if-match", ""}, {"if-modified-since", ""},
    {"if-none-match", ""}, {"if-range", ""}, {"if-unmodified-since", ""}, {"last-modified", ""},
    {"link", ""}, {"location", ""}, {"max-forwards", ""}, {"proxy-authenticate", ""},
    {"proxy-authorization", ""}, {"range", ""}, {"referer", ""}, {"refresh", ""},
    {"retry-after", ""}, {"server", ""}, {"set-cookie", ""}, {"strict-transport-security", ""},
    {"transfer-encoding", ""}, {"user-agent", ""}, {"vary", ""}, {"via", ""}, {"www-authenticate", ""}
};

inline CASTLE_CONSTEXPR uint32_t hpack_huffman_codes[257] =
{
    0x00001FF8u,
    0x007FFFD8u,
    0x0FFFFFE2u,
    0x0FFFFFE3u,
    0x0FFFFFE4u,
    0x0FFFFFE5u,
    0x0FFFFFE6u,
    0x0FFFFFE7u,
    0x0FFFFFE8u,
    0x00FFFFEAu,
    0x3FFFFFFCu,
    0x0FFFFFE9u,
    0x0FFFFFEAu,
    0x3FFFFFFDu,
    0x0FFFFFEBu,
    0x0FFFFFECu,
    0x0FFFFFEDu,
    0x0FFFFFEEu,
    0x0FFFFFEFu,
    0x0FFFFFF0u,
    0x0FFFFFF1u,
    0x0FFFFFF2u,
    0x3FFFFFFEu,
    0x0FFFFFF3u,
    0x0FFFFFF4u,
    0x0FFFFFF5u,
    0x0FFFFFF6u,
    0x0FFFFFF7u,
    0x0FFFFFF8u,
    0x0FFFFFF9u,
    0x0FFFFFFAu,
    0x0FFFFFFBu,
    0x00000014u,
    0x000003F8u,
    0x000003F9u,
    0x00000FFAu,
    0x00001FF9u,
    0x00000015u,
    0x000000F8u,
    0x000007FAu,
    0x000003FAu,
    0x000003FBu,
    0x000000F9u,
    0x000007FBu,
    0x000000FAu,
    0x00000016u,
    0x00000017u,
    0x00000018u,
    0x00000000u,
    0x00000001u,
    0x00000002u,
    0x00000019u,
    0x0000001Au,
    0x0000001Bu,
    0x0000001Cu,
    0x0000001Du,
    0x0000001Eu,
    0x0000001Fu,
    0x0000005Cu,
    0x000000FBu,
    0x00007FFCu,
    0x00000020u,
    0x00000FFBu,
    0x000003FCu,
    0x00001FFAu,
    0x00000021u,
    0x0000005Du,
    0x0000005Eu,
    0x0000005Fu,
    0x00000060u,
    0x00000061u,
    0x00000062u,
    0x00000063u,
    0x00000064u,
    0x00000065u,
    0x00000066u,
    0x00000067u,
    0x00000068u,
    0x00000069u,
    0x0000006Au,
    0x0000006Bu,
    0x0000006Cu,
    0x0000006Du,
    0x0000006Eu,
    0x0000006Fu,
    0x00000070u,
    0x00000071u,
    0x00000072u,
    0x000000FCu,
    0x00000073u,
    0x000000FDu,
    0x00001FFBu,
    0x0007FFF0u,
    0x00001FFCu,
    0x00003FFCu,
    0x00000022u,
    0x00007FFDu,
    0x00000003u,
    0x00000023u,
    0x00000004u,
    0x00000024u,
    0x00000005u,
    0x00000025u,
    0x00000026u,
    0x00000027u,
    0x00000006u,
    0x00000074u,
    0x00000075u,
    0x00000028u,
    0x00000029u,
    0x0000002Au,
    0x00000007u,
    0x0000002Bu,
    0x00000076u,
    0x0000002Cu,
    0x00000008u,
    0x00000009u,
    0x0000002Du,
    0x00000077u,
    0x00000078u,
    0x00000079u,
    0x0000007Au,
    0x0000007Bu,
    0x00007FFEu,
    0x000007FCu,
    0x00003FFDu,
    0x00001FFDu,
    0x0FFFFFFCu,
    0x000FFFE6u,
    0x003FFFD2u,
    0x000FFFE7u,
    0x000FFFE8u,
    0x003FFFD3u,
    0x003FFFD4u,
    0x003FFFD5u,
    0x007FFFD9u,
    0x003FFFD6u,
    0x007FFFDAu,
    0x007FFFDBu,
    0x007FFFDCu,
    0x007FFFDDu,
    0x007FFFDEu,
    0x00FFFFEBu,
    0x007FFFDFu,
    0x00FFFFECu,
    0x00FFFFEDu,
    0x003FFFD7u,
    0x007FFFE0u,
    0x00FFFFEEu,
    0x007FFFE1u,
    0x007FFFE2u,
    0x007FFFE3u,
    0x007FFFE4u,
    0x001FFFDCu,
    0x003FFFD8u,
    0x007FFFE5u,
    0x003FFFD9u,
    0x007FFFE6u,
    0x007FFFE7u,
    0x00FFFFEFu,
    0x003FFFDAu,
    0x001FFFDDu,
    0x000FFFE9u,
    0x003FFFDBu,
    0x003FFFDCu,
    0x007FFFE8u,
    0x007FFFE9u,
    0x001FFFDEu,
    0x007FFFEAu,
    0x003FFFDDu,
    0x003FFFDEu,
    0x00FFFFF0u,
    0x001FFFDFu,
    0x003FFFDFu,
    0x007FFFEBu,
    0x007FFFECu,
    0x001FFFE0u,
    0x001FFFE1u,
    0x003FFFE0u,
    0x001FFFE2u,
    0x007FFFEDu,
    0x003FFFE1u,
    0x007FFFEEu,
    0x007FFFEFu,
    0x000FFFEAu,
    0x003FFFE2u,
    0x003FFFE3u,
    0x003FFFE4u,
    0x007FFFF0u,
    0x003FFFE5u,
    0x003FFFE6u,
    0x007FFFF1u,
    0x03FFFFE0u,
    0x03FFFFE1u,
    0x000FFFEBu,
    0x0007FFF1u,
    0x003FFFE7u,
    0x007FFFF2u,
    0x003FFFE8u,
    0x01FFFFECu,
    0x03FFFFE2u,
    0x03FFFFE3u,
    0x03FFFFE4u,
    0x07FFFFDEu,
    0x07FFFFDFu,
    0x03FFFFE5u,
    0x00FFFFF1u,
    0x01FFFFEDu,
    0x0007FFF2u,
    0x001FFFE3u,
    0x03FFFFE6u,
    0x07FFFFE0u,
    0x07FFFFE1u,
    0x03FFFFE7u,
    0x07FFFFE2u,
    0x00FFFFF2u,
    0x001FFFE4u,
    0x001FFFE5u,
    0x03FFFFE8u,
    0x03FFFFE9u,
    0x0FFFFFFDu,
    0x07FFFFE3u,
    0x07FFFFE4u,
    0x07FFFFE5u,
    0x000FFFECu,
    0x00FFFFF3u,
    0x000FFFEDu,
    0x001FFFE6u,
    0x003FFFE9u,
    0x001FFFE7u,
    0x001FFFE8u,
    0x007FFFF3u,
    0x003FFFEAu,
    0x003FFFEBu,
    0x01FFFFEEu,
    0x01FFFFEFu,
    0x00FFFFF4u,
    0x00FFFFF5u,
    0x03FFFFEAu,
    0x007FFFF4u,
    0x03FFFFEBu,
    0x07FFFFE6u,
    0x03FFFFECu,
    0x03FFFFEDu,
    0x07FFFFE7u,
    0x07FFFFE8u,
    0x07FFFFE9u,
    0x07FFFFEAu,
    0x07FFFFEBu,
    0x0FFFFFFEu,
    0x07FFFFECu,
    0x07FFFFEDu,
    0x07FFFFEEu,
    0x07FFFFEFu,
    0x07FFFFF0u,
    0x03FFFFEEu,
    0x3FFFFFFFu
};
inline CASTLE_CONSTEXPR uint8_t hpack_huffman_lengths[257] =
{
    13U, 23U, 28U, 28U, 28U, 28U, 28U, 28U, 28U, 24U, 30U, 28U, 28U, 30U, 28U, 28U, 28U, 28U, 28U, 28U, 28U, 28U, 30U, 28U, 28U, 28U, 28U, 28U, 28U, 28U, 28U, 28U, 6U, 10U, 10U, 12U, 13U, 6U, 8U, 11U, 10U, 10U, 8U, 11U, 8U, 6U, 6U, 6U, 5U, 5U, 5U, 6U, 6U, 6U, 6U, 6U, 6U, 6U, 7U, 8U, 15U, 6U, 12U, 10U, 13U, 6U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 8U, 7U, 8U, 13U, 19U, 13U, 14U, 6U, 15U, 5U, 6U, 5U, 6U, 5U, 6U, 6U, 6U, 5U, 7U, 7U, 6U, 6U, 6U, 5U, 6U, 7U, 6U, 5U, 5U, 6U, 7U, 7U, 7U, 7U, 7U, 15U, 11U, 14U, 13U, 28U, 20U, 22U, 20U, 20U, 22U, 22U, 22U, 23U, 22U, 23U, 23U, 23U, 23U, 23U, 24U, 23U, 24U, 24U, 22U, 23U, 24U, 23U, 23U, 23U, 23U, 21U, 22U, 23U, 22U, 23U, 23U, 24U, 22U, 21U, 20U, 22U, 22U, 23U, 23U, 21U, 23U, 22U, 22U, 24U, 21U, 22U, 23U, 23U, 21U, 21U, 22U, 21U, 23U, 22U, 23U, 23U, 20U, 22U, 22U, 22U, 23U, 22U, 22U, 23U, 26U, 26U, 20U, 19U, 22U, 23U, 22U, 25U, 26U, 26U, 26U, 27U, 27U, 26U, 24U, 25U, 19U, 21U, 26U, 27U, 27U, 26U, 27U, 24U, 21U, 21U, 26U, 26U, 28U, 27U, 27U, 27U, 20U, 24U, 20U, 21U, 22U, 21U, 21U, 23U, 22U, 22U, 25U, 25U, 24U, 24U, 26U, 23U, 26U, 27U, 26U, 26U, 27U, 27U, 27U, 27U, 27U, 28U, 27U, 27U, 27U, 27U, 27U, 26U, 30U
};

CASTLE_INLINE castle::status hpack_read_integer(uint8_t const* input, size_type size, size_type& offset,
                                                uint8_t prefix, uint64_t& value) CASTLE_NOEXCEPT
{
    if (input == nullptr || offset >= size || prefix == 0U || prefix > 8U) { return castle::status::data_loss; }
    uint8_t mask = static_cast<uint8_t>((1U << prefix) - 1U);
    value = input[offset] & mask;
    ++offset;
    if (value < mask) { return castle::status::ok; }

    uint32_t shift = 0U;
    for (uint8_t count = 0U; count < 10U; ++count)
    {
        if (offset >= size) { return castle::status::data_loss; }
        uint8_t byte = input[offset++];
        uint64_t part = static_cast<uint64_t>(byte & 0x7FU);
        if (shift >= 64U || part > (static_cast<uint64_t>(-1) >> shift)) { return castle::status::out_of_range; }
        uint64_t addend = part << shift;
        if (value > static_cast<uint64_t>(-1) - addend) { return castle::status::out_of_range; }
        value += addend;
        if ((byte & 0x80U) == 0U) { return castle::status::ok; }
        shift += 7U;
    }
    return castle::status::out_of_range;
}

CASTLE_INLINE size_type hpack_write_integer(uint8_t* output, size_type capacity, size_type offset,
                                             uint8_t prefix, uint8_t high_bits, uint32_t value) CASTLE_NOEXCEPT
{
    uint8_t mask = static_cast<uint8_t>((1U << prefix) - 1U);
    if (offset >= capacity) { return static_cast<size_type>(-1); }
    if (value < mask) { output[offset++] = static_cast<uint8_t>(high_bits | value); return offset; }
    output[offset++] = static_cast<uint8_t>(high_bits | mask);
    value -= mask;
    while (value >= 128U)
    {
        if (offset >= capacity) { return static_cast<size_type>(-1); }
        output[offset++] = static_cast<uint8_t>((value & 0x7FU) | 0x80U);
        value >>= 7U;
    }
    if (offset >= capacity) { return static_cast<size_type>(-1); }
    output[offset++] = static_cast<uint8_t>(value);
    return offset;
}

struct huffman_node { int16_t child[2]; int16_t symbol; };
struct huffman_tree { huffman_node nodes[1024]; uint16_t count; };

CASTLE_CONSTEXPR huffman_tree build_huffman_tree() CASTLE_NOEXCEPT
{
    huffman_tree tree{};
    tree.count = 1U;
    for (size_type i = 0U; i < 1024U; ++i)
    {
        tree.nodes[i].child[0] = -1;
        tree.nodes[i].child[1] = -1;
        tree.nodes[i].symbol = -1;
    }
    for (uint16_t symbol = 0U; symbol < 257U; ++symbol)
    {
        uint16_t node = 0U;
        uint8_t length = hpack_huffman_lengths[symbol];
        uint32_t code = hpack_huffman_codes[symbol];
        for (uint8_t bit = 0U; bit < length; ++bit)
        {
            uint8_t direction = static_cast<uint8_t>((code >> (length - bit - 1U)) & 1U);
            int16_t next = tree.nodes[node].child[direction];
            if (next < 0)
            {
                next = static_cast<int16_t>(tree.count++);
                tree.nodes[node].child[direction] = next;
            }
            node = static_cast<uint16_t>(next);
        }
        tree.nodes[node].symbol = static_cast<int16_t>(symbol);
    }
    return tree;
}
inline CASTLE_CONSTEXPR huffman_tree hpack_huffman_tree = build_huffman_tree();
static_assert(hpack_huffman_tree.count <= 1024U, "HPACK Huffman tree capacity is insufficient");

CASTLE_INLINE castle::status hpack_decode_huffman(uint8_t const* input, size_type input_size,
                                                  char* output, size_type output_capacity,
                                                  size_type& output_size) CASTLE_NOEXCEPT
{
    output_size = 0U;
    uint16_t node = 0U;
    uint8_t tail_bits = 0U;
    bool tail_ones = true;
    for (size_type i = 0U; i < input_size; ++i)
    {
        for (int bit = 7; bit >= 0; --bit)
        {
            uint8_t direction = static_cast<uint8_t>((input[i] >> bit) & 1U);
            tail_bits = static_cast<uint8_t>(tail_bits + 1U);
            tail_ones = tail_ones && (direction == 1U);
            int16_t next = hpack_huffman_tree.nodes[node].child[direction];
            if (next < 0) { return castle::status::invalid_argument; }
            node = static_cast<uint16_t>(next);
            int16_t symbol = hpack_huffman_tree.nodes[node].symbol;
            if (symbol >= 0)
            {
                if (symbol == 256 || output_size >= output_capacity) { return castle::status::out_of_range; }
                output[output_size++] = static_cast<char>(symbol);
                node = 0U;
                tail_bits = 0U;
                tail_ones = true;
            }
        }
    }
    if (node != 0U && (tail_bits > 7U || !tail_ones)) { return castle::status::invalid_argument; }
    return castle::status::ok;
}
} // namespace detail

/** @brief Bounded RFC 7541 decoder with the complete static table and Huffman alphabet. */
template <size_type MaxDynamicTableBytes = 0U,
          size_type MaxStringSize = 256U,
          size_type MaxDynamicEntries = (MaxDynamicTableBytes / 32U) + 1U>
class hpack_decoder
{
    static_assert(MaxStringSize > 0U, "HPACK string capacity must be non-zero");
    static_assert(MaxDynamicEntries > 0U, "HPACK dynamic entry capacity must be non-zero");
    struct dynamic_entry
    {
        char name[MaxStringSize + 1U]{};
        char value[MaxStringSize + 1U]{};
        size_type name_size = 0U;
        size_type value_size = 0U;
        size_type bytes = 0U;
    };
public:
    hpack_decoder() CASTLE_DEFAULT;

    template <typename Headers>
    castle::status decode_block(uint8_t const* input, size_type size, Headers& output) CASTLE_NOEXCEPT
    {
        if (input == nullptr && size != 0U) { return castle::status::invalid_argument; }
        size_type offset = 0U;
        bool saw_field = false;
        char name[MaxStringSize + 1U]{};
        char value[MaxStringSize + 1U]{};
        while (offset < size)
        {
            uint8_t first = input[offset];
            if ((first & 0x80U) != 0U)
            {
                uint64_t index = 0U;
                castle::status result = detail::hpack_read_integer(input, size, offset, 7U, index);
                if (!castle::succeeded(result) || index == 0U) { return castle::status::invalid_argument; }
                string_view n, v;
                if (!lookup(static_cast<size_type>(index), n, v)) { return castle::status::invalid_argument; }
                result = output.add(n, v);
                if (!castle::succeeded(result)) { return result; }
                saw_field = true;
                continue;
            }
            if ((first & 0xE0U) == 0x20U)
            {
                if (saw_field) { return castle::status::invalid_argument; }
                uint64_t table_size = 0U;
                castle::status result = detail::hpack_read_integer(input, size, offset, 5U, table_size);
                if (!castle::succeeded(result) || table_size > MaxDynamicTableBytes) { return castle::status::invalid_argument; }
                allowed_table_bytes_ = static_cast<size_type>(table_size);
                evict_to_limit();
                continue;
            }

            bool incremental = (first & 0xC0U) == 0x40U;
            bool never_indexed = (first & 0xF0U) == 0x10U;
            uint8_t prefix = incremental ? 6U : (never_indexed ? 4U : 4U);
            uint64_t name_index = 0U;
            castle::status result = detail::hpack_read_integer(input, size, offset, prefix, name_index);
            if (!castle::succeeded(result)) { return result; }
            string_view n;
            string_view v;
            size_type name_size = 0U;
            if (name_index != 0U)
            {
                string_view ignored;
                if (name_index > static_cast<uint64_t>(static_cast<size_type>(-1)) ||
                    !lookup(static_cast<size_type>(name_index), n, ignored))
                {
                    return castle::status::invalid_argument;
                }
                if (n.size() > MaxStringSize) { return castle::status::out_of_range; }
                copy_to(name, n);
                name_size = n.size();
            }
            else
            {
                result = read_string(input, size, offset, name, name_size);
                if (!castle::succeeded(result)) { return result; }
            }
            size_type value_size = 0U;
            result = read_string(input, size, offset, value, value_size);
            if (!castle::succeeded(result)) { return result; }
            if (name_size == 0U) { return castle::status::invalid_argument; }
            result = output.add(string_view(name, name_size), string_view(value, value_size));
            if (!castle::succeeded(result)) { return result; }
            if (incremental)
            {
                result = insert(string_view(name, name_size), string_view(value, value_size));
                if (!castle::succeeded(result)) { return result; }
            }
            saw_field = true;
        }
        return castle::status::ok;
    }

    size_type dynamic_table_size() CASTLE_CONST CASTLE_NOEXCEPT { return table_bytes_; }
    size_type dynamic_entry_count() CASTLE_CONST CASTLE_NOEXCEPT { return entry_count_; }
    size_type maximum_table_size() CASTLE_CONST CASTLE_NOEXCEPT { return allowed_table_bytes_; }
    void clear_dynamic_table() CASTLE_NOEXCEPT
    {
        entry_count_ = 0U;
        table_bytes_ = 0U;
        allowed_table_bytes_ = MaxDynamicTableBytes;
    }

private:
    static size_type string_length(char const* text) CASTLE_NOEXCEPT
    {
        size_type i = 0U; while (text[i] != '\0') { ++i; } return i;
    }
    static void copy_to(char* out, string_view input) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < input.size(); ++i) { out[i] = input[i]; }
        out[input.size()] = '\0';
    }

    bool lookup(size_type index, string_view& name, string_view& value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index <= 61U)
        {
            name = string_view(detail::hpack_static_table[index - 1U].name);
            value = string_view(detail::hpack_static_table[index - 1U].value);
            return true;
        }
        size_type dynamic_index = index - 62U;
        if (dynamic_index >= entry_count_) { return false; }
        dynamic_entry const& entry = entries_[dynamic_index];
        name = string_view(entry.name, entry.name_size);
        value = string_view(entry.value, entry.value_size);
        return true;
    }

    castle::status read_string(uint8_t const* input, size_type size, size_type& offset,
                               char* output, size_type& output_size) CASTLE_NOEXCEPT
    {
        if (offset >= size) { return castle::status::data_loss; }
        bool huffman = (input[offset] & 0x80U) != 0U;
        uint64_t encoded_size = 0U;
        castle::status result = detail::hpack_read_integer(input, size, offset, 7U, encoded_size);
        if (!castle::succeeded(result) || encoded_size > static_cast<uint64_t>(size - offset)) { return castle::status::data_loss; }
        if (huffman)
        {
            result = detail::hpack_decode_huffman(input + offset, static_cast<size_type>(encoded_size),
                                                   output, MaxStringSize, output_size);
        }
        else
        {
            if (encoded_size > MaxStringSize) { return castle::status::out_of_range; }
            output_size = static_cast<size_type>(encoded_size);
            for (size_type i = 0U; i < output_size; ++i) { output[i] = static_cast<char>(input[offset + i]); }
            result = castle::status::ok;
        }
        offset += static_cast<size_type>(encoded_size);
        output[output_size] = '\0';
        return result;
    }

    castle::status insert(string_view name, string_view value) CASTLE_NOEXCEPT
    {
        if (name.size() > static_cast<size_type>(-1) - 32U ||
            value.size() > static_cast<size_type>(-1) - 32U - name.size())
        {
            return castle::status::out_of_range;
        }
        size_type bytes = 32U + name.size() + value.size();
        if (bytes > allowed_table_bytes_ || MaxDynamicTableBytes == 0U)
        {
            clear_dynamic_table();
            return castle::status::ok;
        }
        if (name.size() > MaxStringSize || value.size() > MaxStringSize) { return castle::status::out_of_range; }
        while (entry_count_ != 0U && (table_bytes_ > allowed_table_bytes_ - bytes || entry_count_ == MaxDynamicEntries))
        {
            table_bytes_ -= entries_[entry_count_ - 1U].bytes;
            --entry_count_;
        }
        if (entry_count_ >= MaxDynamicEntries) { return castle::status::full; }
        for (size_type i = entry_count_; i > 0U; --i) { entries_[i] = entries_[i - 1U]; }
        dynamic_entry& entry = entries_[0U];
        copy_to(entry.name, name);
        copy_to(entry.value, value);
        entry.name_size = name.size();
        entry.value_size = value.size();
        entry.bytes = bytes;
        ++entry_count_;
        table_bytes_ += bytes;
        return castle::status::ok;
    }

    void evict_to_limit() CASTLE_NOEXCEPT
    {
        while (entry_count_ != 0U && table_bytes_ > allowed_table_bytes_)
        {
            table_bytes_ -= entries_[entry_count_ - 1U].bytes;
            --entry_count_;
        }
    }

    dynamic_entry entries_[MaxDynamicEntries]{};
    size_type entry_count_ = 0U;
    size_type table_bytes_ = 0U;
    size_type allowed_table_bytes_ = MaxDynamicTableBytes;
};

/** @brief Bounded HPACK literal encoder; it emits valid non-Huffman, non-indexed fields. */
class hpack_encoder
{
public:
    static castle::status encode_literal(string_view name, string_view value,
                                         uint8_t* output, size_type capacity, size_type& used) CASTLE_NOEXCEPT
    {
        used = 0U;
        if (output == nullptr || name.empty()) { return castle::status::invalid_argument; }
        if (name.size() > 0xFFFFFFFFULL || value.size() > 0xFFFFFFFFULL) { return castle::status::out_of_range; }
        size_type offset = detail::hpack_write_integer(output, capacity, 0U, 4U, 0x00U, 0U);
        if (offset == static_cast<size_type>(-1)) { return castle::status::full; }
        offset = encode_string(name, output, capacity, offset);
        if (offset == static_cast<size_type>(-1)) { return castle::status::full; }
        offset = encode_string(value, output, capacity, offset);
        if (offset == static_cast<size_type>(-1)) { return castle::status::full; }
        used = offset;
        return castle::status::ok;
    }

private:
    static size_type encode_string(string_view value, uint8_t* output,
                                   size_type capacity, size_type offset) CASTLE_NOEXCEPT
    {
        size_type next = detail::hpack_write_integer(output, capacity, offset, 7U, 0U,
                                                      static_cast<uint32_t>(value.size()));
        if (next == static_cast<size_type>(-1) || value.size() > capacity - next)
        {
            return static_cast<size_type>(-1);
        }
        for (size_type i = 0U; i < value.size(); ++i) { output[next++] = static_cast<uint8_t>(value[i]); }
        return next;
    }
};

} // namespace http
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_HTTP_HPACK_HPP
