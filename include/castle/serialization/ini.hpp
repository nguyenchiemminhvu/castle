// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-capacity INI document, parser, and serializer for embedded configuration data.
 *
 * Use this header when configuration text must be parsed into an owning document with
 * deterministic storage and no heap allocation. Supported syntax is limited to Castle's
 * implemented subset: sections, global keys, `=` or `:` assignments, full-line `#` and
 * `;` comments, quoted or unquoted values, and a small escape set for quoted values.
 *
 * @code
 * #include "castle/serialization/ini.hpp"
 *
 * using config_t = castle::serialization::ini::document<16U, 24U, 32U, 96U>;
 * config_t config;
 * auto result = castle::serialization::ini::parse(config, "name = castle");
 * (void)result;
 * @endcode
 */
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

/**
 * @brief INI-specific diagnostics paired with `castle::status`.
 */
enum class error_code : uint8_t
{
    none = 0U,            /**< No format-specific error. */
    invalid_section,      /**< A section header is malformed, empty, or contains invalid trailing text. */
    invalid_key,          /**< A key name is empty or contains invalid characters. */
    invalid_assignment,   /**< A key/value assignment is malformed or missing a separator. */
    invalid_quote,        /**< A quoted value is unterminated or otherwise malformed. */
    invalid_escape,       /**< A quoted value contains an unsupported escape sequence. */
    capacity,             /**< Fixed-capacity document or temporary storage was exhausted. */
    output_full           /**< Serialization output buffer is too small. */
};

/**
 * @brief Full INI parse or serialization result.
 */
struct result
{
    castle::status status = castle::status::ok; /**< Generic Castle status result. */
    error_code code = error_code::none; /**< INI-specific diagnostic code. */
    castle::size_type line = 0U; /**< One-based line number for parse errors, otherwise zero. */
    castle::size_type column = 0U; /**< One-based column number for parse errors, otherwise zero. */

    /**
     * @brief Tests whether the operation succeeded.
     *
     * @return `true` when `status == castle::status::ok`; otherwise `false`.
     */
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

/**
 * @brief Tests whether a character is considered whitespace in an INI file.
 * @param value The character to test.
 * @return `true` if the character is considered whitespace; otherwise `false`.
 */
inline bool is_space(char value) CASTLE_NOEXCEPT
{
    return value == ' ' || value == '\t' || value == '\v' || value == '\f'; // LCOV_EXCL_BR_LINE
}

/**
 * @brief Tests whether a character is considered a line ending in an INI file.
 * @param value The character to test.
 * @return `true` if the character is considered a line ending; otherwise `false`.
 */
inline bool is_line_end(char value) CASTLE_NOEXCEPT
{
    return value == '\r' || value == '\n'; // LCOV_EXCL_BR_LINE
}

/**
 * @brief Converts an ASCII character to its lowercase equivalent.
 * @param value The ASCII character to convert.
 * @return The lowercase equivalent of the ASCII character.
 */
inline char lower_ascii(char value) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    return (value >= 'A' && value <= 'Z')
           ? static_cast<char>(value - 'A' + 'a')
           : value;
    // LCOV_EXCL_STOP
}

/**
 * @brief Compares two string views for equality.
 * @param lhs The first string view to compare.
 * @param rhs The second string view to compare.
 * @return `true` if the string views are equal; otherwise `false`.
 */
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

/**
 * @brief Compares two string views for equality, ignoring case.
 * @param lhs The first string view to compare.
 * @param rhs The second string view to compare.
 * @return `true` if the string views are equal, ignoring case; otherwise `false`.
 */
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

/**
 * @brief Trims leading and trailing whitespace and line endings from a string view.
 * @param value The string view to trim.
 * @return The trimmed string view.
 * @note This function only trims ASCII whitespace and line ending characters.
 */
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

/**
 * @brief Checks if a line starts with a comment character.
 * @param line The line to check.
 * @return `true` if the line starts with a comment character; otherwise `false`.
 */
inline bool starts_with_comment(string_view line) CASTLE_NOEXCEPT
{
    return !line.empty() && (line[0U] == '#' || line[0U] == ';');
}

/**
 * @brief Checks if a section name is valid.
 * @param section The section name to check.
 * @return `true` if the section name is valid; otherwise `false`.
 */
inline bool valid_section_name(string_view section) CASTLE_NOEXCEPT
{
    if (section.empty())
    {
        return true;
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

/**
 * @brief Checks if a key name is valid.
 * @param key The key name to check.
 * @return `true` if the key name is valid; otherwise `false`.
 */
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

/**
 * @brief Finds the position of the key-value separator in a line.
 * @param line The line to search.
 * @return The position of the separator if found; otherwise `string_view::npos`.
 */
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

/**
 * @brief Checks if a value needs to be quoted.
 * @param value The value to check.
 * @return `true` if the value needs to be quoted; otherwise `false`.
 */
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
        // LCOV_EXCL_START
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
        // LCOV_EXCL_STOP
    }
    return false;
}

/**
 * @brief Checks if a character is a valid escape sequence in an INI value.
 * @param value The character to check.
 * @return `true` if the character is a valid escape sequence; otherwise `false`.
 */
inline bool valid_escape(char value) CASTLE_NOEXCEPT
{
    return value == '"' || value == '\\' || value == 'n' || value == 'r' || value == 't';
}

/**
 * @brief Decodes an escape sequence character in an INI value.
 * @param value The escape sequence character to decode.
 * @return The decoded character.
 * @note If the character is not a valid escape sequence, it is returned as-is.
 */
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

/**
 * @brief Decodes a raw INI value into a properly formatted string.
 * @param raw The raw INI value to decode.
 * @param output The container to store the decoded value.
 * @param line The line number of the raw value in the INI file.
 * @param column The column number of the raw value in the INI file.
 * @return A `result` object indicating the success or failure of the decoding operation.
 */
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

/**
 * @brief Appends an escaped INI value to the output container.
 * @param value The INI value to escape and append.
 * @param output The container to store the escaped value.
 * @return The status of the append operation.
 */
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

/**
 * @brief Parses an integer value from a string.
 * @param text The string containing the integer representation.
 * @param output Reference to the variable where the parsed integer will be stored.
 * @return Status indicating the result of the parsing operation.
 */
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

/**
 * @brief Formats an integer value as a string.
 * @tparam T Type of the integer value to format.
 * @param value The integer value to format.
 * @param output Reference to the string where the formatted integer will be stored.
 * @return Status indicating the result of the formatting operation.
 * @note The output string must have sufficient capacity to hold the formatted integer.
 */
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

/**
 * @brief Parses a floating-point value from a string.
 * @tparam T Type of the floating-point value to parse.
 * @param text The string containing the floating-point representation.
 * @param output Reference to the variable where the parsed floating-point value will be stored.
 * @return Status indicating the result of the parsing operation.
 * @note The input string must represent a valid floating-point number within the range of the type `T`.
 */
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

}

/**
 * @brief Owning fixed-capacity INI document.
 *
 * @tparam MaxEntries Maximum number of stored key/value entries.
 * @tparam MaxSectionLength Maximum stored section-name length.
 * @tparam MaxKeyLength Maximum stored key-name length.
 * @tparam MaxValueLength Maximum stored value length after decoding.
 *
 * @note Every entry owns its section, key, and value so the input buffer may be released
 * after parsing.
 */
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
    /** @brief Alias for the document size type. */
    using size_type = castle::size_type;
    /** @brief Alias for owned value storage. */
    using value_type = castle::container::string<MaxValueLength>;
    /** @brief Alias for non-owning string views returned by lookups. */
    using view_type = castle::container::string_view;

    /** @brief Compile-time maximum number of entries. */
    static CASTLE_CONSTEXPR size_type static_capacity = MaxEntries;
    /** @brief Sentinel used when a lookup does not find an entry index. */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /** @brief Constructs an empty document. */
    document() CASTLE_NOEXCEPT CASTLE_DEFAULT;
    /** @brief Copies another document. */
    document(CASTLE_CONST document&) CASTLE_DEFAULT;
    /** @brief Moves another document. */
    document(document&&) CASTLE_NOEXCEPT CASTLE_DEFAULT;
    /** @brief Copy-assigns another document. */
    document& operator=(CASTLE_CONST document&) CASTLE_DEFAULT;
    /** @brief Move-assigns another document. */
    document& operator=(document&&) CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /** @brief Returns the number of stored entries. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return entries_.size(); }
    /** @brief Returns the maximum number of entries this document can store. */
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxEntries; }
    /** @brief Tests whether the document contains no entries. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return entries_.empty(); }
    /** @brief Tests whether the document has reached its entry capacity. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return entries_.full(); }

    /**
     * @brief Removes every stored entry.
     */
    void clear() CASTLE_NOEXCEPT
    {
        entries_.clear();
    }

    /**
     * @brief Tests whether a section/key pair exists.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to search for.
     * @return `true` when the entry exists; otherwise `false`.
     */
    CASTLE_NODISCARD bool contains(view_type section, view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_index(section, key) != npos;
    }

    /**
     * @brief Tests whether a key exists in the global scope.
     *
     * @param key Key name to search for.
     * @return `true` when the key exists in the global scope; otherwise `false`.
     * @note The key is searched for in the global scope only.
     */
    CASTLE_NODISCARD bool contains(view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return contains(view_type(), key);
    }

    /**
     * @brief Tests whether any entry exists in a section.
     *
     * @param section Section name to search for.
     * @return `true` when at least one entry belongs to the section; otherwise `false`.
     */
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

    /**
     * @brief Finds the value associated with a section/key pair.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to search for.
     * @return Optional view of the stored value when present; otherwise an empty optional.
     */
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

    /**
     * @brief Returns the value associated with a section/key pair.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to search for.
     * @return Optional view of the stored value when present; otherwise an empty optional.
     */
    CASTLE_NODISCARD castle::optional<view_type> get(view_type section, view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find(section, key);
    }

    CASTLE_NODISCARD castle::optional<view_type> get(view_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find(view_type(), key);
    }

    /**
     * @brief Reads the value associated with a section/key pair into an output view.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to search for.
     * @param output Receives the stored value view on success, or an empty view on failure.
     * @return `castle::status::ok` on success or `castle::status::not_found` when absent.
     */
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

    /**
     * @brief Inserts or updates a string value.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to create or update.
     * @param value Value text to store.
     * @return `castle::status::ok` on success, or a failure describing invalid names or exhausted capacity.
     */
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

    /**
     * @brief Inserts or updates a boolean value encoded as `true` or `false`.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to create or update.
     * @param value Boolean value to store.
     * @return `castle::status::ok` on success, or a failure describing invalid names or exhausted capacity.
     */
    CASTLE_NODISCARD castle::status set_bool(view_type section, view_type key, bool value) CASTLE_NOEXCEPT
    {
        return set(section, key, value ? view_type("true") : view_type("false"));
    }

    CASTLE_NODISCARD castle::status set_bool(view_type key, bool value) CASTLE_NOEXCEPT
    {
        return set_bool(view_type(), key, value);
    }

    /**
     * @brief Inserts or updates an integral value encoded as decimal text.
     *
     * @tparam T Integral type excluding `bool`.
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to create or update.
     * @param value Integer value to store.
     * @return `castle::status::ok` on success, or a failure describing invalid names, formatting limits, or exhausted capacity.
     */
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

    /**
     * @brief Inserts or updates an enum value encoded through its underlying integer type.
     *
     * @tparam T Enum type.
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to create or update.
     * @param value Enum value to store.
     * @return `castle::status::ok` on success, or a failure describing invalid names, formatting limits, or exhausted capacity.
     */
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

    /**
     * @brief Reads a stored value as a boolean.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to read.
     * @param output Receives the decoded boolean on success.
     * @return `castle::status::ok` on success, `not_found` when absent, or `invalid_argument` for unsupported text.
     */
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

    /**
     * @brief Reads a stored value as an integer.
     *
     * @tparam T Integral type excluding `bool`.
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to read.
     * @param output Receives the decoded integer on success.
     * @return `castle::status::ok` on success, `not_found` when absent, or a conversion failure status.
     */
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

    /**
     * @brief Reads a stored value as an enum through its underlying integer type.
     *
     * @tparam T Enum type.
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to read.
     * @param output Receives the decoded enum value on success.
     * @return `castle::status::ok` on success, `not_found` when absent, or a conversion failure status.
     */
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

    /**
     * @brief Reads a stored value as a floating-point number.
     *
     * @tparam T Floating-point type.
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to read.
     * @param output Receives the decoded floating-point value on success.
     * @return `castle::status::ok` on success, `not_found` when absent, or a conversion failure status.
     */
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

    /**
     * @brief Removes one section/key entry.
     *
     * @param section Section name, or an empty view for the global scope.
     * @param key Key name to remove.
     * @return `castle::status::ok` on success or `castle::status::not_found` when absent.
     */
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

    /**
     * @brief Removes every entry in a section.
     *
     * @param section Section name to remove.
     * @return `castle::status::ok` when at least one entry was removed; otherwise `castle::status::not_found`.
     */
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

    /**
     * @brief Returns the section name stored at an index.
     *
     * @param index Entry index.
     * @return Section view for the entry, or an empty view when `index` is out of range.
     */
    view_type section(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index < size() ? entries_[index].section.view() : view_type();
    }

    /**
     * @brief Returns the key name stored at an index.
     *
     * @param index Entry index.
     * @return Key view for the entry, or an empty view when `index` is out of range.
     */
    view_type key(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index < size() ? entries_[index].key.view() : view_type();
    }

    /**
     * @brief Returns the value stored at an index.
     *
     * @param index Entry index.
     * @return Value view for the entry, or an empty view when `index` is out of range.
     */
    view_type value(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index < size() ? entries_[index].value.view() : view_type();
    }

private:
    /**
     * @brief Validates the section and key names.
     * @param section_name Section name to validate.
     * @param key_name Key name to validate.
     * @return Status indicating whether the section and key names are valid.
     * @note The section and key names must conform to the constraints defined by `MaxSectionLength` and `MaxKeyLength`.
     */
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

    /**
     * @brief Finds the index of an entry with the specified section and key names.
     * @param section_name Section name of the entry to find.
     * @param key_name Key name of the entry to find.
     * @return Index of the entry if found, or `npos` if not found.
     * @note The search is performed in a case-sensitive manner.
     */
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

    /**
     * @brief Erases the entry at the specified index.
     * @param index Index of the entry to erase.
     * @note If the index is out of range, the behavior is undefined.
     * @warning The caller must ensure that the index is valid before calling this function.
     */
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

namespace detail
{

/**
 * @brief Parses an INI document from the given input string and populates the output document.
 * @param output The document object to populate with the parsed entries.
 * @param input The input string containing the INI document to parse.
 * @return A `result` object indicating the success or failure of the parsing operation.
 * @note The parsing is performed in a line-by-line manner, and any syntax errors will result in an invalid argument status.
 */
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

}

/**
 * @brief Parses a complete INI document from memory.
 *
 * @tparam MaxEntries Maximum number of stored entries.
 * @tparam MaxSectionLength Maximum stored section-name length.
 * @tparam MaxKeyLength Maximum stored key-name length.
 * @tparam MaxValueLength Maximum stored value length after decoding.
 * @param output Destination document cleared before parsing and cleared again on failure.
 * @param input Source INI text.
 * @return Detailed parse result with `castle::status`, `error_code`, and line/column diagnostics.
 */
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

/**
 * @brief Serializes a document into INI text.
 *
 * @tparam OutputCapacity Output-string capacity.
 * @tparam MaxEntries Maximum number of stored entries.
 * @tparam MaxSectionLength Maximum stored section-name length.
 * @tparam MaxKeyLength Maximum stored key-name length.
 * @tparam MaxValueLength Maximum stored value length.
 * @param input Document to serialize.
 * @param output Destination string cleared before writing.
 * @return Detailed serialization result. `error_code::output_full` reports insufficient output capacity.
 */
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
        // LCOV_EXCL_START
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
        // LCOV_EXCL_STOP
    };


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

        // LCOV_EXCL_START
        if (output.append(string_view("[")) != castle::status::ok ||
            output.append(section_name) != castle::status::ok ||
            output.append(string_view("]\n")) != castle::status::ok)
        {
            result_value.status = castle::status::full;
            result_value.code = error_code::output_full;
            return result_value;
        }
        // LCOV_EXCL_STOP

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

/**
 * @brief Alias of `serialize()`.
 *
 * @tparam OutputCapacity Output-string capacity.
 * @tparam MaxEntries Maximum number of stored entries.
 * @tparam MaxSectionLength Maximum stored section-name length.
 * @tparam MaxKeyLength Maximum stored key-name length.
 * @tparam MaxValueLength Maximum stored value length.
 * @param input Document to serialize.
 * @param output Destination string cleared before writing.
 * @return Result of `serialize(input, output)`.
 */
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

}
}
}

#endif
