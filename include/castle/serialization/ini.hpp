#ifndef CASTLE_SERIALIZATION_INI_HPP
#define CASTLE_SERIALIZATION_INI_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/error/status.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"
#include "castle/container/vector.hpp"
#include "castle/utility/optional.hpp"

#include <float.h>
#include <stdint.h>

namespace castle
{
namespace serialization
{
namespace ini
{

using string_view = castle::container::string_view;

// ============================================================================
// error_code / result
// ----------------------------------------------------------------------------
// castle::status carries the generic operation result. error_code supplies the
// INI-specific parse/serialization reason without expanding castle::status.
// line and column are one-based for parse errors and zero when not applicable.
// ============================================================================
enum class error_code : uint8_t
{
    none = 0U,
    invalid_section,
    invalid_key,
    invalid_assignment,
    invalid_quote,
    invalid_escape,
    capacity,
    output_full
};

struct result
{
    castle::status status = castle::status::ok;
    error_code code = error_code::none;
    castle::size_type line = 0U;
    castle::size_type column = 0U;

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool succeeded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::succeeded(status);
    }
};

namespace detail
{

template <typename T>
using enable_integer_t = meta::enable_if_t<
    meta::is_integral<T>::value && !meta::is_same<T, bool>::value,
    castle::status>;

template <typename T>
using enable_enum_t = meta::enable_if_t<meta::is_enum<T>::value, castle::status>;

template <typename T>
using enable_float_t = meta::enable_if_t<meta::is_floating_point<T>::value, castle::status>;

inline bool is_space(char value) CASTLE_NOEXCEPT
{
    return value == ' ' || value == '\t' || value == '\v' || value == '\f';
}

inline bool is_line_end(char value) CASTLE_NOEXCEPT
{
    return value == '\r' || value == '\n';
}

inline char lower_ascii(char value) CASTLE_NOEXCEPT
{
    return (value >= 'A' && value <= 'Z')
           ? static_cast<char>(value - 'A' + 'a')
           : value;
}

inline bool equal_views(string_view lhs, string_view rhs) CASTLE_NOEXCEPT
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (castle::size_type i = 0U; i < lhs.size(); ++i)
    {
        if (lhs[i] != rhs[i])
        {
            return false;
        }
    }
    return true;
}

inline bool equals_ignore_case(string_view lhs, string_view rhs) CASTLE_NOEXCEPT
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (castle::size_type i = 0U; i < lhs.size(); ++i)
    {
        if (lower_ascii(lhs[i]) != lower_ascii(rhs[i]))
        {
            return false;
        }
    }
    return true;
}

inline string_view trim(string_view value) CASTLE_NOEXCEPT
{
    castle::size_type first = 0U;
    castle::size_type last = value.size();

    while (first < last && is_space(value[first]))
    {
        ++first;
    }
    while (last > first && is_space(value[last - 1U]))
    {
        --last;
    }

    // CR/LF are line delimiters in the outer parser, but keeping this trim
    // helper independent makes it safe for direct use with arbitrary views.
    while (last > first && (value[last - 1U] == '\r' || value[last - 1U] == '\n'))
    {
        --last;
    }

    if (last == first)
    {
        return string_view();
    }
    return string_view(value.data() + first, last - first);
}

inline bool starts_with_comment(string_view line) CASTLE_NOEXCEPT
{
    return !line.empty() && (line[0U] == '#' || line[0U] == ';');
}

inline bool valid_section_name(string_view section) CASTLE_NOEXCEPT
{
    if (section.empty())
    {
        return true; // empty section denotes the global scope.
    }

    for (castle::size_type i = 0U; i < section.size(); ++i)
    {
        CASTLE_CONST char c = section[i];
        if (c == '[' || c == ']' || is_line_end(c))
        {
            return false;
        }
        if (static_cast<unsigned char>(c) < characters::ascii_control_max && !is_space(c))
        {
            return false;
        }
    }
    return true;
}

inline bool valid_key_name(string_view key) CASTLE_NOEXCEPT
{
    if (key.empty())
    {
        return false;
    }

    for (castle::size_type i = 0U; i < key.size(); ++i)
    {
        CASTLE_CONST char c = key[i];
        if (c == '=' || c == ':' || c == '[' || c == ']' || is_line_end(c))
        {
            return false;
        }
        if (static_cast<unsigned char>(c) < characters::ascii_control_max && !is_space(c))
        {
            return false;
        }
    }
    return true;
}

inline castle::size_type find_separator(string_view line) CASTLE_NOEXCEPT
{
    for (castle::size_type i = 0U; i < line.size(); ++i)
    {
        if (line[i] == '=' || line[i] == ':')
        {
            return i;
        }
    }
    return string_view::npos;
}

inline bool needs_quotes(string_view value) CASTLE_NOEXCEPT
{
    if (value.empty())
    {
        return true;
    }

    if (is_space(value.front()) || is_space(value.back()))
    {
        return true;
    }

    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        CASTLE_CONST char c = value[i];
        if (is_space(c) || c == '#' || c == ';' || c == '=' || c == ':' ||
            c == '[' || c == ']' || c == '"' || c == '\\' || is_line_end(c))
        {
            return true;
        }
        if (static_cast<unsigned char>(c) < characters::ascii_control_max)
        {
            return true;
        }
    }
    return false;
}

// Intentionally local rather than using <cctype>: INI parsing is ASCII text
// and embedded builds should not pull locale machinery into the image.
inline bool valid_escape(char value) CASTLE_NOEXCEPT
{
    return value == '"' || value == '\\' || value == 'n' || value == 'r' || value == 't';
}

inline char decode_escape(char value) CASTLE_NOEXCEPT
{
    switch (value)
    {
        case '"': return '"';
        case '\\': return '\\';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        default: return value;
    }
}

template <castle::size_type MaxValueLength>
result decode_value(
    string_view raw,
    castle::container::string<MaxValueLength>& output,
    castle::size_type line,
    castle::size_type column) CASTLE_NOEXCEPT
{
    result result_value;
    result_value.line = line;
    result_value.column = column;

    raw = trim(raw);
    output.clear();

    if (raw.empty())
    {
        return result_value;
    }

    if (raw.front() != '"' && raw.front() != '\'')
    {
        for (castle::size_type i = 0U; i < raw.size(); ++i)
        {
            if (is_line_end(raw[i]))
            {
                result_value.status = castle::status::invalid_argument;
                result_value.code = error_code::invalid_assignment;
                result_value.column = column + i;
                return result_value;
            }

            if (output.push_back(raw[i]) != castle::status::ok)
            {
                result_value.status = castle::status::full;
                result_value.code = error_code::capacity;
                result_value.column = column + i;
                return result_value;
            }
        }
        return result_value;
    }

    CASTLE_CONST char quote = raw.front();
    if (raw.size() < 2U || raw.back() != quote)
    {
        result_value.status = castle::status::invalid_argument;
        result_value.code = error_code::invalid_quote;
        result_value.column = column;
        return result_value;
    }

    for (castle::size_type i = 1U; i + 1U < raw.size(); ++i)
    {
        CASTLE_CONST char c = raw[i];
        if (c == '\\')
        {
            if (i + 1U >= raw.size() - 1U)
            {
                result_value.status = castle::status::invalid_argument;
                result_value.code = error_code::invalid_escape;
                result_value.column = column + i;
                return result_value;
            }

            CASTLE_CONST char escaped = raw[++i];
            if (!valid_escape(escaped))
            {
                result_value.status = castle::status::invalid_argument;
                result_value.code = error_code::invalid_escape;
                result_value.column = column + i;
                return result_value;
            }

            if (output.push_back(decode_escape(escaped)) != castle::status::ok)
            {
                result_value.status = castle::status::full;
                result_value.code = error_code::capacity;
                result_value.column = column + i;
                return result_value;
            }
            continue;
        }

        if (c == quote)
        {
            result_value.status = castle::status::invalid_argument;
            result_value.code = error_code::invalid_quote;
            result_value.column = column + i;
            return result_value;
        }

        if (output.push_back(c) != castle::status::ok)
        {
            result_value.status = castle::status::full;
            result_value.code = error_code::capacity;
            result_value.column = column + i;
            return result_value;
        }
    }

    return result_value;
}

template <castle::size_type MaxOutput>
castle::status append_escaped_value(
    string_view value,
    castle::container::string<MaxOutput>& output) CASTLE_NOEXCEPT
{
    CASTLE_CONST bool quote = needs_quotes(value);
    if (quote && output.push_back('"') != castle::status::ok)
    {
        return castle::status::full;
    }

    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        CASTLE_CONST char c = value[i];
        if (quote)
        {
            switch (c)
            {
                case '"':
                    if (output.append(string_view("\\\"")) != castle::status::ok)
                    {
                        return castle::status::full;
                    }
                    break;
                case '\\':
                    if (output.append(string_view("\\\\")) != castle::status::ok)
                    {
                        return castle::status::full;
                    }
                    break;
                case '\n':
                    if (output.append(string_view("\\n")) != castle::status::ok)
                    {
                        return castle::status::full;
                    }
                    break;
                case '\r':
                    if (output.append(string_view("\\r")) != castle::status::ok)
                    {
                        return castle::status::full;
                    }
                    break;
                case '\t':
                    if (output.append(string_view("\\t")) != castle::status::ok)
                    {
                        return castle::status::full;
                    }
                    break;
                default:
                    if (output.push_back(c) != castle::status::ok)
                    {
                        return castle::status::full;
                    }
                    break;
            }
        }
        else if (output.push_back(c) != castle::status::ok)
        {
            return castle::status::full;
        }
    }

    if (quote && output.push_back('"') != castle::status::ok)
    {
        return castle::status::full;
    }
    return castle::status::ok;
}

template <typename T>
castle::status parse_integer_value(string_view text, T& output) CASTLE_NOEXCEPT
{
    using U = meta::make_unsigned_t<T>;

    if (text.empty())
    {
        return castle::status::invalid_argument;
    }

    bool negative = false;
    castle::size_type index = 0U;
    if (text[0U] == '+' || text[0U] == '-')
    {
        negative = text[0U] == '-';
        index = 1U;
    }
    if (index == text.size() || (negative && !meta::is_signed<T>::value))
    {
        return castle::status::invalid_argument;
    }

    uint64_t magnitude = 0U;
    for (; index < text.size(); ++index)
    {
        CASTLE_CONST char c = text[index];
        if (c < '0' || c > '9')
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST uint64_t digit = static_cast<uint64_t>(c - '0');
        if (magnitude > (castle::numeric_limits<uint64_t>::max() - digit) / 10U)
        {
            return castle::status::out_of_range;
        }
        magnitude = magnitude * 10U + digit;
    }

    CASTLE_CONST uint64_t max_unsigned = static_cast<uint64_t>(castle::numeric_limits<U>::max());
    if (!negative)
    {
        if (magnitude > max_unsigned || (meta::is_signed<T>::value &&
            magnitude > (max_unsigned >> 1U)))
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

    if (magnitude == minimum_magnitude)
    {
        output = static_cast<T>(-static_cast<T>(magnitude - 1U) - static_cast<T>(1));
    }
    else
    {
        output = static_cast<T>(-static_cast<T>(magnitude));
    }
    return castle::status::ok;
}

template <typename T, size_type MaxValueLength = 128U>
castle::status format_integer_value(T value, castle::container::string<MaxValueLength>& output) CASTLE_NOEXCEPT
{
    using U = meta::make_unsigned_t<T>;
    output.clear();

    bool negative = false;
    U magnitude;
    if (meta::is_signed<T>::value && value < static_cast<T>(0))
    {
        negative = true;
        magnitude = static_cast<U>(-(value + static_cast<T>(1)));
        ++magnitude;
    }
    else
    {
        magnitude = static_cast<U>(value);
    }

    char reverse_digits[max_integer_str_len];
    castle::size_type count = 0U;
    do
    {
        reverse_digits[count++] = static_cast<char>('0' + (magnitude % static_cast<U>(10)));
        magnitude /= static_cast<U>(10);
    }
    while (magnitude != static_cast<U>(0));

    if (negative && output.push_back('-') != castle::status::ok)
    {
        return castle::status::full;
    }

    while (count > 0U)
    {
        --count;
        if (output.push_back(reverse_digits[count]) != castle::status::ok)
        {
            return castle::status::full;
        }
    }
    return castle::status::ok;
}


template <typename T>
detail::enable_float_t<T>
parse_floating_value(string_view text, T& output) CASTLE_NOEXCEPT
{
    if (text.empty())
    {
        return castle::status::invalid_argument;
    }

    castle::size_type index = 0U;
    bool negative = false;
    if (text[0U] == '+' || text[0U] == '-')
    {
        negative = text[0U] == '-';
        index = 1U;
    }

    bool has_digit = false;
    T value = static_cast<T>(0);
    while (index < text.size() && text[index] >= '0' && text[index] <= '9')
    {
        has_digit = true;
        value = value * static_cast<T>(10) + static_cast<T>(text[index] - '0');
        ++index;
    }

    if (index < text.size() && text[index] == '.')
    {
        ++index;
        T scale = static_cast<T>(0.1);
        while (index < text.size() && text[index] >= '0' && text[index] <= '9')
        {
            has_digit = true;
            value += static_cast<T>(text[index] - '0') * scale;
            scale *= static_cast<T>(0.1);
            ++index;
        }
    }

    if (!has_digit)
    {
        return castle::status::invalid_argument;
    }

    int exponent = 0;
    bool exponent_negative = false;
    if (index < text.size() && (text[index] == 'e' || text[index] == 'E'))
    {
        ++index;
        if (index < text.size() && (text[index] == '+' || text[index] == '-'))
        {
            exponent_negative = text[index] == '-';
            ++index;
        }

        if (index == text.size() || text[index] < '0' || text[index] > '9')
        {
            return castle::status::invalid_argument;
        }

        while (index < text.size() && text[index] >= '0' && text[index] <= '9')
        {
            if (exponent > max_floating_exponent)
            {
                return castle::status::out_of_range;
            }
            exponent = exponent * 10 + (text[index] - '0');
            if (exponent > max_floating_exponent)
            {
                return castle::status::out_of_range;
            }
            ++index;
        }
    }

    if (index != text.size())
    {
        return castle::status::invalid_argument;
    }

    for (int i = 0; i < exponent; ++i)
    {
        if (exponent_negative)
        {
            value *= static_cast<T>(0.1);
        }
        else
        {
            value *= static_cast<T>(10);
        }
    }

    if (value > castle::numeric_limits<T>::max())
    {
        return castle::status::out_of_range;
    }

    output = negative ? -value : value;
    return castle::status::ok;
}

} // namespace detail

// ============================================================================
// document
// ----------------------------------------------------------------------------
// Owning, fixed-capacity representation of an INI document. Every key and
// value is stored in the document, so the input buffer may be transient.
// Entries keep insertion order; setting an existing section/key updates it in
// place. No heap allocation is performed.
//
// Every status/optional/bool-returning member is CASTLE_NODISCARD: silently
// dropping a capacity, lookup, or parse failure would leave a safety-critical
// caller acting on an unintended configuration.
// ============================================================================
template <castle::size_type MaxEntries = 32U,
          castle::size_type MaxSectionLength = 32U,
          castle::size_type MaxKeyLength = 32U,
          castle::size_type MaxValueLength = 128U>
class document
{
    static_assert(MaxEntries > 0U, "ini::document requires at least one entry");
    static_assert(MaxSectionLength > 0U, "ini::document requires a section capacity");
    static_assert(MaxKeyLength > 0U, "ini::document requires a key capacity");
    static_assert(MaxValueLength > 0U, "ini::document requires a value capacity");

    struct entry
    {
        castle::container::string<MaxSectionLength> section;
        castle::container::string<MaxKeyLength> key;
        castle::container::string<MaxValueLength> value;
    };

public:
    using size_type = castle::size_type;
    using value_type = castle::container::string<MaxValueLength>;
    using view_type = castle::container::string_view;

    static CASTLE_CONSTEXPR size_type static_capacity = MaxEntries;
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    document() CASTLE_NOEXCEPT CASTLE_DEFAULT;
    document(CASTLE_CONST document&) CASTLE_DEFAULT;
    document(document&&) CASTLE_NOEXCEPT CASTLE_DEFAULT;
    document& operator=(CASTLE_CONST document&) CASTLE_DEFAULT;
    document& operator=(document&&) CASTLE_NOEXCEPT CASTLE_DEFAULT;

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return entries_.size(); }
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxEntries; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return entries_.empty(); }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return entries_.full(); }

    void clear() CASTLE_NOEXCEPT
    {
        entries_.clear();
    }

    CASTLE_NODISCARD bool contains(view_type section, view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_index(section, key) != npos;
    }

    CASTLE_NODISCARD bool contains(view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return contains(view_type(), key);
    }

    CASTLE_NODISCARD bool contains_section(view_type section) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size(); ++i)
        {
            if (detail::equal_views(entries_[i].section.view(), section))
            {
                return true;
            }
        }
        return false;
    }

    CASTLE_NODISCARD castle::optional<view_type> find(view_type section, view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(section, key);
        if (index == npos)
        {
            return castle::optional<view_type>();
        }
        return castle::optional<view_type>(entries_[index].value.view());
    }

    CASTLE_NODISCARD castle::optional<view_type> find(view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find(view_type(), key);
    }

    CASTLE_NODISCARD castle::optional<view_type> get(view_type section, view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find(section, key);
    }

    CASTLE_NODISCARD castle::optional<view_type> get(view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find(view_type(), key);
    }

    CASTLE_NODISCARD castle::status read(view_type section, view_type key, view_type& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(section, key);
        if (index == npos)
        {
            output = view_type();
            return castle::status::not_found;
        }
        output = entries_[index].value.view();
        return castle::status::ok;
    }

    CASTLE_NODISCARD castle::status read(view_type key, view_type& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return read(view_type(), key, output);
    }

    CASTLE_NODISCARD castle::status set(view_type section, view_type key, view_type value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::status validation = validate_names(section, key);
        if (validation != castle::status::ok)
        {
            return validation;
        }
        if (value.size() > MaxValueLength)
        {
            return castle::status::full;
        }

        CASTLE_CONST size_type index = find_index(section, key);
        if (index != npos)
        {
            return entries_[index].value.assign(value);
        }

        if (entries_.full())
        {
            return castle::status::full;
        }

        castle::status status = entries_.emplace_back();
        if (status != castle::status::ok)
        {
            return status;
        }

        entry& added = entries_.back();
        status = added.section.assign(section);
        if (status != castle::status::ok)
        {
            entries_.pop_back();
            return status;
        }
        status = added.key.assign(key);
        if (status != castle::status::ok)
        {
            entries_.pop_back();
            return status;
        }
        status = added.value.assign(value);
        if (status != castle::status::ok)
        {
            entries_.pop_back();
            return status;
        }
        return castle::status::ok;
    }

    CASTLE_NODISCARD castle::status set(view_type key, view_type value) CASTLE_NOEXCEPT
    {
        return set(view_type(), key, value);
    }

    CASTLE_NODISCARD castle::status set_bool(view_type section, view_type key, bool value) CASTLE_NOEXCEPT
    {
        return set(section, key, value ? view_type("true") : view_type("false"));
    }

    CASTLE_NODISCARD castle::status set_bool(view_type key, bool value) CASTLE_NOEXCEPT
    {
        return set_bool(view_type(), key, value);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_integer_t<T> set_integer(view_type section, view_type key, T value) CASTLE_NOEXCEPT
    {
        castle::container::string<MaxValueLength> encoded;
        CASTLE_CONST castle::status status = detail::format_integer_value(value, encoded);
        if (status != castle::status::ok)
        {
            return status;
        }
        if (encoded.size() > MaxValueLength)
        {
            return castle::status::full;
        }
        return set(section, key, encoded.view());
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_integer_t<T> set_integer(view_type key, T value) CASTLE_NOEXCEPT
    {
        return set_integer(view_type(), key, value);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_enum_t<T> set_enum(view_type section, view_type key, T value) CASTLE_NOEXCEPT
    {
        using underlying_type = meta::underlying_type_t<T>;
        return set_integer(section, key, static_cast<underlying_type>(value));
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_enum_t<T> set_enum(view_type key, T value) CASTLE_NOEXCEPT
    {
        return set_enum(view_type(), key, value);
    }

    CASTLE_NODISCARD castle::status get_bool(view_type section, view_type key, bool& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type value;
        CASTLE_CONST castle::status status = read(section, key, value);
        if (status != castle::status::ok)
        {
            return status;
        }

        if (detail::equals_ignore_case(value, view_type("true")) ||
            detail::equals_ignore_case(value, view_type("yes")) ||
            detail::equals_ignore_case(value, view_type("on")) ||
            detail::equal_views(value, view_type("1")))
        {
            output = true;
            return castle::status::ok;
        }
        if (detail::equals_ignore_case(value, view_type("false")) ||
            detail::equals_ignore_case(value, view_type("no")) ||
            detail::equals_ignore_case(value, view_type("off")) ||
            detail::equal_views(value, view_type("0")))
        {
            output = false;
            return castle::status::ok;
        }
        return castle::status::invalid_argument;
    }

    CASTLE_NODISCARD castle::status get_bool(view_type key, bool& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return get_bool(view_type(), key, output);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_integer_t<T> get_integer(view_type section, view_type key, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type value;
        CASTLE_CONST castle::status status = read(section, key, value);
        if (status != castle::status::ok)
        {
            return status;
        }
        return detail::parse_integer_value(value, output);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_integer_t<T> get_integer(view_type key, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return get_integer(view_type(), key, output);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_enum_t<T> get_enum(view_type section, view_type key, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        using underlying_type = meta::underlying_type_t<T>;
        underlying_type value;
        CASTLE_CONST castle::status status = get_integer(section, key, value);
        if (status != castle::status::ok)
        {
            return status;
        }
        output = static_cast<T>(value);
        return castle::status::ok;
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_enum_t<T> get_enum(view_type key, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return get_enum(view_type(), key, output);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_float_t<T> get_floating(view_type section, view_type key, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type value;
        CASTLE_CONST castle::status status = read(section, key, value);
        if (status != castle::status::ok)
        {
            return status;
        }
        return detail::parse_floating_value(value, output);
    }

    template <typename T>
    CASTLE_NODISCARD detail::enable_float_t<T> get_floating(view_type key, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return get_floating(view_type(), key, output);
    }

    CASTLE_NODISCARD castle::status remove(view_type section, view_type key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = find_index(section, key);
        if (index == npos)
        {
            return castle::status::not_found;
        }

        erase_index(index);
        return castle::status::ok;
    }

    CASTLE_NODISCARD castle::status remove(view_type key) CASTLE_NOEXCEPT
    {
        return remove(view_type(), key);
    }

    CASTLE_NODISCARD castle::status remove_section(view_type section) CASTLE_NOEXCEPT
    {
        bool removed = false;
        size_type index = entries_.size();
        while (index > 0U)
        {
            --index;
            if (detail::equal_views(entries_[index].section.view(), section))
            {
                erase_index(index);
                removed = true;
            }
        }
        return removed ? castle::status::ok : castle::status::not_found;
    }

    view_type section(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index < size() ? entries_[index].section.view() : view_type();
    }

    view_type key(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index < size() ? entries_[index].key.view() : view_type();
    }

    view_type value(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index < size() ? entries_[index].value.view() : view_type();
    }

private:
    castle::status validate_names(view_type section_name, view_type key_name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!detail::valid_section_name(section_name))
        {
            return castle::status::invalid_argument;
        }
        if (!detail::valid_key_name(key_name))
        {
            return castle::status::invalid_argument;
        }
        if (section_name.size() > MaxSectionLength || key_name.size() > MaxKeyLength)
        {
            return castle::status::full;
        }
        return castle::status::ok;
    }

    size_type find_index(view_type section_name, view_type key_name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size(); ++i)
        {
            if (detail::equal_views(entries_[i].section.view(), section_name) && detail::equal_views(entries_[i].key.view(), key_name))
            {
                return i;
            }
        }
        return npos;
    }

    void erase_index(size_type index) CASTLE_NOEXCEPT
    {
        for (size_type i = index; i + 1U < entries_.size(); ++i)
        {
            entries_[i] = entries_[i + 1U];
        }
        entries_.pop_back();
    }

    castle::container::vector<entry, MaxEntries> entries_;
};

// ============================================================================
// parse
// ----------------------------------------------------------------------------
// Parses a complete in-memory INI document. The document is cleared before
// parsing and is cleared again on failure, so a failed parse never exposes a
// partially parsed configuration.
// ============================================================================
namespace detail
{

template <castle::size_type MaxEntries,
          castle::size_type MaxSectionLength,
          castle::size_type MaxKeyLength,
          castle::size_type MaxValueLength>
result parse_document(
    document<MaxEntries, MaxSectionLength, MaxKeyLength, MaxValueLength>& output,
    string_view input) CASTLE_NOEXCEPT
{
    result result_value;
    castle::container::string<MaxSectionLength> current_section;
    output.clear();

    castle::size_type position = 0U;
    castle::size_type line_number = 1U;

    while (position < input.size() || (input.empty() && line_number == 1U))
    {
        CASTLE_CONST castle::size_type line_start = position;
        while (position < input.size() && !is_line_end(input[position]))
        {
            ++position;
        }

        CASTLE_CONST castle::size_type line_length = position - line_start;
        CASTLE_CONST string_view line = line_length == 0U
                                        ? string_view()
                                        : string_view(input.data() + line_start, line_length);
        string_view trimmed = trim(line);

        if (!trimmed.empty() && !starts_with_comment(trimmed))
        {
            if (trimmed.front() == '[')
            {
                CASTLE_CONST castle::size_type close = trimmed.find(']');
                if (close == string_view::npos)
                {
                    result_value.status = castle::status::invalid_argument;
                    result_value.code = error_code::invalid_section;
                    result_value.line = line_number;
                    result_value.column = 1U;
                    output.clear();
                    return result_value;
                }

                CASTLE_CONST string_view suffix = trim(trimmed.substr(close + 1U));
                if (!suffix.empty() && !starts_with_comment(suffix))
                {
                    result_value.status = castle::status::invalid_argument;
                    result_value.code = error_code::invalid_section;
                    result_value.line = line_number;
                    result_value.column = close + 2U;
                    output.clear();
                    return result_value;
                }

                string_view section_name = trim(trimmed.substr(1U, close - 1U));
                if (section_name.empty() || !valid_section_name(section_name) ||
                    section_name.size() > MaxSectionLength)
                {
                    result_value.status = section_name.size() > MaxSectionLength
                                          ? castle::status::full
                                          : castle::status::invalid_argument;
                    result_value.code = section_name.size() > MaxSectionLength
                                        ? error_code::capacity
                                        : error_code::invalid_section;
                    result_value.line = line_number;
                    result_value.column = 2U;
                    output.clear();
                    return result_value;
                }

                CASTLE_CONST castle::status status = current_section.assign(section_name);
                if (status != castle::status::ok)
                {
                    result_value.status = status;
                    result_value.code = error_code::capacity;
                    result_value.line = line_number;
                    result_value.column = 2U;
                    output.clear();
                    return result_value;
                }
            }
            else
            {
                CASTLE_CONST castle::size_type separator = find_separator(trimmed);
                if (separator == string_view::npos)
                {
                    result_value.status = castle::status::invalid_argument;
                    result_value.code = error_code::invalid_assignment;
                    result_value.line = line_number;
                    result_value.column = 1U;
                    output.clear();
                    return result_value;
                }

                CASTLE_CONST string_view key = trim(trimmed.substr(0U, separator));
                if (!valid_key_name(key) || key.size() > MaxKeyLength)
                {
                    result_value.status = key.size() > MaxKeyLength
                                          ? castle::status::full
                                          : castle::status::invalid_argument;
                    result_value.code = key.size() > MaxKeyLength
                                        ? error_code::capacity
                                        : error_code::invalid_key;
                    result_value.line = line_number;
                    result_value.column = 1U;
                    output.clear();
                    return result_value;
                }

                castle::container::string<MaxValueLength> decoded_value;
                CASTLE_CONST result decoded = decode_value<MaxValueLength>(
                    trimmed.substr(separator + 1U),
                    decoded_value,
                    line_number,
                    separator + 2U);
                if (!decoded.succeeded())
                {
                    output.clear();
                    return decoded;
                }

                CASTLE_CONST castle::status status = output.set(
                    current_section.view(),
                    key,
                    decoded_value.view());
                if (status != castle::status::ok)
                {
                    result_value.status = status;
                    result_value.code = status == castle::status::full
                                        ? error_code::capacity
                                        : error_code::invalid_assignment;
                    result_value.line = line_number;
                    result_value.column = separator + 1U;
                    output.clear();
                    return result_value;
                }
            }
        }

        if (position >= input.size())
        {
            break;
        }

        if (input[position] == '\r' && position + 1U < input.size() && input[position + 1U] == '\n')
        {
            position += 2U;
        }
        else
        {
            ++position;
        }
        ++line_number;
    }

    return result_value;
}

} // namespace detail

// Concrete overload avoids weakening type safety with a generic parser entry.
template <castle::size_type MaxEntries,
          castle::size_type MaxSectionLength,
          castle::size_type MaxKeyLength,
          castle::size_type MaxValueLength>
CASTLE_NODISCARD result parse(
    document<MaxEntries, MaxSectionLength, MaxKeyLength, MaxValueLength>& output,
    string_view input) CASTLE_NOEXCEPT
{
    return detail::parse_document(output, input);
}

// ============================================================================
// serialize
// ----------------------------------------------------------------------------
// Produces deterministic INI text into any castle::container::string<N>. Global
// entries are emitted first, followed by sections in first-seen order. Entries
// within a section retain document insertion order.
// ============================================================================
template <castle::size_type OutputCapacity,
          castle::size_type MaxEntries,
          castle::size_type MaxSectionLength,
          castle::size_type MaxKeyLength,
          castle::size_type MaxValueLength>
CASTLE_NODISCARD result serialize(
    CASTLE_CONST document<MaxEntries, MaxSectionLength, MaxKeyLength, MaxValueLength>& input,
    castle::container::string<OutputCapacity>& output) CASTLE_NOEXCEPT
{
    result result_value;
    output.clear();

    CASTLE_CONST auto append_line = [&output](string_view key, string_view value) CASTLE_NOEXCEPT -> castle::status
    {
        if (output.append(key) != castle::status::ok ||
            output.append(string_view(" = ")) != castle::status::ok)
        {
            return castle::status::full;
        }
        CASTLE_CONST castle::status value_status = detail::append_escaped_value(value, output);
        if (value_status != castle::status::ok)
        {
            return value_status;
        }
        return output.push_back('\n');
    };

    // Global scope first.
    for (castle::size_type i = 0U; i < input.size(); ++i)
    {
        if (input.section(i).empty())
        {
            CASTLE_CONST castle::status status = append_line(input.key(i), input.value(i));
            if (status != castle::status::ok)
            {
                result_value.status = status;
                result_value.code = error_code::output_full;
                return result_value;
            }
        }
    }

    bool wrote_global = false;
    for (castle::size_type i = 0U; i < input.size(); ++i)
    {
        if (input.section(i).empty())
        {
            wrote_global = true;
            break;
        }
    }

    if (wrote_global)
    {
        if (output.push_back('\n') != castle::status::ok)
        {
            result_value.status = castle::status::full;
            result_value.code = error_code::output_full;
            return result_value;
        }
    }

    // Sections are emitted once, in the order in which the first entry from
    // each section was inserted. The bounded O(N^2) scan is intentional: it
    // avoids a second dynamic/fixed-capacity map solely for serialization.
    for (castle::size_type i = 0U; i < input.size(); ++i)
    {
        CASTLE_CONST string_view section_name = input.section(i);
        if (section_name.empty())
        {
            continue;
        }

        bool already_emitted = false;
        for (castle::size_type previous = 0U; previous < i; ++previous)
        {
            if (detail::equal_views(input.section(previous), section_name))
            {
                already_emitted = true;
                break;
            }
        }
        if (already_emitted)
        {
            continue;
        }

        if (output.append(string_view("[")) != castle::status::ok ||
            output.append(section_name) != castle::status::ok ||
            output.append(string_view("]\n")) != castle::status::ok)
        {
            result_value.status = castle::status::full;
            result_value.code = error_code::output_full;
            return result_value;
        }

        for (castle::size_type entry_index = 0U; entry_index < input.size(); ++entry_index)
        {
            if (detail::equal_views(input.section(entry_index), section_name))
            {
                CASTLE_CONST castle::status status = append_line(
                    input.key(entry_index),
                    input.value(entry_index));
                if (status != castle::status::ok)
                {
                    result_value.status = status;
                    result_value.code = error_code::output_full;
                    return result_value;
                }
            }
        }

        if (output.push_back('\n') != castle::status::ok)
        {
            result_value.status = castle::status::full;
            result_value.code = error_code::output_full;
            return result_value;
        }
    }

    return result_value;
}

// Short alias matching the common serialization verb.
template <castle::size_type OutputCapacity,
          castle::size_type MaxEntries,
          castle::size_type MaxSectionLength,
          castle::size_type MaxKeyLength,
          castle::size_type MaxValueLength>
CASTLE_NODISCARD result write(
    CASTLE_CONST document<MaxEntries, MaxSectionLength, MaxKeyLength, MaxValueLength>& input,
    castle::container::string<OutputCapacity>& output) CASTLE_NOEXCEPT
{
    return serialize(input, output);
}

} // namespace ini
} // namespace serialization
} // namespace castle

#endif // CASTLE_SERIALIZATION_INI_HPP
