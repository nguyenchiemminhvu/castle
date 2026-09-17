#ifndef CASTLE_SERIALIZATION_JSON_HPP
#define CASTLE_SERIALIZATION_JSON_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/constants.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"
#include "castle/utility/optional.hpp"
#include "castle/utility/string_builder.hpp"

#include <float.h>
#include <stdint.h>

namespace castle
{
namespace serialization
{
namespace json
{

using string_view = castle::container::string_view;

// JSON value kinds. `number` stores the source spelling so integer precision is
// never lost merely by parsing a document.
enum class type : uint8_t
{
    null_value = 0U,
    boolean,
    number,
    string,
    object,
    array
};

enum class error_code : uint8_t
{
    none = 0U,
    unexpected_end,
    unexpected_token,
    invalid_string,
    invalid_escape,
    invalid_unicode_escape,
    invalid_number,
    invalid_literal,
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

template <typename T>
using enable_integer_t = meta::enable_if_t<meta::is_integral<T>::value && !meta::is_same<T,bool>::value, castle::status>;

template <typename T>
using enable_float_t = meta::enable_if_t<meta::is_floating_point<T>::value, castle::status>;

template <typename T>
using enable_enum_t = meta::enable_if_t<meta::is_enum<T>::value, castle::status>;

CASTLE_INLINE bool is_space(char c) CASTLE_NOEXCEPT
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

CASTLE_INLINE bool is_digit(char c) CASTLE_NOEXCEPT
{
    return c >= '0' && c <= '9';
}

CASTLE_INLINE bool is_hex(char c) CASTLE_NOEXCEPT
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

CASTLE_INLINE uint8_t hex_value(char c) CASTLE_NOEXCEPT
{
    if (c >= '0' && c <= '9')
    {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f')
    {
        return static_cast<uint8_t>(c - 'a' + 10);
    }
    return static_cast<uint8_t>(c - 'A' + 10);
}

CASTLE_INLINE bool equal(string_view a, string_view b) CASTLE_NOEXCEPT
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

CASTLE_INLINE bool is_control(char c) CASTLE_NOEXCEPT
{
    return static_cast<unsigned char>(c) < characters::ascii_control_max;
}

CASTLE_INLINE bool is_number_delimiter(char c) CASTLE_NOEXCEPT
{
    return is_space(c) || c == ',' || c == ']' || c == '}';
}

template <castle::size_type N>
CASTLE_INLINE bool append_utf8(uint16_t code_unit, castle::container::string<N>& output) CASTLE_NOEXCEPT
{
    if (code_unit >= characters::utf16_surrogate_min && code_unit <= characters::utf16_surrogate_max)
    {
        return false;
    }
    if (code_unit <= characters::utf8_one_byte_max)
    {
        return output.push_back(static_cast<char>(code_unit)) == castle::status::ok;
    }
    if (code_unit <= characters::utf8_two_byte_max)
    {
        return output.push_back(static_cast<char>(characters::utf8_two_byte_prefix | (code_unit >> characters::utf8_shift_6))) == castle::status::ok &&
               output.push_back(static_cast<char>(characters::utf8_continuation_prefix | (code_unit & characters::utf8_continuation_mask))) == castle::status::ok;
    }
    return output.push_back(static_cast<char>(characters::utf8_three_byte_prefix | (code_unit >> characters::utf8_shift_12))) == castle::status::ok &&
           output.push_back(static_cast<char>(characters::utf8_continuation_prefix | ((code_unit >> characters::utf8_shift_6) & characters::utf8_continuation_mask))) == castle::status::ok &&
           output.push_back(static_cast<char>(characters::utf8_continuation_prefix | (code_unit & characters::utf8_continuation_mask))) == castle::status::ok;
}

// JSON strings are decoded into a temporary fixed buffer and then copied into
// the document arena. This keeps the parser independent of the input lifetime.
template <castle::size_type MaxScratch>
result decode_string(string_view source, castle::size_type& position,
                     castle::container::string<MaxScratch>& output)
{
    result r;
    output.clear();
    if (position >= source.size() || source[position] != '"')
    {
        r.status = castle::status::invalid_argument;
        r.code = error_code::invalid_string;
        r.offset = position;
        return r;
    }

    ++position;
    while (position < source.size())
    {
        CASTLE_CONST char c = source[position++];
        if (c == '"')
        {
            return r;
        }
        if (is_control(c))
        {
            r.status = castle::status::invalid_argument;
            r.code = error_code::invalid_string;
            r.offset = position - 1U;
            return r;
        }
        if (c != '\\')
        {
            if (output.push_back(c) != castle::status::ok)
            {
                r.status = castle::status::full;
                r.code = error_code::capacity;
                r.offset = position - 1U;
                return r;
            }
            continue;
        }

        if (position >= source.size())
        {
            r.status = castle::status::invalid_argument;
            r.code = error_code::unexpected_end;
            r.offset = position;
            return r;
        }

        CASTLE_CONST char escape = source[position++];
        char decoded = 0;
        switch (escape)
        {
            case '"': decoded = '"'; break;
            case '\\': decoded = '\\'; break;
            case '/': decoded = '/'; break;
            case 'b': decoded = '\b'; break;
            case 'f': decoded = '\f'; break;
            case 'n': decoded = '\n'; break;
            case 'r': decoded = '\r'; break;
            case 't': decoded = '\t'; break;
            case 'u':
            {
                if (position + 4U > source.size())
                {
                    r.status = castle::status::invalid_argument;
                    r.code = error_code::invalid_unicode_escape;
                    r.offset = position - 2U;
                    return r;
                }
                uint16_t code = 0U;
                for (castle::size_type i = 0U; i < 4U; ++i)
                {
                    if (!is_hex(source[position + i]))
                    {
                        r.status = castle::status::invalid_argument;
                        r.code = error_code::invalid_unicode_escape;
                        r.offset = position + i;
                        return r;
                    }
                    code = static_cast<uint16_t>((code << 4U) | hex_value(source[position + i]));
                }
                position += 4U;
                if (!append_utf8(code, output))
                {
                    r.status = castle::status::invalid_argument;
                    r.code = error_code::invalid_unicode_escape;
                    r.offset = position - 4U;
                    return r;
                }
                continue;
            }
            default:
            {
                r.status = castle::status::invalid_argument;
                r.code = error_code::invalid_escape;
                r.offset = position - 1U;
                return r;
            }
        }

        if (output.push_back(decoded) != castle::status::ok)
        {
            r.status = castle::status::full;
            r.code = error_code::capacity;
            r.offset = position - 1U;
            return r;
        }
    }

    r.status = castle::status::invalid_argument;
    r.code = error_code::unexpected_end;
    r.offset = position;
    return r;
}

CASTLE_INLINE bool valid_number(string_view text) CASTLE_NOEXCEPT
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
        if (i < text.size() && is_digit(text[i]))
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
        while (i < text.size() && is_digit(text[i]))
        {
            ++i;
        }
    }
    if (i < text.size() && text[i] == '.')
    {
        ++i;
        if (i == text.size() || !is_digit(text[i]))
        {
            return false;
        }
        while (i < text.size() && is_digit(text[i]))
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
        if (i == text.size() || !is_digit(text[i]))
        {
            return false;
        }
        while (i < text.size() && is_digit(text[i]))
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
    using U = meta::make_unsigned_t<T>;
    castle::size_type i = 0U;
    bool negative = false;
    if (text[0U] == '-')
    {
        negative = true; i = 1U; 
    }
    if (text[0U] == '+')
    {
        return castle::status::invalid_argument;
    }
    if (i == text.size() || (negative && !meta::is_signed<T>::value))
    {
        return castle::status::invalid_argument;
    }
    uint64_t magnitude = 0U;
    for (; i < text.size(); ++i)
    {
        if (!is_digit(text[i]))
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
        negative = true; ++i; 
    }
    T value = static_cast<T>(0);
    while (i < text.size() && is_digit(text[i]))
    {
        value = value * static_cast<T>(10) + static_cast<T>(text[i] - '0');
        ++i;
    }
    if (i < text.size() && text[i] == '.')
    {
        ++i;
        T scale = static_cast<T>(0.1);
        while (i < text.size() && is_digit(text[i]))
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
            ++i;
            if (exponent > max_floating_exponent)
            {
                return castle::status::out_of_range;
            }
        }
    }
    for (int e = 0; e < exponent; ++e)
    {
        value *= exponent_negative ? static_cast<T>(0.1) : static_cast<T>(10);
    }
    if (value > castle::numeric_limits<T>::max())
    {
        return castle::status::out_of_range;
    }
    output = negative ? -value : value;
    return castle::status::ok;
}

template <typename T>
castle::status format_integer(T value, castle::container::string<max_integer_str_len>& output) CASTLE_NOEXCEPT
{
    using U = meta::make_unsigned_t<T>;
    output.clear();
    bool negative = meta::is_signed<T>::value && value < static_cast<T>(0);
    U magnitude = negative
                  ? static_cast<U>(-(value + static_cast<T>(1))) + static_cast<U>(1)
                  : static_cast<U>(value);
    char digits[max_integer_str_len];
    castle::size_type count = 0U;

    do
    {
        digits[count++] = static_cast<char>('0' + magnitude % static_cast<U>(10));
        magnitude /= static_cast<U>(10);
    } while (magnitude != 0U);

    if (negative && output.push_back('-') != castle::status::ok)
    {
        return castle::status::full;
    }

    while (count > 0U)
    {
        --count;
        if (output.push_back(digits[count]) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return castle::status::ok;
}

template <typename OutputString>
castle::status append_escaped(OutputString& output, string_view value) CASTLE_NOEXCEPT
{
    if (output.push_back('"') != castle::status::ok)
    {
        return castle::status::full;
    }

    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        CASTLE_CONST unsigned char c = static_cast<unsigned char>(value[i]);
        CASTLE_CONST char* escaped = nullptr;
        switch (value[i])
        {
            case '"': escaped = "\\\""; break;
            case '\\': escaped = "\\\\"; break;
            case '\b': escaped = "\\b"; break;
            case '\f': escaped = "\\f"; break;
            case '\n': escaped = "\\n"; break;
            case '\r': escaped = "\\r"; break;
            case '\t': escaped = "\\t"; break;
            default: break;
        }
        if (escaped != nullptr)
        {
            if (output.append(string_view(escaped)) != castle::status::ok)
            {
                return castle::status::full;
            }
        }
        else if (c < characters::ascii_control_max)
        {
            return castle::status::invalid_argument;
        }
        else if (output.push_back(value[i]) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return output.push_back('"');
}

} // namespace detail

// A compact owning DOM. Keys and scalar values live in one fixed arena; tree
// links live in fixed node storage. There is no pointer into the input buffer.
template <castle::size_type MaxNodes = 64U,
          castle::size_type MaxStringBytes = 1024U,
          castle::size_type MaxDepth = 16U>
class document
{
    static_assert(MaxNodes > 0U, "json::document requires nodes");
    static_assert(MaxStringBytes > 0U, "json::document requires string storage");
    static_assert(MaxDepth > 0U, "json::document requires a positive depth");

public:
    using size_type = castle::size_type;
    using view_type = string_view;
    using node_id = size_type;
    static CASTLE_CONSTEXPR node_id npos = static_cast<node_id>(-1);
    static CASTLE_CONSTEXPR size_type static_string_capacity = MaxStringBytes;
    static CASTLE_CONSTEXPR size_type static_max_depth = MaxDepth;

    struct node
    {
        type kind = type::null_value;
        node_id parent = npos;
        node_id first_child = npos;
        node_id last_child = npos;
        node_id next_sibling = npos;
        size_type key_offset = 0U;
        size_type key_size = 0U;
        size_type value_offset = 0U;
        size_type value_size = 0U;
    };

    document() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    void clear() CASTLE_NOEXCEPT
    {
        node_count_ = 0U;
        used_ = 0U;
    }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_; }
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxNodes; }
    size_type bytes_used() CASTLE_CONST CASTLE_NOEXCEPT { return used_; }
    size_type bytes_capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxStringBytes; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == MaxNodes; }

    node_id root() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == 0U ? npos : 0U; }
    type kind(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].kind : type::null_value; }
    node_id parent(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].parent : npos; }
    node_id first_child(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].first_child : npos; }
    node_id next_sibling(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return valid(id) ? nodes_[id].next_sibling : npos; }

    view_type key(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return view(id, nodes_[id].key_offset, nodes_[id].key_size); }
    view_type scalar(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return view(id, nodes_[id].value_offset, nodes_[id].value_size); }
    view_type string(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return kind(id) == type::string ? scalar(id) : view_type(); }

    node_id find(node_id object, view_type name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(object) || kind(object) != type::object)
        {
            return npos;
        }
        for (node_id child = first_child(object); child != npos; child = next_sibling(child))
        {
            if (detail::equal(key(child), name))
            {
                return child;
            }
        }
        return npos;
    }

    node_id at(node_id array, size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(array) || kind(array) != type::array)
        {
            return npos;
        }
        node_id child = first_child(array);
        for (size_type i = 0U; child != npos; ++i, child = next_sibling(child))
        {
            if (i == index)
            {
                return child;
            }
        }
        return npos;
    }

    castle::status set_null(node_id& out, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        return add_node(out, type::null_value, parent_id, key_name, view_type());
    }
    castle::status set_bool(node_id& out, bool value, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        return add_node(out, type::boolean, parent_id, key_name, value ? view_type("true") : view_type("false"));
    }
    castle::status set_string(node_id& out, view_type value, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        return add_node(out, type::string, parent_id, key_name, value);
    }
    castle::status set_number(node_id& out, view_type value, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        if (!detail::valid_number(value))
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, type::number, parent_id, key_name, value);
    }
    castle::status make_object(node_id& out, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        return add_node(out, type::object, parent_id, key_name, view_type());
    }
    castle::status make_array(node_id& out, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        return add_node(out, type::array, parent_id, key_name, view_type());
    }

    template <typename T>
    detail::enable_integer_t<T>
    set_integer(node_id& out, T value, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        castle::container::string<max_integer_str_len> encoded;
        CASTLE_CONST castle::status s = detail::format_integer(value, encoded);
        if (s != castle::status::ok)
        {
            return s;
        }
        return set_number(out, encoded.view(), parent_id, key_name);
    }

    template <typename T>
    detail::enable_enum_t<T>
    set_enum(node_id& out, T value, node_id parent_id = npos, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        return set_integer(out, static_cast<meta::underlying_type_t<T>>(value), parent_id, key_name);
    }

    template <typename T>
    detail::enable_float_t<T>
    set_floating(node_id& out, T value, node_id parent_id = npos, view_type key_name = view_type(), size_type precision = 6U) CASTLE_NOEXCEPT
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
        return set_number(out, builder.view(), parent_id, key_name);
    }

    template <typename T>
    detail::enable_integer_t<T>
    get_integer(node_id id, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(id) || kind(id) != type::number)
        {
            return castle::status::invalid_argument;
        }
        return detail::parse_integer(scalar(id), output);
    }

    template <typename T>
    detail::enable_enum_t<T>
    get_enum(node_id id, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        using U = meta::underlying_type_t<T>;
        U value = 0;
        CASTLE_CONST castle::status s = get_integer(id, value);
        if (s == castle::status::ok)
        {
            output = static_cast<T>(value);
        }
        return s;
    }

    template <typename T>
    detail::enable_float_t<T>
    get_floating(node_id id, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(id) || kind(id) != type::number)
        {
            return castle::status::invalid_argument;
        }
        return detail::parse_floating(scalar(id), output);
    }

    castle::status get_bool(node_id id, bool& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(id) || kind(id) != type::boolean)
        {
            return castle::status::invalid_argument;
        }
        output = detail::equal(scalar(id), string_view("true"));
        return castle::status::ok;
    }

    castle::status get_string(node_id id, view_type& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!valid(id) || kind(id) != type::string)
        {
            output = view_type();
            return castle::status::invalid_argument;
        }
        output = scalar(id);
        return castle::status::ok;
    }

    castle::status append(node_id parent_id, node_id& out, type value_type, view_type value = view_type(), view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        if (!valid(parent_id) || (kind(parent_id) != type::array && kind(parent_id) != type::object))
        {
            return castle::status::invalid_argument;
        }
        if (kind(parent_id) == type::object && key_name.empty())
        {
            return castle::status::invalid_argument;
        }
        return add_node(out, value_type, parent_id, key_name, value);
    }

    castle::status parse(view_type source) CASTLE_NOEXCEPT;

private:
    bool valid(node_id id) CASTLE_CONST CASTLE_NOEXCEPT { return id != npos && id < node_count_; }
    view_type view(node_id id, size_type offset, size_type length) CASTLE_CONST CASTLE_NOEXCEPT
    {
        (void)id;
        return length == 0U
               ? view_type()
               : view_type(storage_ + offset, length);
    }

    castle::status store(view_type value, size_type& offset, size_type& length) CASTLE_NOEXCEPT
    {
        if (value.size() > MaxStringBytes - used_)
        {
            return castle::status::full;
        }
        offset = used_; length = value.size();
        for (size_type i = 0U; i < value.size(); ++i)
        {
            storage_[used_ + i] = value[i];
        }
        used_ += value.size();
        return castle::status::ok;
    }

    castle::status add_node(node_id& out, type value_type, node_id parent_id, view_type key_name, view_type value) CASTLE_NOEXCEPT
    {
        if (parent_id != npos && !valid(parent_id))
        {
            return castle::status::invalid_argument;
        }
        if (value_type == type::object || value_type == type::array) value = view_type();
        if (parent_id == npos && node_count_ != 0U)
        {
            return castle::status::already_exists;
        }
        if (parent_id != npos && kind(parent_id) == type::object && key_name.empty())
        {
            return castle::status::invalid_argument;
        }
        if (parent_id != npos && kind(parent_id) == type::array && !key_name.empty())
        {
            return castle::status::invalid_argument;
        }
        if (node_count_ == MaxNodes)
        {
            return castle::status::full;
        }

        node candidate;
        candidate.kind = value_type;
        candidate.parent = parent_id;
        castle::status s = store(key_name, candidate.key_offset, candidate.key_size);
        if (s != castle::status::ok)
        {
            return s;
        }
        s = store(value, candidate.value_offset, candidate.value_size);
        if (s != castle::status::ok)
        {
            used_ -= candidate.key_size;
            return s;
        }
        out = node_count_++;
        nodes_[out] = candidate;
        if (parent_id != npos)
        {
            if (nodes_[parent_id].first_child == npos)
            {
                nodes_[parent_id].first_child = out;
            }
            else
            {
                nodes_[nodes_[parent_id].last_child].next_sibling = out;
            }
            nodes_[parent_id].last_child = out;
        }
        return castle::status::ok;
    }

    node nodes_[MaxNodes];
    size_type node_count_ = 0U;
    char storage_[MaxStringBytes] = {};
    size_type used_ = 0U;
};

namespace detail
{

template <typename Document>
class parser
{
public:
    using view_type = string_view;
    using size_type = castle::size_type;
    using node_id = typename Document::node_id;

    parser(Document& document, view_type source) CASTLE_NOEXCEPT
        : doc_(document), source_(source), position_(0U), depth_(0U) {}

    result run() CASTLE_NOEXCEPT
    {
        doc_.clear();
        skip_space();
        node_id root = Document::npos;
        result r = parse_value(root, Document::npos);
        if (!r.succeeded())
        {
            doc_.clear();
            return r;
        }
        skip_space();
        if (position_ != source_.size())
        {
            return fail(error_code::unexpected_token);
        }
        return r;
    }

private:
    result fail(error_code code) CASTLE_CONST CASTLE_NOEXCEPT
    {
        result r{};
        r.status = castle::status::invalid_argument;
        r.code = code;
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

    void skip_space() CASTLE_NOEXCEPT
    {
        while (position_ < source_.size() && is_space(source_[position_]))
        {
            ++position_;
        }
    }

    result parse_value(node_id& out, node_id parent, view_type key_name = view_type()) CASTLE_NOEXCEPT
    {
        skip_space();
        if (position_ >= source_.size())
        {
            return fail(error_code::unexpected_end);
        }
        CASTLE_CONST char c = source_[position_];
        if (c == '{') return parse_object(out, parent, key_name);
        if (c == '[') return parse_array(out, parent, key_name);
        if (c == '"') return parse_string_value(out, parent, key_name);
        if (c == 't') return parse_literal(out, parent, key_name, "true", type::boolean);
        if (c == 'f') return parse_literal(out, parent, key_name, "false", type::boolean);
        if (c == 'n') return parse_literal(out, parent, key_name, "null", type::null_value);
        if (c == '-' || is_digit(c)) return parse_number(out, parent, key_name);
        return fail(error_code::unexpected_token);
    }

    result parse_object(node_id& out, node_id parent, view_type key_name) CASTLE_NOEXCEPT
    {
        ++position_;
        ++depth_;
        if (depth_ > Document::static_max_depth)
        {
            --depth_;
            return fail(error_code::depth_exceeded);
        }
        castle::status s = doc_.make_object(out, parent, key_name);
        if (s != castle::status::ok)
        {
            --depth_;
            return capacity_result(s);
        }
        skip_space();
        if (position_ < source_.size() && source_[position_] == '}')
        {
            ++position_;
            --depth_;
            return result();
        }
        while (position_ < source_.size())
        {
            if (source_[position_] != '"')
            {
                --depth_;
                return fail(error_code::unexpected_token);
            }
            castle::container::string<Document::static_string_capacity> key;
            result kr = decode_string(source_, position_, key);
            if (!kr.succeeded())
            {
                --depth_;
                return kr;
            }
            skip_space();
            if (position_ >= source_.size() || source_[position_] != ':')
            {
                --depth_;
                return fail(error_code::unexpected_token);
            }
            ++position_;
            node_id child = Document::npos;
            result vr = parse_value(child, out, key.view());
            if (!vr.succeeded())
            {
                --depth_;
                return vr;
            }
            skip_space();
            if (position_ < source_.size() && source_[position_] == '}')
            {
                ++position_;
                --depth_;
                return result();
            }
            if (position_ >= source_.size() || source_[position_] != ',')
            {
                --depth_;
                return fail(error_code::unexpected_token);
            }
            ++position_;
            skip_space();
        }
        --depth_;
        return fail(error_code::unexpected_end);
    }

    result parse_array(node_id& out, node_id parent, view_type key_name) CASTLE_NOEXCEPT
    {
        ++position_;
        ++depth_;
        if (depth_ > Document::static_max_depth)
        {
            --depth_;
            return fail(error_code::depth_exceeded);
        }
        castle::status s = doc_.make_array(out, parent, key_name);
        if (s != castle::status::ok)
        {
            --depth_;
            return capacity_result(s);
        }
        skip_space();
        if (position_ < source_.size() && source_[position_] == ']')
        {
            ++position_;
            --depth_;
            return result();
        }
        while (position_ < source_.size())
        {
            node_id child = Document::npos;
            result vr = parse_value(child, out);
            if (!vr.succeeded())
            {
                --depth_;
                return vr;
            }
            skip_space();
            if (position_ < source_.size() && source_[position_] == ']')
            {
                ++position_;
                --depth_;
                return result();
            }
            if (position_ >= source_.size() || source_[position_] != ',')
            {
                --depth_;
                return fail(error_code::unexpected_token);
            }
            ++position_;
            skip_space();
        }
        --depth_;
        return fail(error_code::unexpected_end);
    }

    result parse_string_value(node_id& out, node_id parent, view_type key_name) CASTLE_NOEXCEPT
    {
        castle::container::string<Document::static_string_capacity> value;
        result r = decode_string(source_, position_, value);
        if (!r.succeeded())
        {
            return r;
        }
        CASTLE_CONST castle::status s = doc_.set_string(out, value.view(), parent, key_name);
        return s == castle::status::ok ? result() : capacity_result(s);
    }

    result parse_literal(node_id& out, node_id parent, view_type key_name, CASTLE_CONST char* literal, type kind) CASTLE_NOEXCEPT
    {
        size_type length = 0U;
        while (literal[length] != '\0')
        {
            ++length;
        }
        if (position_ + length > source_.size())
        {
            return fail(error_code::unexpected_end);
        }
        for (size_type i = 0U; i < length; ++i)
        {
            if (source_[position_ + i] != literal[i])
            {
                return fail(error_code::invalid_literal);
            }
        }
        position_ += length;
        castle::status s = kind == type::boolean
                                   ? doc_.set_bool(out, literal[0] == 't', parent, key_name)
                                   : doc_.set_null(out, parent, key_name);
        return s == castle::status::ok
               ? result()
               : capacity_result(s);
    }

    result parse_number(node_id& out, node_id parent, view_type key_name) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type start = position_;
        while (position_ < source_.size() && !is_number_delimiter(source_[position_]))
        {
            CASTLE_CONST char c = source_[position_];
            if (!(is_digit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E'))
            {
                return fail(error_code::invalid_number);
            }
            ++position_;
        }
        CASTLE_CONST view_type number(source_.data() + start, position_ - start);
        if (!valid_number(number))
        {
            return fail(error_code::invalid_number);
        }
        CASTLE_CONST castle::status s = doc_.set_number(out, number, parent, key_name);
        return s == castle::status::ok
               ? result()
               : capacity_result(s);
    }

    result capacity_result(castle::status s) CASTLE_CONST CASTLE_NOEXCEPT
    {
        result r;
        r.status = s;
        r.code = error_code::capacity;
        r.offset = position_;
        return r;
    }

    Document& doc_;
    view_type source_;
    size_type position_;
    size_type depth_;
};

} // namespace detail

template <castle::size_type MaxNodes, castle::size_type MaxStringBytes, castle::size_type MaxDepth>
result parse(document<MaxNodes, MaxStringBytes, MaxDepth>& doc, string_view source) CASTLE_NOEXCEPT
{
    // The parser's compile-time depth is intentionally checked against its
    // actual configured document depth below through the public wrapper.
    detail::parser<document<MaxNodes, MaxStringBytes, MaxDepth>> parser_instance(doc, source);
    return parser_instance.run();
}

template <castle::size_type MaxNodes, castle::size_type MaxStringBytes, castle::size_type MaxDepth>
castle::status document<MaxNodes, MaxStringBytes, MaxDepth>::parse(view_type source) CASTLE_NOEXCEPT
{
    return json::parse(*this, source).status;
}

namespace detail
{
template <typename Document, typename OutputString>
castle::status write_indent(Document CASTLE_CONST&, OutputString& out, castle::size_type depth, castle::size_type spaces) CASTLE_NOEXCEPT
{
    for (castle::size_type i = 0U; i < depth * spaces; ++i)
    {
        if (out.push_back(' ') != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return castle::status::ok;
}

template <typename Document, typename OutputString>
castle::status serialize_node(Document CASTLE_CONST& doc, typename Document::node_id id,
                              OutputString& out,
                              bool pretty, castle::size_type depth, castle::size_type spaces) CASTLE_NOEXCEPT
{
    CASTLE_CONST type k = doc.kind(id);
    if (k == type::null_value)
    {
        return out.append(string_view("null"));
    }
    if (k == type::boolean || k == type::number)
    {
        return out.append(doc.scalar(id));
    }
    if (k == type::string)
    {
        return append_escaped(out, doc.string(id));
    }

    CASTLE_CONST bool object = (k == type::object);
    if (out.push_back(object ? '{' : '[') != castle::status::ok)
    {
        return castle::status::full;
    }
    typename Document::node_id child = doc.first_child(id);
    if (child == Document::npos)
    {
        return out.push_back(object ? '}' : ']');
    }
    if (pretty && out.push_back('\n') != castle::status::ok)
    {
        return castle::status::full;
    }

    bool first = true;
    for (; child != Document::npos; child = doc.next_sibling(child))
    {
        if (!first)
        {
            if (out.push_back(',') != castle::status::ok)
            {
                return castle::status::full;
            }
            if (pretty && out.push_back('\n') != castle::status::ok)
            {
                return castle::status::full;
            }
        }
        first = false;
        if (pretty && write_indent(doc, out, depth + 1U, spaces) != castle::status::ok)
        {
            return castle::status::full;
        }
        if (object)
        {
            castle::status s = append_escaped(out, doc.key(child));
            if (s != castle::status::ok || out.push_back(':') != castle::status::ok)
            {
                return castle::status::full;
            }
            if (pretty && out.push_back(' ') != castle::status::ok)
            {
                return castle::status::full;
            }
        }
        castle::status s = serialize_node(doc, child, out, pretty, depth + 1U, spaces);
        if (s != castle::status::ok)
        {
            return s;
        }
    }
    if (pretty)
    {
        if (out.push_back('\n') != castle::status::ok)
        {
            return castle::status::full;
        }
        if (write_indent(doc, out, depth, spaces) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return out.push_back(object ? '}' : ']');
}
} // namespace detail

template <castle::size_type MaxNodes, castle::size_type MaxStringBytes, castle::size_type MaxDepth, castle::size_type MaxOutput>
result serialize(document<MaxNodes, MaxStringBytes, MaxDepth> CASTLE_CONST& doc,
                 castle::container::string<MaxOutput>& output,
                 bool pretty = false, castle::size_type indent_spaces = 2U) CASTLE_NOEXCEPT
{
    result r;
    output.clear();
    if (doc.root() == document<MaxNodes, MaxStringBytes, MaxDepth>::npos)
    {
        r.status = castle::status::empty;
        return r;
    }
    CASTLE_CONST castle::status s = detail::serialize_node(doc, doc.root(), output, pretty, 0U, indent_spaces);
    if (s != castle::status::ok)
    {
        output.clear();
        r.status = s;
        r.code = error_code::output_full;
        return r;
    }
    return r;
}

} // namespace json
} // namespace serialization
} // namespace castle

#endif // CASTLE_SERIALIZATION_JSON_HPP
