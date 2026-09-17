#ifndef CASTLE_SERIALIZATION_XML_HPP
#define CASTLE_SERIALIZATION_XML_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"
#include "castle/utility/string_builder.hpp"

#include <float.h>
#include <stdint.h>

namespace castle
{
namespace serialization
{
namespace xml
{

using string_view = castle::container::string_view;

// XML DOM node kinds supported by Castle's embedded parser. The document node
// is internal and is exposed only through document::document_node().
enum class type : uint8_t
{
    document = 0U,
    element,
    text,
    cdata,
    comment,
    processing_instruction,
    declaration,
    doctype
};

enum class error_code : uint8_t
{
    none = 0U,
    unexpected_end,
    unexpected_token,
    malformed_name,
    mismatched_tag,
    duplicate_attribute,
    invalid_attribute,
    invalid_entity,
    invalid_character,
    invalid_comment,
    invalid_cdata,
    invalid_processing_instruction,
    invalid_declaration,
    invalid_doctype,
    unsupported_construct,
    depth_exceeded,
    capacity,
    output_full,
    invalid_argument
};

struct result
{
    castle::status status = castle::status::ok;
    error_code code = error_code::none;
    castle::size_type offset = 0U;
    castle::size_type line = 1U;
    castle::size_type column = 1U;

    CASTLE_CONSTEXPR bool succeeded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::succeeded(status);
    }
};

namespace detail
{

template <typename Document>
class parser;

template <typename T>
using enable_integer_t = meta::enable_if_t<meta::is_integral<T>::value && !meta::is_same<T, bool>::value, castle::status>;

template <typename T>
using enable_float_t = meta::enable_if_t<meta::is_floating_point<T>::value, castle::status>;

template <typename T>
using enable_enum_t = meta::enable_if_t<meta::is_enum<T>::value, castle::status>;

inline bool equal(string_view a, string_view b) CASTLE_NOEXCEPT
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (castle::size_type i = 0U; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

inline bool equal_ci(string_view a, string_view b) CASTLE_NOEXCEPT
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (castle::size_type i = 0U; i < a.size(); ++i)
    {
        char lhs = a[i];
        char rhs = b[i];
        if (lhs >= 'A' && lhs <= 'Z')
        {
            lhs = static_cast<char>(lhs - 'A' + 'a');
        }
        if (rhs >= 'A' && rhs <= 'Z')
        {
            rhs = static_cast<char>(rhs - 'A' + 'a');
        }
        if (lhs != rhs)
        {
            return false;
        }
    }
    return true;
}

inline bool is_space(char c) CASTLE_NOEXCEPT
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline bool is_ascii_name_start(char c) CASTLE_NOEXCEPT
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == ':';
}

inline bool is_ascii_name_char(char c) CASTLE_NOEXCEPT
{
    return is_ascii_name_start(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
}

inline bool is_hex(char c) CASTLE_NOEXCEPT
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline uint32_t hex_value(char c) CASTLE_NOEXCEPT
{
    if (c >= '0' && c <= '9')
    {
        return static_cast<uint32_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f')
    {
        return static_cast<uint32_t>(c - 'a' + 10);
    }
    return static_cast<uint32_t>(c - 'A' + 10);
}

inline bool is_decimal(char c) CASTLE_NOEXCEPT
{
    return c >= '0' && c <= '9';
}

inline bool is_xml_char(uint32_t codepoint) CASTLE_NOEXCEPT
{
    return codepoint == xml_tab
        || codepoint == xml_line_feed
        || codepoint == xml_carriage_return
        || (codepoint >= xml_char_min && codepoint <= xml_bmp_first_max)
        || (codepoint >= xml_bmp_second_min && codepoint <= xml_bmp_second_max)
        || (codepoint >= xml_supplementary_min && codepoint <= xml_supplementary_max);
}

inline castle::size_type utf8_sequence_length(CASTLE_CONST char* data, castle::size_type remaining) CASTLE_NOEXCEPT
{
    if (remaining == 0U)
    {
        return 0U;
    }
    CASTLE_CONST uint8_t c0 = static_cast<uint8_t>(data[0]);

    if (c0 <= characters::utf8_ascii_max)
    {
        return 1U;
    }
    if (c0 >= characters::utf8_lead2_min && c0 <= characters::utf8_lead2_max)
    {
        return remaining >= 2U ? 2U : 0U;
    }
    if (c0 >= characters::utf8_lead3_min && c0 <= characters::utf8_lead3_max)
    {
        return remaining >= 3U ? 3U : 0U;
    }
    if (c0 >= characters::utf8_lead4_min && c0 <= characters::utf8_lead4_max)
    {
        return remaining >= 4U ? 4U : 0U;
    }
    return 0U;
}

inline bool decode_utf8(CASTLE_CONST char* data, castle::size_type remaining,
                        uint32_t& codepoint, castle::size_type& length) CASTLE_NOEXCEPT
{
    length = utf8_sequence_length(data, remaining);
    if (length == 0U)
    {
        return false;
    }

    CASTLE_CONST uint8_t c0 = static_cast<uint8_t>(data[0]);
    if (length == 1U)
    {
        codepoint = c0;
        return is_xml_char(codepoint);
    }

    for (castle::size_type i = 1U; i < length; ++i)
    {
        CASTLE_CONST uint8_t ci = static_cast<uint8_t>(data[i]);
        if ((ci & characters::utf8_continuation_prefix) != characters::utf8_continuation_prefix)
        {
            return false;
        }
    }

    if (length == 2U)
    {
        codepoint = static_cast<uint32_t>(c0 & characters::utf8_decode_mask_2) << characters::utf8_shift_6;
        codepoint |= static_cast<uint32_t>(static_cast<uint8_t>(data[1]) & characters::utf8_continuation_mask);
    }
    else if (length == 3U)
    {
        codepoint = static_cast<uint32_t>(c0 & characters::utf8_decode_mask_3) << characters::utf8_shift_12;
        codepoint |= static_cast<uint32_t>(static_cast<uint8_t>(data[1]) & characters::utf8_continuation_mask) << characters::utf8_shift_6;
        codepoint |= static_cast<uint32_t>(static_cast<uint8_t>(data[2]) & characters::utf8_continuation_mask);
    }
    else
    {
        codepoint = static_cast<uint32_t>(c0 & characters::utf8_decode_mask_4) << characters::utf8_shift_18;
        codepoint |= static_cast<uint32_t>(static_cast<uint8_t>(data[1]) & characters::utf8_continuation_mask) << characters::utf8_shift_12;
        codepoint |= static_cast<uint32_t>(static_cast<uint8_t>(data[2]) & characters::utf8_continuation_mask) << characters::utf8_shift_6;
        codepoint |= static_cast<uint32_t>(static_cast<uint8_t>(data[3]) & characters::utf8_continuation_mask);
    }

    if (!is_xml_char(codepoint))
    {
        return false;
    }
    if ((length == 3U && codepoint < characters::utf8_min_3_byte_codepoint)
     || (length == 4U && codepoint < characters::utf8_min_4_byte_codepoint))
    {
        return false;
    }
    if (codepoint >= characters::utf16_surrogate_min
     && codepoint <= characters::utf16_surrogate_max)
    {
        return false;
    }
    return true;
}

inline bool append_utf8(uint32_t codepoint, char* output, castle::size_type capacity,
                        castle::size_type& used) CASTLE_NOEXCEPT
{
    if (!is_xml_char(codepoint))
    {
        return false;
    }
    if (codepoint <= characters::utf8_ascii_max)
    {
        if (used >= capacity)
        {
            return false;
        }
        output[used++] = static_cast<char>(codepoint);
        return true;
    }
    if (codepoint <= characters::utf8_two_byte_max)
    {
        if (capacity - used < 2U)
        {
            return false;
        }
        output[used++] = static_cast<char>(characters::utf8_two_byte_prefix | (codepoint >> characters::utf8_shift_6));
        output[used++] = static_cast<char>(characters::utf8_continuation_prefix | (codepoint & characters::utf8_continuation_mask));
        return true;
    }
    if (codepoint <= characters::utf8_three_byte_max)
    {
        if (capacity - used < 3U)
        {
            return false;
        }
        output[used++] = static_cast<char>(characters::utf8_three_byte_prefix | (codepoint >> characters::utf8_shift_12));
        output[used++] = static_cast<char>(characters::utf8_continuation_prefix | ((codepoint >> characters::utf8_shift_6) & characters::utf8_continuation_mask));
        output[used++] = static_cast<char>(characters::utf8_continuation_prefix | (codepoint & characters::utf8_continuation_mask));
        return true;
    }
    if (capacity - used < 4U)
    {
        return false;
    }
    output[used++] = static_cast<char>(characters::utf8_four_byte_prefix | (codepoint >> characters::utf8_shift_18));
    output[used++] = static_cast<char>(characters::utf8_continuation_prefix | ((codepoint >> characters::utf8_shift_12) & characters::utf8_continuation_mask));
    output[used++] = static_cast<char>(characters::utf8_continuation_prefix | ((codepoint >> characters::utf8_shift_6) & characters::utf8_continuation_mask));
    output[used++] = static_cast<char>(characters::utf8_continuation_prefix | (codepoint & characters::utf8_continuation_mask));
    return true;
}

inline bool parse_character_reference(string_view source, castle::size_type& position,
                                      uint32_t& codepoint) CASTLE_NOEXCEPT
{
    if (position >= source.size() || source[position] != '&')
    {
        return false;
    }
    ++position;
    CASTLE_CONST castle::size_type start = position;
    if (position < source.size() && source[position] == '#')
    {
        ++position;
        bool hex = false;
        if (position < source.size() && (source[position] == 'x' || source[position] == 'X'))
        {
            hex = true;
            ++position;
        }
        CASTLE_CONST castle::size_type digits = position;
        uint64_t value = 0U;
        while (position < source.size())
        {
            CASTLE_CONST char c = source[position];
            CASTLE_CONST bool valid = hex ? is_hex(c) : is_decimal(c);
            if (!valid)
            {
                break;
            }

            CASTLE_CONST uint32_t digit = hex ? hex_value(c) : static_cast<uint32_t>(c - '0');
            value = hex ? (value * 16U + digit) : (value * 10U + digit);
            if (value > unicode_max_codepoint)
            {
                return false;
            }
            ++position;
        }
        if (position == digits || position >= source.size() || source[position] != ';')
        {
            return false;
        }
        ++position;
        codepoint = static_cast<uint32_t>(value);
        (void)start;
        return is_xml_char(codepoint);
    }

    while (position < source.size() && is_ascii_name_char(source[position])) ++position;
    if (position == start || position >= source.size() || source[position] != ';')
    {
        return false;
    }

    CASTLE_CONST string_view entity(source.data() + start, position - start);
    ++position;
    if (equal(entity, string_view("amp")))
    {
        codepoint = '&';
        return true;
    }
    if (equal(entity, string_view("lt")))
    {
        codepoint = '<';
        return true;
    }
    if (equal(entity, string_view("gt")))
    {
        codepoint = '>';
        return true;
    }
    if (equal(entity, string_view("quot")))
    {
        codepoint = '"';
        return true;
    }
    if (equal(entity, string_view("apos")))
    {
        codepoint = '\'';
        return true;
    }
    return false;
}

inline bool valid_declaration(string_view data) CASTLE_NOEXCEPT
{
    castle::size_type i = 0U;
    while (i < data.size() && is_space(data[i]))
    {
        ++i;
    }

    CASTLE_CONST castle::size_type version_start = i;
    while (i < data.size() && is_ascii_name_char(data[i]))
    {
        ++i;
    }

    if (!equal(string_view(data.data() + version_start, i - version_start), string_view("version")))
    {
        return false;
    }

    if (i >= data.size() || data[i] != '=')
    {
        return false;
    }

    ++i;

    if (i >= data.size() || (data[i] != '\'' && data[i] != '"'))
    {
        return false;
    }

    CASTLE_CONST char version_quote = data[i++];
    CASTLE_CONST castle::size_type version_value_start = i;
    while (i < data.size() && data[i] != version_quote)
    {
        ++i;
    }

    if (i >= data.size())
    {
        return false;
    }

    CASTLE_CONST string_view version(data.data() + version_value_start, i - version_value_start);
    if (!equal(version, string_view("1.0")) && !equal(version, string_view("1.1")))
    {
        return false;
    }

    ++i;

    bool encoding_seen = false;
    bool standalone_seen = false;
    while (true)
    {
        while (i < data.size() && is_space(data[i]))
        {
            ++i;
        }

        if (i == data.size())
        {
            return true;
        }

        CASTLE_CONST castle::size_type name_start = i;
        while (i < data.size() && is_ascii_name_char(data[i]))
        {
            ++i;
        }

        if (i == name_start)
        {
            return false;
        }

        CASTLE_CONST string_view name(data.data() + name_start, i - name_start);
        if (i >= data.size() || data[i] != '=')
        {
            return false;
        }

        ++i;
        if (i >= data.size() || (data[i] != '\'' && data[i] != '"'))
        {
            return false;
        }

        CASTLE_CONST char quote = data[i++];
        CASTLE_CONST castle::size_type value_start = i;
        while (i < data.size() && data[i] != quote)
        {
            ++i;
        }
        if (i >= data.size())
        {
            return false;
        }

        CASTLE_CONST string_view value(data.data() + value_start, i - value_start);
        ++i;

        if (equal(name, string_view("encoding")))
        {
            if (encoding_seen || value.empty())
            {
                return false;
            }

            encoding_seen = true;
            if (!((value[0U] >= 'A' && value[0U] <= 'Z') || (value[0U] >= 'a' && value[0U] <= 'z')))
            {
                return false;
            }

            for (castle::size_type k = 1U; k < value.size(); ++k)
            {
                if (!is_ascii_name_char(value[k]))
                {
                    return false;
                }
            }
        }
        else if (equal(name, string_view("standalone")))
        {
            if (standalone_seen || (!equal(value, string_view("yes")) && !equal(value, string_view("no"))))
            {
                return false;
            }

            standalone_seen = true;
        }
        else
        {
            return false;
        }
    }
}

inline bool valid_number(string_view text) CASTLE_NOEXCEPT
{
    if (text.empty())
    {
        return false;
    }

    castle::size_type i = 0U;
    if (text[i] == '-')
    {
        ++i;
    }

    if (i == text.size())
    {
        return false;
    }

    if (text[i] == '0')
    {
        ++i;
        if (i < text.size() && is_decimal(text[i]))
        {
            return false;
        }
    }
    else
    {
        if (text[i] < '1' || text[i] > '9')
        {
            return false;
        }
        while (i < text.size() && is_decimal(text[i]))
        {
            ++i;
        }
    }
    if (i < text.size() && text[i] == '.')
    {
        ++i;
        if (i == text.size() || !is_decimal(text[i]))
        {
            return false;
        }
        while (i < text.size() && is_decimal(text[i]))
        {
            ++i;
        }
    }
    if (i < text.size() && (text[i] == 'e' || text[i] == 'E'))
    {
        ++i;
        if (i < text.size() && (text[i] == '+' || text[i] == '-'))
        {
            ++i;
        }
        if (i == text.size() || !is_decimal(text[i]))
        {
            return false;
        }

        while (i < text.size() && is_decimal(text[i]))
        {
            ++i;
        }
    }
    return i == text.size();
}

template <typename T>
castle::status parse_integer(string_view text, T& output) CASTLE_NOEXCEPT
{
    if (text.empty())
    {
        return castle::status::invalid_argument;
    }

    if (text[0U] == '+')
    {
        return castle::status::invalid_argument;
    }

    using U = meta::make_unsigned_t<T>;
    castle::size_type i = 0U;
    bool negative = false;
    if (text[0U] == '-')
    {
        negative = true;
        i = 1U;
    }

    if (i == text.size() || (negative && !meta::is_signed<T>::value))
    {
        return castle::status::invalid_argument;
    }

    uint64_t magnitude = 0U;
    for (; i < text.size(); ++i)
    {
        if (!is_decimal(text[i]))
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST uint64_t digit = static_cast<uint64_t>(text[i] - '0');
        if (magnitude > (castle::numeric_limits<uint64_t>::max() - digit) / 10U)
        {
            return castle::status::out_of_range;
        }

        magnitude = magnitude * 10U + digit;
    }

    CASTLE_CONST uint64_t max_unsigned = static_cast<uint64_t>(castle::numeric_limits<U>::max());
    if (!negative)
    {
        if (magnitude > max_unsigned || (meta::is_signed<T>::value && magnitude > (max_unsigned >> 1U)))
        {
            return castle::status::out_of_range;
        }
        output = static_cast<T>(static_cast<U>(magnitude));
        return castle::status::ok;
    }

    CASTLE_CONST uint64_t minimum_magnitude = (max_unsigned >> 1U) + 1U;
    if (magnitude > minimum_magnitude)
    {
        return castle::status::out_of_range;
    }

    output = magnitude == minimum_magnitude
             ? static_cast<T>(-static_cast<T>(magnitude - 1U) - static_cast<T>(1))
             : static_cast<T>(-static_cast<T>(magnitude));
    return castle::status::ok;
}

template <typename T>
enable_float_t<T>
parse_floating(string_view text, T& output) CASTLE_NOEXCEPT
{
    if (!valid_number(text))
    {
        return castle::status::invalid_argument;
    }

    castle::size_type i = 0U;
    bool negative = false;
    if (text[i] == '-')
    {
        negative = true;
        ++i;
    }

    T value = static_cast<T>(0);
    while (i < text.size() && is_decimal(text[i]))
    {
        value = value * static_cast<T>(10) + static_cast<T>(text[i] - '0');
        ++i;
    }
    if (i < text.size() && text[i] == '.')
    {
        ++i;
        T scale = static_cast<T>(0.1);
        while (i < text.size() && is_decimal(text[i]))
        {
            value += static_cast<T>(text[i] - '0') * scale;
            scale *= static_cast<T>(0.1);
            ++i;
        }
    }
    int exponent = 0;
    bool exponent_negative = false;
    if (i < text.size())
    {
        ++i;
        if (text[i] == '+' || text[i] == '-')
        {
            exponent_negative = text[i] == '-'; ++i;
        }

        while (i < text.size())
        {
            exponent = exponent * 10 + static_cast<int>(text[i] - '0');
            if (exponent > max_floating_exponent)
            {
                return castle::status::out_of_range;
            }
            ++i;
        }
    }

    for (int e = 0; e < exponent; ++e)
    {
        value *= exponent_negative ? static_cast<T>(0.1) : static_cast<T>(10);
    }

    CASTLE_CONST T max_value = castle::numeric_limits<T>::max();
    if (value > max_value)
    {
        return castle::status::out_of_range;
    }

    output = negative ? -value : value;
    return castle::status::ok;
}

} // namespace detail

template <castle::size_type MaxNodes = 128U,
          castle::size_type MaxAttributes = 128U,
          castle::size_type MaxStringBytes = 4096U,
          castle::size_type MaxDepth = 16U>
class document
{
    static_assert(MaxNodes > 0U, "xml::document requires nodes");
    static_assert(MaxAttributes > 0U, "xml::document requires attributes");
    static_assert(MaxStringBytes > 0U, "xml::document requires string storage");
    static_assert(MaxDepth > 0U, "xml::document requires a positive depth");

public:
    using size_type = castle::size_type;
    using view_type = string_view;
    using node_id = size_type;
    using attribute_id = size_type;
    static CASTLE_CONSTEXPR node_id npos = static_cast<node_id>(-1);
    static CASTLE_CONSTEXPR attribute_id attribute_npos = static_cast<attribute_id>(-1);
    static CASTLE_CONSTEXPR size_type static_string_capacity = MaxStringBytes;
    static CASTLE_CONSTEXPR size_type static_max_depth = MaxDepth;

    struct node
    {
        type kind = type::document;
        node_id parent = npos;
        node_id first_child = npos;
        node_id last_child = npos;
        node_id next_sibling = npos;
        attribute_id first_attribute = attribute_npos;
        attribute_id last_attribute = attribute_npos;
        size_type name_offset = 0U;
        size_type name_size = 0U;
        size_type value_offset = 0U;
        size_type value_size = 0U;
    };

    struct attribute
    {
        attribute_id next = attribute_npos;
        size_type name_offset = 0U;
        size_type name_size = 0U;
        size_type value_offset = 0U;
        size_type value_size = 0U;
    };

    document() CASTLE_NOEXCEPT
    {
        clear();
    }

    void clear() CASTLE_NOEXCEPT
    {
        node_count_ = 1U;
        attribute_count_ = 0U;
        used_ = 0U;
        nodes_[0U] = node();
        nodes_[0U].kind = type::document;
    }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ - 1U; }
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxNodes; }
    size_type attribute_size() CASTLE_CONST CASTLE_NOEXCEPT { return attribute_count_; }
    size_type attribute_capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxAttributes; }
    size_type bytes_used() CASTLE_CONST CASTLE_NOEXCEPT { return used_; }
    size_type bytes_capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxStringBytes; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return root() == npos; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size() == MaxNodes; }
    bool attributes_full() CASTLE_CONST CASTLE_NOEXCEPT { return attribute_count_ == MaxAttributes; }

    node_id document_node() CASTLE_CONST CASTLE_NOEXCEPT { return 0U; }

    node_id root() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (node_id child = nodes_[0U].first_child; child != npos; child = nodes_[child].next_sibling)
        {
            if (nodes_[child].kind == type::element)
            {
                return child;
            }
        }
        return npos;
    }

    type kind(node_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid(id) ? nodes_[id].kind : type::document;
    }

    node_id parent(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].parent : npos; }
    node_id first_child(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].first_child : npos; }
    node_id next_sibling(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].next_sibling : npos; }

    view_type name(node_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid(id) ? view(nodes_[id].name_offset, nodes_[id].name_size) : view_type();
    }

    view_type value(node_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid(id) ? view(nodes_[id].value_offset, nodes_[id].value_size) : view_type();
    }

    attribute_id first_attribute(node_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid(id) ? nodes_[id].first_attribute : attribute_npos;
    }

    attribute_id next_attribute(attribute_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid_attribute(id) ? attributes_[id].next : attribute_npos;
    }

    view_type attribute_name(attribute_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid_attribute(id) ? view(attributes_[id].name_offset, attributes_[id].name_size) : view_type();
    }

    view_type attribute_value(attribute_id id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid_attribute(id) ? view(attributes_[id].value_offset, attributes_[id].value_size) : view_type();
    }

    attribute_id find_attribute(node_id element, view_type attribute_name_view) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(element) || kind(element) != type::element || attribute_name_view.empty())
        {
            return attribute_npos;
        }

        for (attribute_id id = first_attribute(element); id != attribute_npos; id = next_attribute(id))
        {
            if (detail::equal(attribute_name(id), attribute_name_view))
            {
                return id;
            }
        }
        return attribute_npos;
    }

    node_id find_child(node_id parent_id, view_type child_name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(parent_id) || child_name.empty())
        {
            return npos;
        }

        for (node_id id = first_child(parent_id); id != npos; id = next_sibling(id))
        {
            if (kind(id) == type::element && detail::equal(name(id), child_name))
            {
                return id;
            }
        }
        return npos;
    }

    castle::status make_element(node_id& out, view_type element_name, node_id parent_id = 0U) CASTLE_NOEXCEPT
    {
        if (!valid(parent_id) || (kind(parent_id) != type::document && kind(parent_id) != type::element))
        {
            return castle::status::invalid_argument;
        }
        if (!valid_element_name(element_name))
        {
            return castle::status::invalid_argument;
        }
        if (would_exceed_depth(parent_id))
        {
            return castle::status::out_of_range;
        }
        return add_node(out, type::element, parent_id, element_name, view_type());
    }

    castle::status append_text(node_id& out, view_type text, node_id parent_id) CASTLE_NOEXCEPT
    {
        if (!valid_text_parent(parent_id))
        {
            return castle::status::invalid_argument;
        }
        if (text.empty())
        {
            return castle::status::invalid_argument;
        }
        if (!valid_xml_string(text))
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::text, parent_id, view_type(), text);
    }

    castle::status append_cdata(node_id& out, view_type text, node_id parent_id) CASTLE_NOEXCEPT
    {
        if (!valid_text_parent(parent_id))
        {
            return castle::status::invalid_argument;
        }
        if (!valid_xml_string(text))
        {
            return castle::status::invalid_argument;
        }
        if (text.find(view_type("]]>")) != view_type::npos)
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::cdata, parent_id, view_type(), text);
    }

    castle::status append_comment(node_id& out, view_type text, node_id parent_id = 0U) CASTLE_NOEXCEPT
    {
        if (!valid_text_parent(parent_id))
        {
            return castle::status::invalid_argument;
        }
        if (!valid_xml_string(text) || !valid_comment(text))
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::comment, parent_id, view_type(), text);
    }

    castle::status append_processing_instruction(node_id& out, view_type target,
                                                 view_type data = view_type(),
                                                 node_id parent_id = 0U) CASTLE_NOEXCEPT
    {
        if (!valid_text_parent(parent_id) || !valid_element_name(target))
        {
            return castle::status::invalid_argument;
        }
        if (detail::equal_ci(target, string_view("xml")))
        {
            return castle::status::invalid_argument;
        }
        if (data.find(view_type("?>")) != view_type::npos || !valid_xml_string(data))
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::processing_instruction, parent_id, target, data);
    }

    castle::status append_declaration(node_id& out, view_type data, node_id parent_id = 0U) CASTLE_NOEXCEPT
    {
        if (!valid(parent_id) || kind(parent_id) != type::document)
        {
            return castle::status::invalid_argument;
        }
        if (has_node_kind(type::declaration))
        {
            return castle::status::already_exists;
        }
        if (root() != npos || nodes_[0U].first_child != npos)
        {
            return castle::status::invalid_argument;
        }
        if (data.find(view_type("?>")) != view_type::npos || !valid_xml_string(data) || data.empty())
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::declaration, parent_id, string_view("xml"), data);
    }

    castle::status append_doctype(node_id& out, view_type data, node_id parent_id = 0U) CASTLE_NOEXCEPT
    {
        if (!valid(parent_id) || kind(parent_id) != type::document)
        {
            return castle::status::invalid_argument;
        }
        if (root() != npos)
        {
            return castle::status::invalid_argument;
        }
        if (has_node_kind(type::doctype))
        {
            return castle::status::already_exists;
        }
        if (data.empty() || !valid_xml_string(data))
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::doctype, parent_id, view_type(), data);
    }

    castle::status add_attribute(attribute_id& out, node_id element,
                                 view_type attribute_name_view, view_type attribute_value_view) CASTLE_NOEXCEPT
    {
        if (!valid(element) || kind(element) != type::element)
        {
            return castle::status::invalid_argument;
        }
        if (!valid_attribute_name(attribute_name_view) || !valid_xml_string(attribute_value_view))
        {
            return castle::status::invalid_argument;
        }
        if (find_attribute(element, attribute_name_view) != attribute_npos)
        {
            return castle::status::already_exists;
        }
        if (attribute_count_ == MaxAttributes)
        {
            return castle::status::full;
        }

        CASTLE_CONST size_type old_used = used_;
        attribute candidate;
        castle::status s = store(attribute_name_view, candidate.name_offset, candidate.name_size);
        if (s != castle::status::ok)
        {
            return s;
        }

        s = store(attribute_value_view, candidate.value_offset, candidate.value_size);
        if (s != castle::status::ok)
        {
            used_ = old_used;
            return s;
        }

        out = attribute_count_++;
        attributes_[out] = candidate;
        if (nodes_[element].first_attribute == attribute_npos)
        {
            nodes_[element].first_attribute = out;
        }
        else
        {
            attributes_[nodes_[element].last_attribute].next = out;
        }
        nodes_[element].last_attribute = out;
        return castle::status::ok;
    }

    castle::status set_attribute(node_id element, view_type attribute_name_view, view_type attribute_value_view) CASTLE_NOEXCEPT
    {
        if (!valid(element)
         || kind(element) != type::element
         || !valid_attribute_name(attribute_name_view)
         || !valid_xml_string(attribute_value_view))
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST attribute_id existing = find_attribute(element, attribute_name_view);
        if (existing == attribute_npos)
        {
            attribute_id unused = attribute_npos;
            return add_attribute(unused, element, attribute_name_view, attribute_value_view);
        }

        size_type offset = 0U;
        size_type length = 0U;
        CASTLE_CONST size_type old_used = used_;
        CASTLE_CONST castle::status s = store(attribute_value_view, offset, length);
        if (s != castle::status::ok)
        {
            used_ = old_used;
            return s;
        }

        attributes_[existing].value_offset = offset;
        attributes_[existing].value_size = length;
        return castle::status::ok;
    }

    castle::status set_text(node_id element, view_type text) CASTLE_NOEXCEPT
    {
        if (!valid(element)
         || kind(element) != type::element
         || !valid_xml_string(text))
        {
            return castle::status::invalid_argument;
        }

        for (node_id id = first_child(element); id != npos; id = next_sibling(id))
        {
            if (kind(id) == type::text)
            {
                size_type offset = 0U;
                size_type length = 0U;
                CASTLE_CONST size_type old_used = used_;
                CASTLE_CONST castle::status s = store(text, offset, length);
                if (s != castle::status::ok)
                {
                    used_ = old_used;
                    return s;
                }
                nodes_[id].value_offset = offset;
                nodes_[id].value_size = length;
                return castle::status::ok;
            }
        }
        node_id unused = npos;
        return append_text(unused, text, element);
    }

    template <typename T>
    detail::enable_integer_t<T> set_text(node_id element, T value) CASTLE_NOEXCEPT
    {
        castle::string_builder<max_integer_str_len> builder;
        if (builder.append(value).truncated())
        {
            return castle::status::full;
        }
        return set_text(element, builder.view());
    }

    castle::status set_text(node_id element, bool value) CASTLE_NOEXCEPT
    {
        return set_text(element, value ? string_view("true") : string_view("false"));
    }

    template <typename T>
    detail::enable_float_t<T> set_text(node_id element, T value, size_type precision = 6U) CASTLE_NOEXCEPT
    {
        if (value != value)
        {
            return castle::status::invalid_argument;
        }

        castle::string_builder<max_floating_str_len> builder;
        builder.append(value, precision);
        if (builder.truncated()
         || detail::equal(builder.view(), string_view("inf"))
         || detail::equal(builder.view(), string_view("-inf"))
         || detail::equal(builder.view(), string_view("nan")))
        {
            return castle::status::invalid_argument;
        }
        return set_text(element, builder.view());
    }

    template <typename T>
    detail::enable_enum_t<T> set_text(node_id element, T value) CASTLE_NOEXCEPT
    {
        return set_text(element, static_cast<meta::underlying_type_t<T>>(value));
    }

    castle::status get_attribute(node_id element, view_type attribute_name_view, view_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST attribute_id id = find_attribute(element, attribute_name_view);
        if (id == attribute_npos)
        {
            out = view_type();
            return castle::status::not_found;
        }

        out = attribute_value(id);
        return castle::status::ok;
    }

    castle::status get_text(node_id element, view_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(element) || kind(element) != type::element)
        {
            out = view_type();
            return castle::status::invalid_argument;
        }

        node_id match = npos;
        for (node_id id = first_child(element); id != npos; id = next_sibling(id))
        {
            if (kind(id) == type::text || kind(id) == type::cdata)
            {
                if (match != npos)
                {
                    out = view_type();
                    return castle::status::data_loss;
                }
                match = id;
            }
        }

        if (match == npos)
        {
            out = view_type();
            return castle::status::not_found;
        }
        out = value(match);
        return castle::status::ok;
    }

    template <typename T>
    detail::enable_integer_t<T> get_attribute(node_id element, view_type attribute_name_view, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status s = get_attribute(element, attribute_name_view, text);
        return s == castle::status::ok ? detail::parse_integer(text, output) : s;
    }

    template <typename T>
    detail::enable_float_t<T> get_attribute(node_id element, view_type attribute_name_view, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status s = get_attribute(element, attribute_name_view, text);
        return s == castle::status::ok ? detail::parse_floating(text, output) : s;
    }

    castle::status get_attribute(node_id element, view_type attribute_name_view, bool& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status s = get_attribute(element, attribute_name_view, text);
        if (s != castle::status::ok)
        {
            return s;
        }

        if (detail::equal(text, string_view("true")) || detail::equal(text, string_view("1")))
        {
            output = true; return castle::status::ok;
        }

        if (detail::equal(text, string_view("false")) || detail::equal(text, string_view("0")))
        {
            output = false;
            return castle::status::ok;
        }

        return castle::status::invalid_argument;
    }

    template <typename T>
    detail::enable_integer_t<T> get_text(node_id element, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status s = get_text(element, text);
        return s == castle::status::ok ? detail::parse_integer(text, output) : s;
    }

    template <typename T>
    detail::enable_float_t<T> get_text(node_id element, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status s = get_text(element, text);
        return s == castle::status::ok ? detail::parse_floating(text, output) : s;
    }

    castle::status get_text(node_id element, bool& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status s = get_text(element, text);
        if (s != castle::status::ok)
        {
            return s;
        }
        if (detail::equal(text, string_view("true")) || detail::equal(text, string_view("1")))
        {
            output = true;
            return castle::status::ok;
        }
        if (detail::equal(text, string_view("false")) || detail::equal(text, string_view("0")))
        {
            output = false;
            return castle::status::ok;
        }
        return castle::status::invalid_argument;
    }

    template <typename T>
    detail::enable_enum_t<T> get_attribute(node_id element, view_type attribute_name_view, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        using U = meta::underlying_type_t<T>;
        U value = 0;
        CASTLE_CONST castle::status s = get_attribute(element, attribute_name_view, value);
        if (s == castle::status::ok)
        {
            output = static_cast<T>(value);
        }
        return s;
    }

    template <typename T>
    detail::enable_enum_t<T> get_text(node_id element, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        using U = meta::underlying_type_t<T>;
        U value = 0;
        CASTLE_CONST castle::status s = get_text(element, value);
        if (s == castle::status::ok)
        {
            output = static_cast<T>(value);
        }
        return s;
    }

    template <typename T>
    detail::enable_integer_t<T> set_attribute(node_id element, view_type attribute_name_view, T value) CASTLE_NOEXCEPT
    {
        castle::container::string<max_integer_str_len> encoded;
        castle::string_builder<max_integer_str_len> builder;
        if (builder.append(value).truncated())
        {
            return castle::status::full;
        }
        encoded = builder.view();
        return set_attribute(element, attribute_name_view, encoded.view());
    }

    template <typename T>
    detail::enable_enum_t<T> set_attribute(node_id element, view_type attribute_name_view, T value) CASTLE_NOEXCEPT
    {
        return set_attribute(element, attribute_name_view, static_cast<meta::underlying_type_t<T>>(value));
    }

    castle::status set_attribute(node_id element, view_type attribute_name_view, bool value) CASTLE_NOEXCEPT
    {
        return set_attribute(element, attribute_name_view, value ? string_view("true") : string_view("false"));
    }

    template <typename T>
    detail::enable_float_t<T> set_attribute(node_id element, view_type attribute_name_view, T value, size_type precision = 6U) CASTLE_NOEXCEPT
    {
        if (value != value)
        {
            return castle::status::invalid_argument;
        }
        castle::string_builder<max_floating_str_len> builder;
        builder.append(value, precision);
        if (builder.truncated()
         || detail::equal(builder.view(), string_view("inf"))
         || detail::equal(builder.view(), string_view("-inf"))
         || detail::equal(builder.view(), string_view("nan")))
        {
            return castle::status::invalid_argument;
        }
        return set_attribute(element, attribute_name_view, builder.view());
    }

    castle::status parse(view_type source) CASTLE_NOEXCEPT;

private:
    template <typename> friend class detail::parser;

    bool valid(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return id < node_count_; }
    bool valid_attribute(attribute_id id) CASTLE_CONST CASTLE_NOEXCEPT { return id < attribute_count_; }

    view_type view(size_type offset, size_type length) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return length == 0U ? view_type() : view_type(storage_ + offset, length);
    }

    static bool valid_element_name(view_type text) CASTLE_NOEXCEPT
    {
        if (text.empty() || !detail::is_ascii_name_start(text[0U]))
        {
            return false;
        }

        for (size_type i = 1U; i < text.size(); ++i)
        {
            if (!detail::is_ascii_name_char(text[i]))
            {
                return false;
            }
        }
        return true;
    }

    static bool valid_attribute_name(view_type text) CASTLE_NOEXCEPT
    {
        return valid_element_name(text);
    }

    static bool valid_xml_string(view_type text) CASTLE_NOEXCEPT
    {
        size_type i = 0U;
        while (i < text.size())
        {
            uint32_t codepoint = 0U;
            size_type length = 0U;
            if (!detail::decode_utf8(text.data() + i, text.size() - i, codepoint, length))
            {
                return false;
            }
            i += length;
        }
        return true;
    }

    static bool valid_comment(view_type text) CASTLE_NOEXCEPT
    {
        if (text.find(view_type("--")) != view_type::npos)
        {
            return false;
        }
        return text.empty() || text.back() != '-';
    }

    bool valid_text_parent(node_id parent_id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return valid(parent_id) && (kind(parent_id) == type::document || kind(parent_id) == type::element);
    }

    bool would_exceed_depth(node_id parent_id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type depth = 0U;
        node_id id = parent_id;
        while (id != 0U && id != npos)
        {
            ++depth;
            id = nodes_[id].parent;
            if (depth >= MaxDepth)
            {
                return true;
            }
        }
        return false;
    }

    bool has_node_kind(type value_type) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (node_id id = 1U; id < node_count_; ++id)
        {
            if (nodes_[id].kind == value_type)
            {
                return true;
            }
        }
        return false;
    }

    castle::status store(view_type source, size_type& offset, size_type& length) CASTLE_NOEXCEPT
    {
        if (source.size() > MaxStringBytes - used_)
        {
            return castle::status::full;
        }

        offset = used_;
        length = source.size();
        for (size_type i = 0U; i < source.size(); ++i)
        {
            storage_[used_ + i] = source[i];
        }

        used_ += source.size();
        return castle::status::ok;
    }

    void rollback_used(size_type value) CASTLE_NOEXCEPT
    {
        if (value <= used_)
        {
            used_ = value;
        }
    }

    castle::status append_codepoint(uint32_t codepoint) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type old = used_;
        if (!detail::append_utf8(codepoint, storage_, MaxStringBytes, used_))
        {
            used_ = old;
            return castle::status::full;
        }
        return castle::status::ok;
    }

    castle::status add_node(node_id& out, type node_kind, node_id parent_id,
                            view_type node_name, view_type node_value) CASTLE_NOEXCEPT
    {
        if (!valid(parent_id) ||
            (kind(parent_id) != type::document && kind(parent_id) != type::element))
        {
            return castle::status::invalid_argument;
        }

        if (node_count_ == MaxNodes + 1U)
        {
            return castle::status::full;
        }

        if (node_kind == type::element && would_exceed_depth(parent_id))
        {
            return castle::status::out_of_range;
        }

        CASTLE_CONST size_type old_used = used_;
        node candidate;
        candidate.kind = node_kind;
        candidate.parent = parent_id;
        castle::status s = store(node_name, candidate.name_offset, candidate.name_size);
        if (s != castle::status::ok)
        {
            return s;
        }

        s = store(node_value, candidate.value_offset, candidate.value_size);
        if (s != castle::status::ok)
        {
            used_ = old_used;
            return s;
        }

        out = node_count_++;
        nodes_[out] = candidate;
        if (nodes_[parent_id].first_child == npos)
        {
            nodes_[parent_id].first_child = out;
        }
        else
        {
            nodes_[nodes_[parent_id].last_child].next_sibling = out;
        }
        nodes_[parent_id].last_child = out;
        return castle::status::ok;
    }

    node nodes_[MaxNodes + 1U];
    attribute attributes_[MaxAttributes];
    size_type node_count_ = 1U;
    size_type attribute_count_ = 0U;
    char storage_[MaxStringBytes] = {};
    size_type used_ = 0U;
};

namespace detail
{

template <typename Document>
class parser
{
public:
    using size_type = castle::size_type;
    using node_id = typename Document::node_id;
    using view_type = typename Document::view_type;

    parser(Document& document, view_type source) CASTLE_NOEXCEPT
        : doc_(document), source_(source), position_(0U), depth_(0U), root_seen_(false), declaration_seen_(false), doctype_seen_(false), decode_error_(error_code::invalid_entity) {}

    result run() CASTLE_NOEXCEPT
    {
        doc_.clear();
        if (source_.size() >= 3U
         && static_cast<uint8_t>(source_[0U]) == 0xEFU
         && static_cast<uint8_t>(source_[1U]) == 0xBBU
         && static_cast<uint8_t>(source_[2U]) == 0xBFU)
        {
            position_ = 3U;
        }

        while (position_ < source_.size())
        {
            if (depth_ == 0U
             && !root_seen_
             && !declaration_seen_
             && position_ == (source_.size() >= 3U
             && static_cast<uint8_t>(source_[0U]) == 0xEFU
             && static_cast<uint8_t>(source_[1U]) == 0xBBU
             && static_cast<uint8_t>(source_[2U]) == 0xBFU ? 3U : 0U)
             && position_ + 6U <= source_.size()
             && source_[position_] == '<' && source_[position_ + 1U] == '?'
             && source_[position_ + 2U] == 'x' && source_[position_ + 3U] == 'm' && source_[position_ + 4U] == 'l'
             && detail::is_space(source_[position_ + 5U]))
            {
                result r = parse_declaration();
                if (!r.succeeded())
                {
                    return fail_and_clear(r);
                }
                continue;
            }

            if (source_[position_] == '<')
            {
                result r = parse_markup();
                if (!r.succeeded())
                {
                    return fail_and_clear(r);
                }
                continue;
            }

            CASTLE_CONST size_type start = position_;
            while (position_ < source_.size() && source_[position_] != '<')
            {
                ++position_;
            }
            result r = parse_text(start, position_ - start);
            if (!r.succeeded())
            {
                return fail_and_clear(r);
            }
        }

        if (depth_ != 0U)
        {
            return fail_and_clear(fail(error_code::unexpected_end, position_));
        }
        if (!root_seen_)
        {
            return fail_and_clear(fail(error_code::unexpected_token, position_));
        }
        return result();
    }

private:
    result fail(error_code code, size_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        result r;
        r.status = castle::status::invalid_argument;
        r.code = code;
        r.offset = offset;
        r.line = 1U;
        r.column = 1U;
        for (size_type i = 0U; i < offset && i < source_.size(); ++i)
        {
            if (source_[i] == '\n')
            {
                ++r.line;
                r.column = 1U;
            }
            else
            {
                ++r.column;
            }
        }
        return r;
    }

    result fail(error_code code) CASTLE_CONST CASTLE_NOEXCEPT { return fail(code, position_); }

    result fail_and_clear(result r) CASTLE_NOEXCEPT
    {
        doc_.clear();
        return r;
    }

    bool at(size_type count, CASTLE_CONST char* text) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type i = 0U;
        while (i < count && position_ + i < source_.size() && source_[position_ + i] == text[i])
        {
            ++i;
        }

        return i == count;
    }

    void skip_space() CASTLE_NOEXCEPT
    {
        while (position_ < source_.size() && detail::is_space(source_[position_]))
        {
            ++position_;
        }
    }

    bool parse_name_view(view_type& out) CASTLE_NOEXCEPT
    {
        if (position_ >= source_.size() || !detail::is_ascii_name_start(source_[position_]))
        {
            return false;
        }

        CASTLE_CONST size_type start = position_++;
        while (position_ < source_.size() && detail::is_ascii_name_char(source_[position_]))
        {
            ++position_;
        }

        out = view_type(source_.data() + start, position_ - start);
        return true;
    }

    result parse_markup() CASTLE_NOEXCEPT
    {
        if (at(4U, "<!--"))      return parse_comment();
        if (at(9U, "<![CDATA[")) return parse_cdata();
        if (at(9U, "<!DOCTYPE")) return parse_doctype();
        if (at(2U, "<?"))        return parse_processing_instruction();
        if (at(2U, "</"))        return parse_end_element();
        return parse_start_element();
    }

    result parse_start_element() CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type tag_offset = position_;
        ++position_;
        view_type element_name;
        if (!parse_name_view(element_name))
        {
            return fail(error_code::malformed_name, position_);
        }

        CASTLE_CONST bool at_document_level = depth_ == 0U;
        if (at_document_level)
        {
            if (root_seen_)
            {
                return fail(error_code::unexpected_token, tag_offset);
            }
            root_seen_ = true;
        }

        CASTLE_CONST node_id parent = depth_ == 0U ? doc_.document_node() : stack_[depth_ - 1U];
        node_id element = Document::npos;
        CASTLE_CONST castle::status s = doc_.make_element(element, element_name, parent);
        if (s == castle::status::out_of_range)
        {
            return fail(error_code::depth_exceeded);
        }
        if (s != castle::status::ok)
        {
            return capacity_result(s);
        }

        if (position_ < source_.size()
         && source_[position_] != '>'
         && !(source_[position_] == '/'
         && position_ + 1U < source_.size()
         && source_[position_ + 1U] == '>')
         && !detail::is_space(source_[position_]))
        {
            return fail(error_code::invalid_attribute);
        }

        skip_space();
        while (position_ < source_.size()
            && source_[position_] != '>'
            && !(source_[position_] == '/'
            && position_ + 1U < source_.size()
            && source_[position_ + 1U] == '>'))
        {
            view_type attribute_name;
            if (!parse_name_view(attribute_name))
            {
                return fail(error_code::malformed_name);
            }

            if (doc_.find_attribute(element, attribute_name) != Document::attribute_npos)
            {
                return fail(error_code::duplicate_attribute);
            }

            skip_space();
            if (position_ >= source_.size() || source_[position_] != '=')
            {
                return fail(error_code::invalid_attribute);
            }

            ++position_;
            skip_space();
            if (position_ >= source_.size() || (source_[position_] != '\'' && source_[position_] != '"'))
            {
                return fail(error_code::invalid_attribute);
            }

            CASTLE_CONST char quote = source_[position_++];
            CASTLE_CONST size_type value_start = position_;
            while (position_ < source_.size() && source_[position_] != quote)
            {
                ++position_;
            }

            if (position_ >= source_.size())
            {
                return fail(error_code::unexpected_end);
            }

            CASTLE_CONST size_type value_end = position_;

            typename Document::attribute_id unused = Document::attribute_npos;
            result ar = append_attribute_from_source(element, attribute_name, value_start, value_end - value_start, unused);
            if (!ar.succeeded())
            {
                return ar;
            }

            ++position_;
            CASTLE_CONST size_type separator_start = position_;
            skip_space();
            if (position_ < source_.size()
             && source_[position_] != '>'
             && !(source_[position_] == '/'
             && position_ + 1U < source_.size()
             && source_[position_ + 1U] == '>')
             && separator_start == position_)
            {
                return fail(error_code::invalid_attribute);
            }
        }

        bool self_closing = false;
        if (position_ < source_.size() && source_[position_] == '/')
        {
            self_closing = true;
            ++position_;
            if (position_ >= source_.size() || source_[position_] != '>')
            {
                return fail(error_code::unexpected_token);
            }
        }
        if (position_ >= source_.size() || source_[position_] != '>')
        {
            return fail(error_code::unexpected_end);
        }
        ++position_;

        if (!self_closing)
        {
            if (depth_ == Document::static_max_depth)
            {
                return fail(error_code::depth_exceeded);
            }
            stack_[depth_++] = element;
        }
        return result();
    }

    result parse_end_element() CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type start = position_;
        position_ += 2U;
        if (depth_ == 0U)
        {
            return fail(error_code::mismatched_tag, start);
        }
        view_type closing_name;
        if (!parse_name_view(closing_name))
        {
            return fail(error_code::malformed_name);
        }
        skip_space();
        if (position_ >= source_.size() || source_[position_] != '>')
        {
            return fail(error_code::unexpected_end);
        }
        if (!detail::equal(doc_.name(stack_[depth_ - 1U]), closing_name))
        {
            return fail(error_code::mismatched_tag, start);
        }
        ++position_;
        --depth_;
        return result();
    }

    result parse_text(size_type start, size_type length) CASTLE_NOEXCEPT
    {
        if (length == 0U)
        {
            return result();
        }
        CASTLE_CONST bool at_document_level = depth_ == 0U;
        if (at_document_level)
        {
            for (size_type i = start; i < start + length; ++i)
            {
                if (!detail::is_space(source_[i]))
                {
                    return fail(error_code::unexpected_token, i);
                }
            }
            return result();
        }

        size_type offset = 0U;
        size_type used = 0U;
        CASTLE_CONST castle::status s = decode_into_storage(start, length, offset, used);
        if (s != castle::status::ok)
        {
            if (s == castle::status::invalid_argument)
            {
                return fail(decode_error_, start);
            }
            return capacity_result(s);
        }
        if (used == 0U)
        {
            return result();
        }
        node_id unused = Document::npos;
        CASTLE_CONST castle::status ns = doc_.add_node(unused, type::text, stack_[depth_ - 1U], view_type(), doc_.view(offset, used));
        if (ns != castle::status::ok)
        {
            doc_.rollback_used(offset);
            return capacity_result(ns);
        }
        return result();
    }

    result parse_comment() CASTLE_NOEXCEPT
    {
        position_ += 4U;
        CASTLE_CONST size_type start = position_;
        while (position_ + 2U < source_.size()
            && !(source_[position_] == '-'
            && source_[position_ + 1U] == '-'
            && source_[position_ + 2U] == '>'))
        {
            ++position_;
        }

        if (position_ + 2U >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }

        CASTLE_CONST size_type length = position_ - start;
        CASTLE_CONST view_type value(source_.data() + start, length);
        if (!Document::valid_xml_string(value) || !Document::valid_comment(value))
        {
            return fail(error_code::invalid_comment, start);
        }

        node_id parent = depth_ == 0U ? doc_.document_node() : stack_[depth_ - 1U];
        node_id unused = Document::npos;
        CASTLE_CONST castle::status s = doc_.add_node(unused, type::comment, parent, view_type(), value);
        if (s != castle::status::ok)
        {
            return capacity_result(s);
        }

        position_ += 3U;
        return result();
    }

    result parse_cdata() CASTLE_NOEXCEPT
    {
        if (depth_ == 0U)
        {
            return fail(error_code::unexpected_token);
        }

        position_ += 9U;
        CASTLE_CONST size_type start = position_;
        while (position_ + 2U < source_.size()
            && !(source_[position_] == ']'
            && source_[position_ + 1U] == ']'
            && source_[position_ + 2U] == '>'))
        {
            ++position_;
        }
        if (position_ + 2U >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }
        CASTLE_CONST size_type length = position_ - start;
        CASTLE_CONST view_type value(source_.data() + start, length);
        if (!Document::valid_xml_string(value))
        {
            return fail(error_code::invalid_cdata, start);
        }
        node_id unused = Document::npos;
        CASTLE_CONST castle::status s = doc_.add_node(unused, type::cdata, stack_[depth_ - 1U], view_type(), value);
        if (s != castle::status::ok)
        {
            return capacity_result(s);
        }
        position_ += 3U;
        return result();
    }

    result parse_processing_instruction() CASTLE_NOEXCEPT
    {
        position_ += 2U;
        CASTLE_CONST size_type target_offset = position_;
        view_type target;
        if (!parse_name_view(target))
        {
            return fail(error_code::invalid_processing_instruction);
        }
        if (detail::equal_ci(target, string_view("xml")))
        {
            return fail(error_code::invalid_declaration, target_offset);
        }
        if (position_ >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }
        if (source_[position_] != '?' && !detail::is_space(source_[position_]))
        {
            return fail(error_code::invalid_processing_instruction);
        }
        skip_space();
        CASTLE_CONST size_type data_start = position_;
        while (position_ + 1U < source_.size() && !(source_[position_] == '?' && source_[position_ + 1U] == '>'))
        {
            ++position_;
        }

        if (position_ + 1U >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }
        CASTLE_CONST view_type data(source_.data() + data_start, position_ - data_start);
        if (!Document::valid_xml_string(data))
        {
            return fail(error_code::invalid_processing_instruction, data_start);
        }
        CASTLE_CONST node_id parent = depth_ == 0U ? doc_.document_node() : stack_[depth_ - 1U];
        node_id unused = Document::npos;
        CASTLE_CONST castle::status s = doc_.add_node(unused, type::processing_instruction, parent, target, data);
        if (s != castle::status::ok)
        {
            return capacity_result(s);
        }
        position_ += 2U;
        return result();
    }

    result parse_declaration() CASTLE_NOEXCEPT
    {
        position_ += 5U;
        if (position_ >= source_.size() || !detail::is_space(source_[position_]))
        {
            return fail(error_code::invalid_declaration);
        }

        ++position_;
        CASTLE_CONST size_type start = position_;
        while (position_ + 1U < source_.size() && !(source_[position_] == '?' && source_[position_ + 1U] == '>'))
        {
            ++position_;
        }
        if (position_ + 1U >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }
        CASTLE_CONST view_type data(source_.data() + start, position_ - start);
        if (!Document::valid_xml_string(data) || data.empty() || !detail::valid_declaration(data))
        {
            return fail(error_code::invalid_declaration, start);
        }
        node_id unused = Document::npos;
        CASTLE_CONST castle::status s = doc_.add_node(unused, type::declaration, doc_.document_node(), string_view("xml"), data);
        if (s != castle::status::ok)
        {
            return capacity_result(s);
        }
        declaration_seen_ = true;
        position_ += 2U;
        return result();
    }

    result parse_doctype() CASTLE_NOEXCEPT
    {
        if (depth_ != 0U || root_seen_ || doctype_seen_)
        {
            return fail(error_code::invalid_doctype);
        }

        position_ += 9U;
        if (position_ >= source_.size() || !detail::is_space(source_[position_]))
        {
            return fail(error_code::invalid_doctype);
        }

        CASTLE_CONST size_type start = position_;
        char quote = 0;
        size_type subset_depth = 0U;
        while (position_ < source_.size())
        {
            CASTLE_CONST char c = source_[position_];
            if (quote != 0)
            {
                if (c == quote)
                {
                    quote = 0;
                }
                ++position_;
                continue;
            }
            if (c == '\'' || c == '"')
            {
                quote = c;
                ++position_;
                continue;
            }
            if (c == '[')
            {
                ++subset_depth;
                ++position_;
                continue;
            }
            if (c == ']' && subset_depth != 0U)
            {
                --subset_depth;
                ++position_;
                continue;
            }
            if (c == '>' && subset_depth == 0U)
            {
                break;
            }
            ++position_;
        }

        if (position_ >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }

        CASTLE_CONST view_type value(source_.data() + start, position_ - start);
        if (value.empty() || !Document::valid_xml_string(value))
        {
            return fail(error_code::invalid_doctype, start);
        }

        node_id unused = Document::npos;
        CASTLE_CONST castle::status s = doc_.add_node(unused, type::doctype, doc_.document_node(), view_type(), value);
        if (s != castle::status::ok)
        {
            return capacity_result(s);
        }
        doctype_seen_ = true;
        ++position_;
        return result();
    }

    result append_attribute_from_source(node_id element, view_type attribute_name,
                                        size_type value_start, size_type value_length,
                                        typename Document::attribute_id& out) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type old_used = doc_.used_;
        size_type value_offset = 0U;
        size_type value_size = 0U;
        CASTLE_CONST castle::status s = decode_into_storage(value_start, value_length, value_offset, value_size);
        if (s != castle::status::ok)
        {
            if (s == castle::status::invalid_argument)
            {
                return fail(decode_error_, value_start);
            }
            return capacity_result(s);
        }
        CASTLE_CONST view_type decoded(doc_.storage_ + value_offset, value_size);
        CASTLE_CONST castle::status as = doc_.add_attribute(out, element, attribute_name, decoded);
        if (as != castle::status::ok)
        {
            doc_.used_ = old_used;
            return capacity_result(as);
        }
        return result();
    }

    castle::status decode_into_storage(size_type start, size_type length,
                                       size_type& offset, size_type& used) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type old_used = doc_.used_;
        offset = old_used;
        size_type position = start;
        CASTLE_CONST size_type end = start + length;
        while (position < end)
        {
            CASTLE_CONST char c = source_[position];
            if (c == '&')
            {
                uint32_t codepoint = 0U;
                size_type entity_position = position;
                if (!detail::parse_character_reference(source_, entity_position, codepoint) || entity_position > end)
                {
                    doc_.used_ = old_used;
                    decode_error_ = error_code::invalid_entity;
                    return castle::status::invalid_argument;
                }
                position = entity_position;
                if (doc_.append_codepoint(codepoint) != castle::status::ok)
                {
                    doc_.used_ = old_used;
                    return castle::status::full;
                }
                continue;
            }
            if (c == '\r')
            {
                if (position + 1U < end && source_[position + 1U] == '\n')
                {
                    ++position;
                }
                ++position;
                if (doc_.append_codepoint('\n') != castle::status::ok)
                {
                    doc_.used_ = old_used;
                    return castle::status::full;
                }
                continue;
            }
            uint32_t codepoint = 0U;
            size_type utf8_length = 0U;
            if (!detail::decode_utf8(source_.data() + position, end - position, codepoint, utf8_length))
            {
                doc_.used_ = old_used;
                decode_error_ = error_code::invalid_character;
                return castle::status::invalid_argument;
            }
            if (doc_.used_ + utf8_length > Document::static_string_capacity)
            {
                doc_.used_ = old_used;
                return castle::status::full;
            }
            for (size_type i = 0U; i < utf8_length; ++i)
            {
                doc_.storage_[doc_.used_ + i] = source_[position + i];
            }
            doc_.used_ += utf8_length;
            position += utf8_length;
        }
        used = doc_.used_ - old_used;
        return castle::status::ok;
    }

    result capacity_result(castle::status s) CASTLE_CONST CASTLE_NOEXCEPT
    {
        result r;
        r.status = s;
        r.code = error_code::capacity;
        r.offset = position_;
        r.line = 1U;
        r.column = 1U;
        for (size_type i = 0U; i < position_ && i < source_.size(); ++i)
        {
            if (source_[i] == '\n')
            {
                ++r.line;
                r.column = 1U;
            }
            else
            {
                ++r.column;
            }
        }
        return r;
    }

    Document& doc_;
    view_type source_;
    size_type position_;
    size_type depth_;
    node_id stack_[Document::static_max_depth] = {};
    bool root_seen_;
    bool declaration_seen_;
    bool doctype_seen_;
    error_code decode_error_;
};

} // namespace detail

template <castle::size_type MaxNodes, castle::size_type MaxAttributes, castle::size_type MaxStringBytes, castle::size_type MaxDepth>
result parse(document<MaxNodes, MaxAttributes, MaxStringBytes, MaxDepth>& doc, string_view source) CASTLE_NOEXCEPT
{
    detail::parser<document<MaxNodes, MaxAttributes, MaxStringBytes, MaxDepth>> parser_instance(doc, source);
    return parser_instance.run();
}

template <castle::size_type MaxNodes, castle::size_type MaxAttributes, castle::size_type MaxStringBytes, castle::size_type MaxDepth>
castle::status document<MaxNodes, MaxAttributes, MaxStringBytes, MaxDepth>::parse(view_type source) CASTLE_NOEXCEPT
{
    return xml::parse(*this, source).status;
}

namespace detail
{

template <typename OutputString>
castle::status append_escaped(OutputString& output, string_view text, bool attribute) CASTLE_NOEXCEPT
{
    for (castle::size_type i = 0U; i < text.size(); ++i)
    {
        CASTLE_CONST unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '&')
        {
            if (output.append(string_view("&amp;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (c == '<')
        {
            if (output.append(string_view("&lt;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (c == '>')
        {
            if (output.append(string_view("&gt;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (attribute && c == '"')
        {
            if (output.append(string_view("&quot;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (attribute && c == '\'')
        {
            if (output.append(string_view("&apos;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (attribute && c == '\n')
        {
            if (output.append(string_view("&#10;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (attribute && c == '\r')
        {
            if (output.append(string_view("&#13;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (attribute && c == '\t')
        {
            if (output.append(string_view("&#9;")) != castle::status::ok)
            {
                return castle::status::full;
            }
            continue;
        }
        if (output.push_back(text[i]) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return castle::status::ok;
}

template <typename Document, typename OutputString>
castle::status write_indent(OutputString& output, castle::size_type depth, castle::size_type spaces) CASTLE_NOEXCEPT
{
    for (castle::size_type i = 0U; i < depth * spaces; ++i)
    {
        if (output.push_back(' ') != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return castle::status::ok;
}

template <typename Document>
bool has_mixed_text(Document CASTLE_CONST& doc, typename Document::node_id id) CASTLE_NOEXCEPT
{
    for (typename Document::node_id child = doc.first_child(id); child != Document::npos; child = doc.next_sibling(child))
    {
        if (doc.kind(child) == type::text || doc.kind(child) == type::cdata)
        {
            return true;
        }
    }
    return false;
}

template <typename Document, typename OutputString>
castle::status serialize_node(Document CASTLE_CONST& doc, typename Document::node_id id,
                              OutputString& output, bool pretty,
                              castle::size_type depth, castle::size_type spaces) CASTLE_NOEXCEPT
{
    CASTLE_CONST type kind = doc.kind(id);
    if (kind == type::text)
    {
        return append_escaped(output, doc.value(id), false);
    }

    if (kind == type::cdata)
    {
        if (output.append(string_view("<![CDATA[")) != castle::status::ok)
        {
            return castle::status::full;
        }

        string_view text = doc.value(id);
        castle::size_type start = 0U;
        while (start < text.size())
        {
            CASTLE_CONST castle::size_type marker = text.find(string_view("]]>") , start);
            if (marker == string_view::npos)
            {
                if (output.append(text.substr(start)) != castle::status::ok)
                {
                    return castle::status::full;
                }
                break;
            }
            if (output.append(text.substr(start, marker - start)) != castle::status::ok)
            {
                return castle::status::full;
            }
            if (output.append(string_view("]]]]><![CDATA[>")) != castle::status::ok)
            {
                return castle::status::full;
            }
            start = marker + 3U;
        }
        return output.append(string_view("]]>") );
    }
    if (kind == type::comment)
    {
        if (output.append(string_view("<!--")) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(doc.value(id)) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(string_view("-->")) != castle::status::ok)
        {
            return castle::status::full;
        }
        return castle::status::ok;
    }
    if (kind == type::processing_instruction)
    {
        if (output.append(string_view("<?")) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(doc.name(id)) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (!doc.value(id).empty())
        {
            if (output.push_back(' ') != castle::status::ok)
            {
                return castle::status::full;
            }
            if (output.append(doc.value(id)) != castle::status::ok)
            {
                return castle::status::full;
            }
        }
        if (output.append(string_view("?>")) != castle::status::ok)
        {
            return castle::status::full;
        }
        return castle::status::ok;
    }
    if (kind == type::declaration)
    {
        if (output.append(string_view("<?xml")) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.push_back(' ') != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(doc.value(id)) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(string_view("?>")) != castle::status::ok)
        {
            return castle::status::full;
        }
        return castle::status::ok;
    }
    if (kind == type::doctype)
    {
        if (output.append(string_view("<!DOCTYPE")) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(doc.value(id)) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(string_view(">")) != castle::status::ok)
        {
            return castle::status::full;
        }
        return castle::status::ok;
    }
    if (kind == type::document)
    {
        bool first = true;
        for (typename Document::node_id child = doc.first_child(id); child != Document::npos; child = doc.next_sibling(child))
        {
            if (!first && pretty)
            {
                if (output.push_back('\n') != castle::status::ok)
                {
                    return castle::status::full;
                }
            }
            first = false;
            if (pretty && child != doc.root())
            {
                if (write_indent<Document>(output, depth, spaces) != castle::status::ok)
                {
                    return castle::status::full;
                }
            }
            CASTLE_CONST castle::status s = serialize_node(doc, child, output, pretty, depth, spaces);
            if (s != castle::status::ok)
            {
                return s;
            }
        }
        return castle::status::ok;
    }
    if (kind != type::element)
    {
        return castle::status::invalid_argument;
    }

    if (output.push_back('<') != castle::status::ok)
    {
        return castle::status::full;
    }
    if (output.append(doc.name(id)) != castle::status::ok)
    {
        return castle::status::full;
    }
    for (typename Document::attribute_id a = doc.first_attribute(id); a != Document::attribute_npos; a = doc.next_attribute(a))
    {
        if (output.push_back(' ') != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(doc.attribute_name(a)) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.append(string_view("=\"")) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (append_escaped(output, doc.attribute_value(a), true) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (output.push_back('"') != castle::status::ok)
        {
            return castle::status::full;
        }
    }

    CASTLE_CONST typename Document::node_id first = doc.first_child(id);
    if (first == Document::npos)
    {
        return output.append(string_view("/>"));
    }
    if (output.push_back('>') != castle::status::ok)
    {
        return castle::status::full;
    }

    CASTLE_CONST bool mixed = has_mixed_text(doc, id);
    if (pretty && !mixed)
    {
        if (output.push_back('\n') != castle::status::ok)
        {
            return castle::status::full;
        }
    }

    bool child_first = true;
    for (typename Document::node_id child = first; child != Document::npos; child = doc.next_sibling(child))
    {
        if (pretty && !mixed)
        {
            if (!child_first && output.push_back('\n') != castle::status::ok)
            {
                return castle::status::full;
            }
            if (write_indent<Document>(output, depth + 1U, spaces) != castle::status::ok)
            {
                return castle::status::full;
            }
        }
        child_first = false;
        CASTLE_CONST castle::status s = serialize_node(doc, child, output, pretty, depth + 1U, spaces);
        if (s != castle::status::ok)
        {
            return s;
        }
    }
    if (pretty && !mixed)
    {
        if (output.push_back('\n') != castle::status::ok)
        {
            return castle::status::full;
        }
        if (write_indent<Document>(output, depth, spaces) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    if (output.append(string_view("</")) != castle::status::ok)
    {
        return castle::status::full;
    }
    if (output.append(doc.name(id)) != castle::status::ok)
    {
        return castle::status::full;
    }
    if (output.push_back('>') != castle::status::ok)
    {
        return castle::status::full;
    }
    return castle::status::ok;
}

} // namespace detail

template <castle::size_type MaxNodes, castle::size_type MaxAttributes, castle::size_type MaxStringBytes, castle::size_type MaxDepth, castle::size_type MaxOutput>
result serialize(CASTLE_CONST document<MaxNodes, MaxAttributes, MaxStringBytes, MaxDepth>& doc,
                 castle::container::string<MaxOutput>& output,
                 bool pretty = false, castle::size_type indent_spaces = 2U) CASTLE_NOEXCEPT
{
    result r;
    output.clear();
    if (doc.root() == document<MaxNodes, MaxAttributes, MaxStringBytes, MaxDepth>::npos)
    {
        r.status = castle::status::empty;
        return r;
    }
    if (indent_spaces == 0U && pretty)
    {
        indent_spaces = 1U;
    }
    CASTLE_CONST castle::status s = detail::serialize_node(doc, doc.document_node(), output, pretty, 0U, indent_spaces);
    if (s != castle::status::ok)
    {
        output.clear();
        r.status = s;
        r.code = error_code::output_full;
        return r;
    }
    return r;
}

} // namespace xml
} // namespace serialization
} // namespace castle

#endif // CASTLE_SERIALIZATION_XML_HPP
