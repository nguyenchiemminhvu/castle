// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-capacity CSV document, parser, and serializer for embedded configuration data.
 *
 * Use this header when configuration text must be parsed into an owning document with
 * deterministic storage and no heap allocation.
 *
 * @code
 * #include "castle/serialization/csv.hpp"
 * 
 * auto result = castle::serialization::csv::parse(config, "name,age\ncastle,42");
 * if (result.succeeded())
 * {
 *     // Handle successful parsing
 * }
 * else
 * {
 *     // Handle parsing error
 * }
 * @endcode
 */

#ifndef CASTLE_SERIALIZATION_CSV_HPP
#define CASTLE_SERIALIZATION_CSV_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"

#include <float.h>
#include <stdint.h>

namespace castle
{
namespace serialization
{
namespace csv
{

using string_view = castle::container::string_view;

/**
* @brief Parser and serializer error codes.
*
* Used by result::code to describe the reason an operation failed.
*
* @see result
*/
enum class error_code : uint8_t
{
    none = 0U,             ///< No error occurred; operation succeeded.
    unexpected_end,        ///< The input ended prematurely before parsing was complete.
    invalid_quote,         ///< Encountered an unclosed or misplaced quotation mark.
    invalid_character,     ///< Found a character that violates syntax rules.
    invalid_delimiter,     ///< Missing or incorrectly placed field delimiter.
    inconsistent_columns,  ///< The row contains a different number of columns than expected.
    capacity,              ///< Internal capacity limit or buffer size exceeded.
    output_full,           ///< The destination output buffer runs out of available space.
    invalid_argument       ///< An invalid parameter or argument was passed to the function.
};

/**
* @brief Result of a CSV parsing or serialization operation.
*
* Contains the operation status, an optional CSV-specific error code,
* and source location information when a parsing error is reported.
*
* For parsing failures:
* - offset is a zero-based byte position.
* - line and column are one-based coordinates.
*
* For serialization failures:
* - offset, line, and column are zero.
*/
struct result
{
    castle::status status = castle::status::ok;
    error_code code = error_code::none;
    castle::size_type offset = 0U;
    castle::size_type line = 0U;
    castle::size_type column = 0U;

    /// @brief Checks if the parsing operation succeeded.
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool succeeded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::succeeded(status);
    }
};

/**
* @brief Configuration options for CSV parsing and serialization.
*
* The same structure is used by both parse() and serialize().
*/
struct options
{
    /**
    * @brief Field delimiter character.
    *
    * Must not be:
    * - '\0'
    * - '"'
    * - '\r'
    * - '\n'
    */
    char delimiter = ',';

    /**
    * @brief Enforces equal column counts for all rows during parsing.
    *
    * When enabled, every record must contain the same number of
    * columns as the first record.
    */
    bool strict_columns = false;
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

/// @brief Checks if a character represents a line ending (CR or LF).
CASTLE_INLINE bool is_line_end(char value) CASTLE_NOEXCEPT
{
    return value == '\r' || value == '\n'; // LCOV_EXCL_BR_LINE
}

/// @brief Checks if a character is a valid field delimiter.
CASTLE_INLINE bool is_valid_delimiter(char value) CASTLE_NOEXCEPT
{
    return value != '\0' && value != '"' && !is_line_end(value); // LCOV_EXCL_BR_LINE
}

/// @brief Converts an ASCII character to its lowercase equivalent.
CASTLE_INLINE char lower_ascii(char value) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    return (value >= 'A' && value <= 'Z')
           ? static_cast<char>(value - 'A' + 'a')
           : value;
    // LCOV_EXCL_STOP
}

/**
* @brief Compares two string views for equality.
*
* @param lhs Left-hand operand.
* @param rhs Right-hand operand.
*
* @return true if both views contain identical characters.
* @return false otherwise.
*/
CASTLE_INLINE bool equal_views(string_view lhs, string_view rhs) CASTLE_NOEXCEPT
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
 * @brief Compares two string_view instances using ASCII case folding.
 * 
 * This function determines if two `std::string_view` instances are equal by performing
 * a case-insensitive comparison of their characters. It only applies ASCII case folding.
 * 
 * @param lhs The left-hand side string view to compare.
 * @param rhs The right-hand side string view to compare.
 * @return true If both strings have the same length and matching characters (case-insensitive).
 * @return false If the strings differ in length or contain non-matching characters.
 */
CASTLE_INLINE bool equals_ignore_case(string_view lhs, string_view rhs) CASTLE_NOEXCEPT
{
    if (lhs.size() != rhs.size()) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    for (castle::size_type i = 0U; i < lhs.size(); ++i)
    {
        // LCOV_EXCL_START
        if (lower_ascii(lhs[i]) != lower_ascii(rhs[i]))
        {
            return false;
        }
        // LCOV_EXCL_STOP
    }
    return true;
}

/**
* @brief Checks whether a string contains embedded null characters.
*
* @param value String to inspect.
*
* @return true if at least one '\0' character is present.
* @return false otherwise.
*/
CASTLE_INLINE bool contains_nul(string_view value) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] == '\0')
        {
            return true;
        }
    }
    // LCOV_EXCL_STOP

    return false;
}

/**
 * @brief Parses a string view into an integer of type T.
 * 
 * This function processes a given text representation of an integer (supporting optional 
 * signs and decimal digits), handles arithmetic overflow/underflow checks, and populates 
 * the output parameter upon successful extraction.
 * 
 * @tparam T The target integer type to parse into (signed or unsigned).
 * @param[in] text The string view containing the numeric characters to be parsed.
 * @param[out] output A reference to the variable where the successfully parsed value will be stored.
 * @return castle::status The execution outcome:
 *         - `castle::status::ok` on success.
 *         - `castle::status::invalid_argument` if the input is empty, malformed, or contains a negative sign when parsing an unsigned type.
 *         - `castle::status::out_of_range` if the parsed number exceeds the limits of type T or a 64-bit unsigned magnitude accumulator.
 * 
 * @note This function is marked `CASTLE_NOEXCEPT` and does not throw exceptions.
 */
template <typename T>
enable_integer_t<T> parse_integer(string_view text, T& output) CASTLE_NOEXCEPT
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
        if (c < '0' || c > '9') // LCOV_EXCL_BR_LINE
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
        if (magnitude > max_unsigned
         || (meta::is_signed<T>::value && magnitude > (max_unsigned >> 1U)))
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
 * @brief Formats an integer of type T into its string representation.
 * 
 * This function converts an integer value into a decimal string representation, appending the digits 
 * directly to a fixed-capacity or bounded string container. It automatically handles sign extraction 
 * for signed integer types (including negative minimum edge cases) without provoking undefined behavior.
 * 
 * @tparam T The source integer type to format (signed or unsigned).
 * @tparam MaxValueLength The maximum allocation storage capacity of the destination string. 
 *         Defaults to `serialization::max_integer_str_len`.
 * @param[in] value The integer value to be converted to a string.
 * @param[out] output A reference to the destination string container where the formatted result is written. 
 *             The string is cleared at the beginning of the function execution.
 * @return castle::status The execution outcome:
 *         - `castle::status::ok` on success.
 *         - `castle::status::full` if the target string container reaches its capacity boundary while appending digits or the sign.
 * 
 * @note This function is marked `CASTLE_NOEXCEPT` and guarantees exception safety.
 */
template <typename T, castle::size_type MaxValueLength = serialization::max_integer_str_len>
enable_integer_t<T> format_integer(T value, castle::container::string<MaxValueLength>& output) CASTLE_NOEXCEPT
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

    char reverse_digits[serialization::max_integer_str_len];
    castle::size_type count = 0U;
    do
    {
        reverse_digits[count++] = static_cast<char>('0' + (magnitude % static_cast<U>(10)));
        magnitude /= static_cast<U>(10);
    }
    while (magnitude != static_cast<U>(0));

    // LCOV_EXCL_START
    if (negative && output.push_back('-') != castle::status::ok)
    {
        return castle::status::full;
    }
    // LCOV_EXCL_STOP

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
* @brief Parses a floating-point value from a textual representation.
*
* Converts the specified character sequence into a floating-point value of
* type @p T. The parser supports:
* - Optional leading sign ('+' or '-').
* - Integer and fractional parts separated by a decimal point ('.').
* - Optional scientific notation using 'e' or 'E' with an optional exponent sign.
*
* Examples of supported formats:
* - "123"
* - "-123.45"
* - "+0.5"
* - "1.23e5"
* - "4.56E-3"
*
* The function validates the input format and performs range checks during
* conversion. If the input cannot be parsed completely or the resulting value
* exceeds the representable range of @p T, an appropriate error status is
* returned.
*
* @tparam T Floating-point type to parse into (e.g. float, double).
* @param text Input character sequence containing the floating-point value.
* @param[out] output Parsed floating-point value when the operation succeeds.
*
* @return castle::status::ok
* if parsing completed successfully.
* @return castle::status::invalid_argument
* if the input is empty, malformed, or contains unsupported characters.
* @return castle::status::out_of_range
* if the parsed value exceeds the numeric limits of @p T or the
* exponent exceeds the supported range.
*
* @note The input string must be fully consumed by the parser. Any trailing
* non-numeric characters result in an invalid_argument error.
* @note Exponent values are limited by
* serialization::max_floating_exponent.
*/
template <typename T>
enable_float_t<T> parse_floating(string_view text, T& output) CASTLE_NOEXCEPT
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
        CASTLE_CONST T digit = static_cast<T>(text[index] - '0');
        // LCOV_EXCL_START
        if (value > (castle::numeric_limits<T>::max() - digit) / static_cast<T>(10))
        {
            return castle::status::out_of_range;
        }
        // LCOV_EXCL_STOP
        value = value * static_cast<T>(10) + digit;
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
            // LCOV_EXCL_START
            if (value > castle::numeric_limits<T>::max())
            {
                return castle::status::out_of_range;
            }
            // LCOV_EXCL_STOP
            scale *= static_cast<T>(0.1);
            ++index;
        }
    }

    if (!has_digit)
    {
        return castle::status::invalid_argument;
    }

    if (index < text.size() && (text[index] == 'e' || text[index] == 'E'))
    {
        ++index;
        bool exponent_negative = false;
        if (index < text.size() && (text[index] == '+' || text[index] == '-'))
        {
            exponent_negative = text[index] == '-';
            ++index;
        }

        if (index == text.size())
        {
            return castle::status::invalid_argument;
        }

        int exponent = 0;
        while (index < text.size() && text[index] >= '0' && text[index] <= '9')
        {
            // LCOV_EXCL_START
            if (exponent > serialization::max_floating_exponent)
            {
                return castle::status::out_of_range;
            }
            // LCOV_EXCL_STOP
            exponent = exponent * 10 + (text[index] - '0');
            if (exponent > serialization::max_floating_exponent)
            {
                return castle::status::out_of_range;
            }
            ++index;
        }

        // LCOV_EXCL_START
        if (index != text.size())
        {
            return castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP

        if (exponent_negative)
        {
            for (int i = 0; i < exponent; ++i)
            {
                value *= static_cast<T>(0.1);
            }
        }
        else
        {
            for (int i = 0; i < exponent; ++i)
            {
                if (value > castle::numeric_limits<T>::max() / static_cast<T>(10))
                {
                    return castle::status::out_of_range;
                }
                value *= static_cast<T>(10);
            }
        }
    }
    else if (index != text.size())
    {
        return castle::status::invalid_argument;
    }

    if (value > castle::numeric_limits<T>::max())
    {
        return castle::status::out_of_range;
    }

    output = negative ? -value : value;
    return castle::status::ok;
}

/**
* @brief Converts a floating-point value to its decimal string representation.
*
* Formats the specified floating-point value into the provided fixed-capacity
* string buffer using fixed-point notation. The generated output consists of:
* - An optional leading minus sign for negative values.
* - A decimal integer part.
* - A fractional part containing exactly @p precision digits when
* @p precision is greater than zero.
*
* The fractional part is rounded to the requested precision using
* round-to-nearest behavior. Scientific notation is not generated.
*
* Examples:
* - format_floating(123.456, output, 2) -> "123.46"
* - format_floating(-1.5, output, 3) -> "-1.500"
* - format_floating(42.0, output, 0) -> "42"
*
* @tparam T Floating-point type to format (e.g. float, double).
* @tparam MaxValueLength Maximum capacity of the destination string buffer.
*
* @param value Floating-point value to format.
* @param[out] output Destination string receiving the formatted result.
* Existing contents are cleared before formatting.
* @param precision Number of digits to generate after the decimal point.
* Values greater than 9 are clamped to 9.
*
* @return castle::status::ok
* if formatting completed successfully.
* @return castle::status::invalid_argument
* if the value is NaN or exceeds the supported conversion range.
* @return castle::status::full
* if the destination buffer does not have sufficient capacity to
* store the formatted result.
*
* @note The output always uses '.' as the decimal separator.
* @note Scientific notation is not supported and is never generated.
* @note Precision is limited to 9 fractional digits.
* @note Values with magnitude greater than or equal to
* 18446744073709551615 are rejected.
*/
template <typename T, castle::size_type MaxValueLength = serialization::max_floating_str_len>
enable_float_t<T> format_floating(
    T value,
    castle::container::string<MaxValueLength>& output,
    castle::size_type precision = 6U) CASTLE_NOEXCEPT
{
    output.clear();
    if (precision > 9U)
    {
        precision = 9U;
    }

    if (value != value)
    {
        return castle::status::invalid_argument;
    }

    CASTLE_CONST bool negative = value < static_cast<T>(0);
    CASTLE_CONST T magnitude = negative ? -value : value;
    CASTLE_CONST T overflow_threshold = static_cast<T>(castle::numeric_limits<uint64_t>::max() /* 18446744073709551615ULL */);
    if (magnitude >= overflow_threshold)
    {
        return castle::status::invalid_argument;
    }

    uint64_t integer_part = static_cast<uint64_t>(magnitude);
    T fractional = magnitude - static_cast<T>(integer_part);

    uint64_t scale = 1ULL;
    for (castle::size_type i = 0U; i < precision; ++i)
    {
        scale *= 10U;
    }

    uint64_t fractional_scaled = static_cast<uint64_t>((fractional * static_cast<T>(scale)) + static_cast<T>(0.5));
    if (fractional_scaled >= scale && scale > 0U)
    {
        fractional_scaled -= scale;
        ++integer_part;
    }

    if (negative && output.push_back('-') != castle::status::ok)
    {
        return castle::status::full;
    }

    char reverse_digits[serialization::max_integer_str_len];
    castle::size_type count = 0U;
    do
    {
        reverse_digits[count++] = static_cast<char>('0' + (integer_part % 10U));
        integer_part /= 10U;
    }
    while (integer_part != 0U);

    while (count > 0U)
    {
        --count;
        if (output.push_back(reverse_digits[count]) != castle::status::ok)
        {
            return castle::status::full;
        }
    }

    if (precision > 0U)
    {
        if (output.push_back('.') != castle::status::ok)
        {
            return castle::status::full;
        }

        char fraction_digits[9U];
        for (castle::size_type i = precision; i > 0U; --i)
        {
            fraction_digits[i - 1U] = static_cast<char>('0' + (fractional_scaled % 10U));
            fractional_scaled /= 10U;
        }

        for (castle::size_type i = 0U; i < precision; ++i)
        {
            if (output.push_back(fraction_digits[i]) != castle::status::ok)
            {
                return castle::status::full;
            }
        }
    }

    return castle::status::ok;
}

} // namespace detail

/**
* @brief Fixed-capacity CSV document container.
*
* Stores CSV data using a row-major, value-owning representation. All rows and
* fields are copied into internal storage during parsing, allowing the source
* buffer to be released immediately after the parse operation completes.
*
* The implementation is designed for deterministic memory usage and provides
* constant-time indexed access to rows and fields without requiring dynamic
* memory allocation, heap-backed arenas, or complex pointer-based structures.
*
* @tparam MaxRows Maximum number of rows that can be stored.
* @tparam MaxColumns Maximum number of columns per row.
* @tparam MaxFieldLength Maximum number of characters that can be stored in a
* single field, excluding any internal terminator.
*
* @note Capacity limits are fixed at compile time.
* @note All field values are owned by the document instance.
* @note Memory consumption is deterministic and does not grow at runtime.
*/
template <castle::size_type MaxRows = 32U,
          castle::size_type MaxColumns = 16U,
          castle::size_type MaxFieldLength = 128U>
class document
{
    static_assert(MaxRows > 0U, "csv::document requires at least one row");
    static_assert(MaxColumns > 0U, "csv::document requires at least one column");
    static_assert(MaxFieldLength > 0U, "csv::document requires a positive field capacity");

public:
    using size_type = castle::size_type;
    using view_type = string_view;
    using field_type = castle::container::string<MaxFieldLength>;

    static CASTLE_CONSTEXPR size_type row_capacity = MaxRows;
    static CASTLE_CONSTEXPR size_type column_capacity = MaxColumns;
    static CASTLE_CONSTEXPR size_type field_capacity = MaxFieldLength;
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

private:
    /**
     * @brief Represents a single CSV row.
     *
     * Stores an ordered collection of field values with a compile-time maximum
     * column count. Each field is stored as an owning string, providing
     * deterministic memory usage and constant-time indexed access.
     *
     * A row may be modified incrementally while parsing or populated directly
     * through the append() and set() operations.
     *
     * @note The number of fields in a row cannot exceed @p MaxColumns.
     * @note Field values must not contain embedded null characters.
     */
    class row_type
    {
    public:
        /** @brief Constructs an empty CSV row. */
        row_type() CASTLE_NOEXCEPT : field_count_(0U) {}

        /** @brief Returns the number of fields in the row. */
        size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return field_count_; }
        /** @brief Returns true if the row contains no fields. */
        bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return field_count_ == 0U; }
        /** @brief Returns true if the row has reached the maximum number of fields. */
        bool full() CASTLE_CONST CASTLE_NOEXCEPT { return field_count_ == MaxColumns; }

        /** @brief Clears all fields from the row. */
        void clear() CASTLE_NOEXCEPT
        {
            field_count_ = 0U;
            for (size_type i = 0U; i < MaxColumns; ++i)
            {
                fields_[i].clear();
            }
        }

        /** @brief Appends a new field to the row.
         *
         * @param value The value to append.
         * @return Status indicating success or failure.
         */
        castle::status append(view_type value) CASTLE_NOEXCEPT
        {
            if (full())
            {
                return castle::status::full;
            }
            if (detail::contains_nul(value))
            {
                return castle::status::invalid_argument;
            }

            field_type candidate;
            CASTLE_CONST castle::status status = candidate.assign(value);
            if (status != castle::status::ok)
            {
                return status;
            }

            fields_[field_count_] = candidate;
            ++field_count_;
            return castle::status::ok;
        }

        /** @brief Appends a character to an existing field.
         *
         * @param column The index of the field.
         * @param value The character to append.
         * @return Status indicating success or failure.
         */
        castle::status append_character(size_type column, char value) CASTLE_NOEXCEPT
        {
            if (column >= field_count_)
            {
                return castle::status::out_of_range;
            }
            if (value == '\0')
            {
                return castle::status::invalid_argument;
            }
            return fields_[column].push_back(value);
        }

        /**
         * @brief Replaces a field value or appends a new field.
         *
         * When @p column equals the current field count, the value is appended as a
         * new field. Otherwise, the existing field at the specified index is
         * replaced.
         *
         * @param column Zero-based field index.
         * @param value Field value to store.
         *
         * @return castle::status::ok on success.
         * @return castle::status::out_of_range if the index exceeds the next
         *         append position.
         * @return castle::status::invalid_argument if the field contains a null
         *         character.
         * @return Any error returned by the underlying field storage.
         */
        castle::status set(size_type column, view_type value) CASTLE_NOEXCEPT
        {
            if (column > field_count_)
            {
                return castle::status::out_of_range;
            }
            if (detail::contains_nul(value))
            {
                return castle::status::invalid_argument;
            }

            if (column == field_count_)
            {
                return append(value);
            }

            return fields_[column].assign(value);
        }

        /**
         * @brief Retrieves a view of a field.
         *
         * @param column Zero-based field index.
         *
         * @return A view of the requested field, or an empty view if the index is
         *         out of range.
         */
        view_type field(size_type column) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return column < field_count_ ? fields_[column].view() : view_type();
        }

        /**
         * @brief Reads a field value into an output view.
         *
         * @param column Zero-based field index.
         * @param[out] output Receives the field view on success.
         *
         * @return castle::status::ok on success.
         * @return castle::status::out_of_range if the field index is invalid. In
         *         this case, @p output is set to an empty view.
         */
        castle::status read(size_type column, view_type& output) CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (column >= field_count_)
            {
                output = view_type();
                return castle::status::out_of_range;
            }
            output = fields_[column].view();
            return castle::status::ok;
        }

    private:
        castle::container::array<field_type, MaxColumns> fields_;
        size_type field_count_;
    };

public:
    document() CASTLE_NOEXCEPT CASTLE_DEFAULT;
    document(CASTLE_CONST document&) CASTLE_DEFAULT;
    document& operator=(CASTLE_CONST document&) CASTLE_DEFAULT;
    document(document&&) CASTLE_NOEXCEPT CASTLE_DEFAULT;
    document& operator=(document&&) CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /** @brief Retrieves the n*mber of rows in the document. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return row_count_; }
    /** @brief Retrieves the maximum number of rows the document can hold. */
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return MaxRows; }
    /** @brief Checks if the document has no rows. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return row_count_ == 0U; }
    /** @brief Checks if the document has reached its maximum capacity. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return row_count_ == MaxRows; }
    /** @brief Retrieves the number of rows in the document. */
    size_type rows() CASTLE_CONST CASTLE_NOEXCEPT { return row_count_; }

    /**
     * @brief Clears all rows in the document. After calling this function, the document will be empty.
     * @note This operation does not change the maximum capacity of the document.
     */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < row_count_; ++i)
        {
            rows_[i].clear();
        }
        row_count_ = 0U;
    }

    /**
    * @brief Appends a new empty row to the document.
    *
    * Creates a row at the end of the document, clears any existing data in the
    * underlying storage slot, and returns the index of the newly created row.
    *
    * @param[out] out_row Receives the zero-based index of the new row on
    * success. Set to @c npos if the operation fails.
    *
    * @return castle::status::ok
    * if the row was added successfully.
    * @return castle::status::full
    * if the document has reached its maximum row capacity.
    *
    * @note The newly created row is initially empty.
    */
    CASTLE_NODISCARD castle::status add_row(size_type& out_row) CASTLE_NOEXCEPT
    {
        if (full())
        {
            out_row = npos;
            return castle::status::full;
        }

        rows_[row_count_].clear();
        out_row = row_count_;
        ++row_count_;
        return castle::status::ok;
    }

    /**
    * @brief Appends a field value to the specified row.
    *
    * Adds a new field at the end of the row identified by @p row.
    *
    * @param row Zero-based row index.
    * @param value Field value to append.
    *
    * @return castle::status::ok
    * if the field was appended successfully.
    * @return castle::status::out_of_range
    * if @p row does not identify an existing row.
    * @return castle::status::invalid_argument
    * if @p value contains an embedded null character.
    * @return castle::status::full
    * if the target row has reached its maximum column capacity or the
    * field exceeds the configured storage capacity.
    */
    CASTLE_NODISCARD castle::status append(size_type row, view_type value) CASTLE_NOEXCEPT
    {
        if (row >= row_count_)
        {
            return castle::status::out_of_range;
        }
        if (detail::contains_nul(value))
        {
            return castle::status::invalid_argument;
        }
        return rows_[row].append(value);
    }

    /**
    * @brief Sets the value of a field in the specified row.
    *
    * Replaces the field at the specified column index. If @p column equals the
    * current number of fields in the row, a new field is appended.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param value Field value to store.
    *
    * @return castle::status::ok
    * if the field was stored successfully.
    * @return castle::status::out_of_range
    * if @p row is not a valid row index or @p column exceeds the next
    * valid insertion position in the target row.
    * @return castle::status::invalid_argument
    * if @p value contains an embedded null character.
    * @return castle::status::full
    * if the target row has reached its column capacity or the field
    * exceeds the configured storage capacity.
    *
    * @note When @p column is equal to the current field count of the row, the
    * value is appended as a new field.
    */
    CASTLE_NODISCARD castle::status set(size_type row, size_type column, view_type value) CASTLE_NOEXCEPT
    {
        if (row >= row_count_)
        {
            return castle::status::out_of_range;
        }
        return rows_[row].set(column, value);
    }

    /**
    * @brief Appends a character to an existing field.
    *
    * Adds the specified character to the end of the field identified by
    * @p row and @p column.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param value Character to append.
    *
    * @return castle::status::ok
    * if the character was appended successfully.
    * @return castle::status::out_of_range
    * if @p row or @p column does not identify an existing field.
    * @return castle::status::invalid_argument
    * if @p value is a null character.
    * @return castle::status::full
    * if the destination field has reached its storage capacity.
    *
    * @note This function can only modify an existing field. It does not create
    * rows or columns automatically.
    */
    CASTLE_NODISCARD castle::status append_character(
        size_type row, size_type column, char value) CASTLE_NOEXCEPT
    {
        if (row >= row_count_ || column >= rows_[row].size())
        {
            return castle::status::out_of_range;
        }
        if (value == '\0')
        {
            return castle::status::invalid_argument;
        }
        return rows_[row].append_character(column, value);
    }

    /**
    * @brief Retrieves the value of a field.
    *
    * Reads the field identified by @p row and @p column and returns it through
    * the output parameter.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param[out] output Receives a view of the requested field on success.
    *
    * @return castle::status::ok
    * if the field was read successfully.
    * @return castle::status::out_of_range
    * if @p row or @p column does not identify an existing field. In this
    * case, @p output is set to an empty view.
    *
    * @note The returned view references data owned by the document and remains
    * valid until the corresponding field or row is modified or cleared.
    */
    CASTLE_NODISCARD castle::status read(
        size_type row,
        size_type column,
        view_type& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (row >= row_count_)
        {
            output = view_type();
            return castle::status::out_of_range;
        }
        return rows_[row].read(column, output);
    }

    /**
     * @brief Retrieves the value of a field as a view.
     * 
     * This function returns a view of the field identified by @p row and @p column.
     * 
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return A view of the requested field if it exists; otherwise, an empty view.
     */
    view_type get(size_type row, size_type column) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return row < row_count_ ? rows_[row].field(column) : view_type();
    }

    /**
     * @brief Retrieves the number of columns in a given row.
     * 
     * @param row Zero-based row index.
     * @return The number of columns in the specified row. If the row does not exist, returns 0.
     * @note If the row exists, the returned count reflects the actual number of columns in that row.
     */
    size_type column_count(size_type row) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return row < row_count_ ? rows_[row].size() : 0U;
    }

    /**
     * @brief Retrieves the maximum number of columns across all rows.
     * @return The maximum number of columns found in any row. If there are no rows, returns 0.
     */
    size_type max_column_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type maximum = 0U;
        for (size_type i = 0U; i < row_count_; ++i)
        {
            if (rows_[i].size() > maximum)
            {
                maximum = rows_[i].size();
            }
        }
        return maximum;
    }

    /** @brief Checks if a given row is full. */
    bool row_full(size_type row) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return row < row_count_ && rows_[row].full();
    }

    /** @brief Sets an integer value in the specified cell. */
    CASTLE_NODISCARD detail::enable_integer_t<int> set_integer(
        size_type row, size_type column, int value) CASTLE_NOEXCEPT
    {
        return set_integer_value(row, column, value);
    }

    /** @brief Sets an integer value of a generic type in the specified cell. */
    template <typename T>
    CASTLE_NODISCARD detail::enable_integer_t<T> set_integer(
        size_type row, size_type column, T value) CASTLE_NOEXCEPT
    {
        return set_integer_value(row, column, value);
    }

    /** @brief Sets a boolean value in the specified cell. */
    CASTLE_NODISCARD castle::status set_bool(
        size_type row, size_type column, bool value) CASTLE_NOEXCEPT
    {
        return set(row, column, value ? view_type("true") : view_type("false"));
    }

    /** @brief Sets an enumeration value in the specified cell. */
    template <typename T>
    CASTLE_NODISCARD detail::enable_enum_t<T> set_enum(
        size_type row, size_type column, T value) CASTLE_NOEXCEPT
    {
        return set_integer(row, column, static_cast<meta::underlying_type_t<T>>(value));
    }

    /**
    * @brief Stores a floating-point value in the specified field.
    *
    * Converts @p value to its decimal string representation using the specified
    * precision and stores the resulting text in the field identified by
    * @p row and @p column.
    *
    * @tparam T Floating-point type to store.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param value Floating-point value to convert and store.
    * @param precision Number of digits to generate after the decimal point.
    * Values greater than the supported limit are clamped by the
    * formatting implementation.
    *
    * @return castle::status::ok
    * if the value was successfully formatted and stored.
    * @return castle::status::out_of_range
    * if @p row is invalid or @p column exceeds the next valid insertion
    * position within the row.
    * @return castle::status::invalid_argument
    * if @p value cannot be represented by the floating-point formatter or
    * the generated value cannot be stored.
    * @return castle::status::full
    * if the destination field or row has insufficient capacity to store
    * the formatted value.
    *
    * @note The value is stored using the formatting rules implemented by
    * detail::format_floating().
    * @note When @p column is equal to the current field count of the row, a new
    * field is appended.
    */
    template <typename T>
    CASTLE_NODISCARD detail::enable_float_t<T> set_floating(
        size_type row,
        size_type column,
        T value,
        size_type precision = 6U) CASTLE_NOEXCEPT
    {
        castle::container::string<serialization::max_floating_str_len> text;
        CASTLE_CONST castle::status format_status = detail::format_floating(value, text, precision);
        if (format_status != castle::status::ok)
        {
            return format_status;
        }
        return set(row, column, text.view());
    }

    /**
    * @brief Reads a field and converts it to a boolean value.
    *
    * The following values are recognized:
    * - "true" (case-insensitive)
    * - "false" (case-insensitive)
    * - "1"
    * - "0"
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param[out] output Receives the converted boolean value on success.
    *
    * @return castle::status::ok
    * if the field was successfully read and converted.
    * @return castle::status::out_of_range
    * if @p row or @p column does not identify an existing field.
    * @return castle::status::invalid_argument
    * if the field value does not represent a supported boolean value.
    *
    * @note Textual boolean values are matched case-insensitively.
    * @note Valid values are "true", "false", "1", and "0".
    */
    CASTLE_NODISCARD castle::status get_bool(
        size_type row, size_type column, bool& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status read_status = read(row, column, text);
        if (read_status != castle::status::ok)
        {
            return read_status;
        }

        if (detail::equals_ignore_case(text, view_type("true")) || text == view_type("1"))
        {
            output = true;
            return castle::status::ok;
        }
        if (detail::equals_ignore_case(text, view_type("false")) || text == view_type("0"))
        {
            output = false;
            return castle::status::ok;
        }
        return castle::status::invalid_argument;
    }

    /**
    * @brief Reads a field and converts it to an integer value.
    *
    * Retrieves the field identified by @p row and @p column and parses its
    * textual representation as an integer of type @p T.
    *
    * @tparam T Integer type to convert to.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param[out] output Receives the parsed integer value on success.
    *
    * @return castle::status::ok
    * if the field was successfully read and converted.
    * @return castle::status::out_of_range
    * if @p row or @p column does not identify an existing field.
    * @return castle::status::invalid_argument
    * if the field does not contain a valid integer representation.
    * @return castle::status::out_of_range
    * if the parsed value cannot be represented by type @p T.
    *
    * @note Integer parsing follows the rules implemented by
    * detail::parse_integer().
    */
    template <typename T>
    CASTLE_NODISCARD detail::enable_integer_t<T> get_integer(
        size_type row, size_type column, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status read_status = read(row, column, text);
        if (read_status != castle::status::ok)
        {
            return read_status;
        }
        return detail::parse_integer(text, output);
    }

    /**
    * @brief Reads a field and converts it to an enumeration value.
    *
    * Retrieves the field identified by @p row and @p column, converts its
    * textual representation to the underlying integer type of @p T, and then
    * casts the result to the requested enumeration type.
    *
    * @tparam T Enumeration type to convert to.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param[out] output Receives the converted enumeration value on success.
    *
    * @return castle::status::ok
    * if the field was successfully read and converted.
    * @return castle::status::out_of_range
    * if @p row or @p column does not identify an existing field, or if
    * the parsed integer value is outside the representable range of the
    * enumeration's underlying type.
    * @return castle::status::invalid_argument
    * if the field does not contain a valid integer representation.
    *
    * @note The field value is interpreted as the underlying integral value of
    * the enumeration.
    * @note No validation is performed to ensure that the resulting value matches
    * a defined enumerator of type @p T.
    */
    template <typename T>
    CASTLE_NODISCARD detail::enable_enum_t<T> get_enum(
        size_type row, size_type column, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        meta::underlying_type_t<T> value = static_cast<meta::underlying_type_t<T>>(0);
        CASTLE_CONST castle::status status = get_integer(row, column, value);
        if (status != castle::status::ok)
        {
            return status;
        }
        output = static_cast<T>(value);
        return castle::status::ok;
    }

    /**
    * @brief Reads a field and converts it to a floating-point value.
    *
    * Retrieves the field identified by @p row and @p column and parses its
    * textual representation as a floating-point value of type @p T.
    *
    * Supported formats include:
    * - Integer values (e.g. "123")
    * - Decimal values (e.g. "123.45")
    * - Scientific notation (e.g. "1.23e4", "-5E-2")
    *
    * @tparam T Floating-point type to convert to.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param[out] output Receives the parsed floating-point value on success.
    *
    * @return castle::status::ok
    * if the field was successfully read and converted.
    * @return castle::status::out_of_range
    * if @p row or @p column does not identify an existing field, or if
    * the parsed value exceeds the representable range of type @p T.
    * @return castle::status::invalid_argument
    * if the field does not contain a valid floating-point representation.
    *
    * @note Floating-point parsing follows the rules implemented by
    * detail::parse_floating().
    */
    template <typename T>
    CASTLE_NODISCARD detail::enable_float_t<T> get_floating(
        size_type row, size_type column, T& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        view_type text;
        CASTLE_CONST castle::status read_status = read(row, column, text);
        if (read_status != castle::status::ok)
        {
            return read_status;
        }
        return detail::parse_floating(text, output);
    }

private:
    /**
    * @brief Stores an integer value in the specified field.
    *
    * Converts @p value to its decimal string representation and stores the
    * resulting text in the field identified by @p row and @p column.
    *
    * @tparam T Integer type to store.
    *
    * @param row Zero-based row index.
    * @param column Zero-based column index.
    * @param value Integer value to convert and store.
    *
    * @return castle::status::ok
    * if the value was successfully formatted and stored.
    * @return castle::status::out_of_range
    * if @p row is invalid or @p column exceeds the next valid insertion
    * position within the row.
    * @return castle::status::full
    * if the destination field or row does not have sufficient capacity
    * to store the formatted value.
    * @return Any error returned by detail::format_integer().
    *
    * @note The value is stored using the formatting rules implemented by
    * detail::format_integer().
    * @note When @p column is equal to the current field count of the row, a new
    * field is appended.
    */
    template <typename T>
    detail::enable_integer_t<T> set_integer_value(size_type row, size_type column, T value) CASTLE_NOEXCEPT
    {
        castle::container::string<serialization::max_integer_str_len> text;
        CASTLE_CONST castle::status format_status = detail::format_integer(value, text);
        if (format_status != castle::status::ok)
        {
            return format_status;
        }
        return set(row, column, text.view());
    }

    castle::container::array<row_type, MaxRows> rows_;
    size_type row_count_ = 0U;
};

namespace detail
{

/**
* @brief Stateful CSV parser implementation.
*
* Parses CSV text from a character buffer and populates a
* document<MaxRows, MaxColumns, MaxFieldLength> instance with the resulting
* records and fields.
*
* The parser implements a single-pass, character-by-character state machine
* that supports:
* - Configurable field delimiters.
* - Quoted fields enclosed in double quotes.
* - Escaped quotes represented as two consecutive double quotes ("").
* - Embedded delimiters within quoted fields.
* - Embedded line breaks within quoted fields.
* - Optional UTF-8 byte order mark (BOM) at the beginning of the input.
* - Optional validation of column-count consistency across records.
*
* During parsing, detailed location information is maintained to enable
* precise error reporting, including byte offset, line number, and column
* number within the source text.
*
* The destination document is cleared before parsing begins. If a parsing
* error occurs, the document is cleared and the failure is reported through
* the returned result object.
*
* @tparam MaxRows Maximum number of rows that can be stored in the output
* document.
* @tparam MaxColumns Maximum number of columns per row.
* @tparam MaxFieldLength Maximum number of characters allowed in a field.
*
* @note This class is an internal implementation detail and is not intended
* for direct use by library consumers.
* @note Memory usage is bounded by the capacity limits of the target
* document type.
* @note The parser does not perform dynamic memory allocation.
*/
template <castle::size_type MaxRows,
          castle::size_type MaxColumns,
          castle::size_type MaxFieldLength>
class parser
{
    static_assert(MaxRows > 0U, "MaxRows must be greater than 0");
    static_assert(MaxColumns > 0U, "MaxColumns must be greater than 0");
    static_assert(MaxFieldLength > 0U, "MaxFieldLength must be greater than 0");

public:
    using document_type = document<MaxRows, MaxColumns, MaxFieldLength>;
    using size_type = castle::size_type;
    using view_type = string_view;

    parser(document_type& output, view_type source, options parse_options) CASTLE_NOEXCEPT
        : output_(output), source_(source), options_(parse_options), offset_(0U), line_(1U), column_(1U),
          row_(document_type::npos), field_started_(false), in_quotes_(false), quote_closed_(false),
          expected_columns_(0U)
    {
    }

    /**
    * @brief Parses the source CSV text into the destination document.
    *
    * Executes the parser state machine and populates the associated output
    * document with the records and fields extracted from the source text.
    *
    * Parsing begins by clearing the destination document. If a UTF-8 byte order
    * mark (BOM) is present at the beginning of the input, it is skipped
    * automatically.
    *
    * The parser supports quoted fields, escaped quotes, embedded delimiters
    * within quoted fields, and line breaks contained in quoted fields. When
    * strict column validation is enabled, all records must contain the same
    * number of fields.
    *
    * @return A result object describing the outcome of the parse operation.
    * On success, the result indicates success and the output document
    * contains the parsed data.
    *
    * @retval error_code::invalid_delimiter
    * The configured field delimiter is not supported.
    * @retval error_code::invalid_quote
    * An invalid quote sequence was encountered.
    * @retval error_code::unexpected_end
    * The input ended while a quoted field was still open.
    * @retval error_code::inconsistent_columns
    * Records contain differing column counts while strict column
    * validation is enabled.
    * @retval error_code::capacity
    * The output document exceeded one of its configured capacity limits.
    *
    * @note On failure, the destination document is cleared and the returned
    * result contains the offset, line, and column of the detected error.
    * @note An empty input is considered valid and produces an empty document.
    */
    result run() CASTLE_NOEXCEPT
    {
        result result_value;
        output_.clear();

        if (!is_valid_delimiter(options_.delimiter))
        {
            return fail(error_code::invalid_delimiter);
        }

        size_type index = 0U;
        if (source_.size() >= 3U &&
            static_cast<uint8_t>(source_[0U]) == castle::serialization::utf8_bom_byte_1 &&
            static_cast<uint8_t>(source_[1U]) == castle::serialization::utf8_bom_byte_2 &&
            static_cast<uint8_t>(source_[2U]) == castle::serialization::utf8_bom_byte_3)
        {
            index = 3U;
            offset_ = 3U;
        }

        if (index == source_.size())
        {
            return result_value;
        }

        while (index < source_.size())
        {
            CASTLE_CONST char current = source_[index];

            if (row_ == document_type::npos)
            {
                CASTLE_CONST castle::status row_status = ensure_row();
                if (row_status != castle::status::ok)
                {
                    return fail_capacity(row_status);
                }
            }

            if (in_quotes_)
            {
                if (current == '"')
                {
                    if (index + 1U < source_.size() && source_[index + 1U] == '"')
                    {
                        CASTLE_CONST castle::status append_status = append_char('"');
                        if (append_status != castle::status::ok)
                        {
                            return fail_capacity(append_status);
                        }
                        consume_plain_char(index);
                        consume_plain_char(index);
                        continue;
                    }

                    in_quotes_ = false;
                    quote_closed_ = true;
                    consume_plain_char(index);
                    continue;
                }

                CASTLE_CONST castle::status append_status = append_char(current);
                if (append_status != castle::status::ok)
                {
                    return fail_capacity(append_status);
                }

                if (is_line_end(current))
                {
                    if (current == '\r' && index + 1U < source_.size() && source_[index + 1U] == '\n')
                    {
                        CASTLE_CONST castle::status lf_status = append_char('\n');
                        if (lf_status != castle::status::ok)
                        {
                            return fail_capacity(lf_status);
                        }
                        consume_plain_char(index);
                    }
                    advance_line(index, current);
                    continue;
                }

                consume_plain_char(index);
                continue;
            }

            if (quote_closed_)
            {
                if (current == options_.delimiter)
                {
                    CASTLE_CONST castle::status field_status = begin_next_field();
                    if (field_status != castle::status::ok)
                    {
                        return fail_capacity(field_status);
                    }
                    consume_plain_char(index);
                    continue;
                }

                if (is_line_end(current))
                {
                    CASTLE_CONST result finish_result = finish_record(index, current);
                    if (!finish_result.succeeded())
                    {
                        return finish_result;
                    }
                    if (current == '\r' && index + 1U < source_.size() && source_[index + 1U] == '\n')
                    {
                        consume_plain_char(index);
                    }
                    advance_line(index, current);
                    continue;
                }

                return fail(error_code::invalid_quote);
            }

            if (current == options_.delimiter)
            {
                CASTLE_CONST castle::status field_status = ensure_field();
                if (field_status != castle::status::ok)
                {
                    return fail_capacity(field_status);
                }
                field_started_ = false;
                CASTLE_CONST castle::status next_status = begin_next_field();
                if (next_status != castle::status::ok)
                {
                    return fail_capacity(next_status);
                }
                consume_plain_char(index);
                continue;
            }

            if (is_line_end(current))
            {
                CASTLE_CONST castle::status field_status = ensure_field();
                if (field_status != castle::status::ok)
                {
                    return fail_capacity(field_status);
                }

                CASTLE_CONST result finish_result = finish_record(index, current);
                if (!finish_result.succeeded())
                {
                    return finish_result;
                }
                if (current == '\r' && index + 1U < source_.size() && source_[index + 1U] == '\n')
                {
                    consume_plain_char(index);
                }
                advance_line(index, current);
                continue;
            }

            if (current == '"')
            {
                if (field_started_)
                {
                    return fail(error_code::invalid_quote);
                }
                CASTLE_CONST castle::status field_status = ensure_field();
                if (field_status != castle::status::ok)
                {
                    return fail_capacity(field_status);
                }
                field_started_ = true;
                in_quotes_ = true;
                consume_plain_char(index);
                continue;
            }

            CASTLE_CONST castle::status field_status = ensure_field();
            if (field_status != castle::status::ok)
            {
                return fail_capacity(field_status);
            }
            field_started_ = true;
            CASTLE_CONST castle::status append_status = append_char(current);
            if (append_status != castle::status::ok)
            {
                return fail_capacity(append_status);
            }
            consume_plain_char(index);
        }

        if (in_quotes_)
        {
            return fail(error_code::unexpected_end);
        }

        return result_value;
    }

private:
    /** Ensures that a new row is available in the output document. */
    castle::status ensure_row() CASTLE_NOEXCEPT
    {
        return output_.add_row(row_);
    }

    /** Ensures that a new field is available in the output document. */
    castle::status ensure_field() CASTLE_NOEXCEPT
    {
        if (output_.column_count(row_) != 0U || field_started_ || quote_closed_)
        {
            return castle::status::ok;
        }

        return output_.append(row_, view_type());
    }

    /** Begins a new field in the output document. */
    castle::status begin_next_field() CASTLE_NOEXCEPT
    {
        field_started_ = false;
        quote_closed_ = false;
        return output_.append(row_, view_type());
    }

    /**
    * @brief Appends a character to the current field being parsed.
    *
    * Adds @p value to the most recently created field in the active row.
    * This function is used internally while constructing field contents during
    * CSV parsing.
    *
    * @param value Character to append.
    *
    * @return castle::status::ok
    * if the character was appended successfully.
    * @return castle::status::invalid_argument
    * if @p value is a null character.
    * @return castle::status::out_of_range
    * if no active field exists in the current row.
    * @return castle::status::full
    * if the destination field has reached its storage capacity.
    */
    castle::status append_char(char value) CASTLE_NOEXCEPT
    {
        if (value == '\0')
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST size_type columns = output_.column_count(row_);
        if (columns == 0U)
        {
            return castle::status::out_of_range;
        }

        return output_.append_character(row_, columns - 1U, value);
    }

    /**
    * @brief Finalizes the current CSV record.
    *
    * Completes processing of the active row and prepares the parser to begin
    * parsing the next record.
    *
    * When strict column validation is enabled, the number of fields in the
    * current row is compared against the column count established by the first
    * record. A mismatch results in a parsing failure.
    *
    * @param error_offset Source offset associated with the current record
    * terminator. Used when reporting column-count errors.
    * @param line_end Record terminator character that ended the current record.
    *
    * @return An empty success result if the record was finalized successfully.
    * @return A failure result with error_code::inconsistent_columns if the
    * current record contains a different number of columns than
    * previously parsed records while strict column validation is enabled.
    *
    * @note On success, parser state is reset so that the next non-processed
    * character starts a new record.
    * @note The first record establishes the expected column count when strict
    * column validation is enabled.
    */
    result finish_record(size_type error_offset, char line_end) CASTLE_NOEXCEPT
    {
        result result_value;
        if (options_.strict_columns)
        {
            CASTLE_CONST size_type columns = output_.column_count(row_);
            if (expected_columns_ == 0U)
            {
                expected_columns_ = columns;
            }
            else if (columns != expected_columns_)
            {
                result_value = fail(error_code::inconsistent_columns);
                result_value.offset = error_offset;
                return result_value;
            }
        }

        (void)line_end;
        row_ = document_type::npos;
        field_started_ = false;
        in_quotes_ = false;
        quote_closed_ = false;
        return result_value;
    }

    result fail_capacity(castle::status status_value) CASTLE_NOEXCEPT
    {
        result result_value = fail(error_code::capacity);
        result_value.status = status_value == castle::status::invalid_argument
                              ? castle::status::invalid_argument
                              : castle::status::full;
        result_value.code = status_value == castle::status::invalid_argument
                            ? error_code::invalid_character
                            : error_code::capacity;
        return result_value;
    }

    /**
    * @brief Converts a storage-related failure into a parser result.
    *
    * Creates a parser failure result from a status value returned by the
    * destination document or field storage. Capacity-related failures are mapped
    * to parser capacity errors, while invalid field content is reported as an
    * invalid character error.
    *
    * @param status_value Status returned by a document or field operation.
    *
    * @return A failure result describing the corresponding parser error.
    *
    * @retval error_code::capacity
    * The parser could not store additional rows, columns, or field data
    * due to configured capacity limits.
    * @retval error_code::invalid_character
    * An invalid character was encountered while constructing a field.
    *
    * @note The returned result preserves the current parser position information
    * through the underlying fail() helper.
    */
    result fail(error_code code) CASTLE_NOEXCEPT
    {
        result result_value;
        result_value.status = (code == error_code::capacity || code == error_code::output_full)
                              ? castle::status::full
                              : (code == error_code::invalid_argument || code == error_code::invalid_delimiter)
                                ? castle::status::invalid_argument
                                : castle::status::invalid_argument;
        result_value.code = code;
        result_value.offset = offset_;
        result_value.line = line_;
        result_value.column = column_;
        output_.clear();
        return result_value;
    }

    /**
    * @brief Consumes a non-record-terminating character from the input stream.
    *
    * Advances the parser position by one character and updates the associated
    * tracking information, including the source offset and column number.
    *
    * @param[in,out] index Current position within the source buffer. Incremented
    * to reference the next character.
    *
    * @note This helper is intended for ordinary character consumption and does
    * not update line tracking. Record terminators are handled separately by
    * advance_line().
    */
    void consume_plain_char(size_type& index) CASTLE_NOEXCEPT
    {
        ++index;
        ++offset_;
        ++column_;
    }

    /**
    * @brief Advances the parser past a record terminator.
    *
    * Consumes a line-ending character, updates the current source position, and
    * moves the parser to the beginning of the next line.
    *
    * @param[in,out] index Current position within the source buffer. Incremented
    * to reference the next character.
    * @param line_end Line terminator character that ended the current record.
    *
    * @note The parser line number is incremented and the column counter is reset
    * to the first column of the next line.
    * @note This helper is used for record terminators, whereas
    * consume_plain_char() is used for ordinary character consumption.
    */
    void advance_line(size_type& index, char line_end) CASTLE_NOEXCEPT
    {
        (void)line_end;
        ++index;
        ++offset_;
        ++line_;
        column_ = 1U;
    }

    document_type& output_;
    view_type source_;
    options options_;
    size_type offset_;
    size_type line_;
    size_type column_;
    size_type row_;
    bool field_started_;
    bool in_quotes_;
    bool quote_closed_;
    size_type expected_columns_;
};

/**
* @brief Appends a field value to a serialized CSV record.
*
* Writes @p value to the destination buffer using CSV escaping rules. Fields
* containing the configured delimiter, double quotes, carriage returns, or
* line feeds are automatically enclosed in double quotes. Embedded double
* quotes are escaped by doubling them.
*
* Examples:
* - abc -> abc
* - a,b -> "a,b"
* - a"b -> "a""b"
* - a\nb -> "a\nb"
*
* @tparam MaxOutput Maximum capacity of the destination string buffer.
*
* @param value Field value to serialize.
* @param delimiter Field delimiter used by the CSV format.
* @param[out] output Destination buffer receiving the serialized field.
*
* @return castle::status::ok
* if the field was serialized successfully.
* @return castle::status::invalid_argument
* if @p value contains an embedded null character.
* @return castle::status::full
* if the destination buffer does not have sufficient capacity to
* store the serialized field.
*
* @note Fields requiring quoting are enclosed in double quotes.
* @note Embedded double quotes are escaped according to RFC 4180-style CSV
* conventions by writing two consecutive double quote characters.
*/
template <castle::size_type MaxOutput>
castle::status append_serialized_field(
    string_view value,
    char delimiter,
    castle::container::string<MaxOutput>& output) CASTLE_NOEXCEPT
{
    if (contains_nul(value))
    {
        return castle::status::invalid_argument;
    }

    bool needs_quotes = false;
    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] == delimiter || value[i] == '"' || value[i] == '\r' || value[i] == '\n')
        {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes)
    {
        return output.append(value);
    }

    if (output.push_back('"') != castle::status::ok)
    {
        return castle::status::full;
    }

    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] == '"')
        {
            if (output.append(string_view("\"\"")) != castle::status::ok)
            {
                return castle::status::full;
            }
        }
        else if (output.push_back(value[i]) != castle::status::ok)
        {
            return castle::status::full;
        }
    }

    return output.push_back('"');
}

} // namespace detail

/**
* @brief Parses CSV text into a document.
*
* Reads the CSV data contained in @p input and populates the specified
* document using the provided parsing options.
*
* The parser supports configurable field delimiters, quoted fields, escaped
* quotes, embedded delimiters within quoted fields, and line breaks contained
* in quoted fields. If a UTF-8 byte order mark (BOM) is present at the
* beginning of the input, it is ignored automatically.
*
* @tparam MaxRows Maximum number of rows that can be stored in the document.
* @tparam MaxColumns Maximum number of columns per row.
* @tparam MaxFieldLength Maximum number of characters allowed in a field.
*
* @param[out] output Destination document receiving the parsed CSV data.
* The document is cleared before parsing begins.
* @param input CSV text to parse.
* @param parse_options Parsing configuration options.
*
* @return A result object describing the outcome of the parse operation.
*
* @retval castle::status::ok
* Parsing completed successfully and @p output contains the parsed
* CSV data.
* @retval castle::status::invalid_argument
* The input contains invalid CSV syntax or an unsupported delimiter.
* @retval castle::status::full
* The parsed data exceeds one or more document capacity limits.
*
* @note On failure, the returned result contains error details including the
* source offset, line number, and column number where the error was
* detected.
* @note An empty input is considered valid and results in an empty document.
*/
template <castle::size_type MaxRows,
          castle::size_type MaxColumns,
          castle::size_type MaxFieldLength>
CASTLE_NODISCARD result parse(
    document<MaxRows, MaxColumns, MaxFieldLength>& output,
    string_view input,
    options parse_options = options()) CASTLE_NOEXCEPT
{
    detail::parser<MaxRows, MaxColumns, MaxFieldLength> parser_instance(
        output,
        input,
        parse_options);
    return parser_instance.run();
}

/**
* @brief Serializes a CSV document into text.
*
* Writes the contents of @p input to the destination buffer using CSV
* formatting rules. Fields are quoted only when required by CSV syntax, and
* embedded double quotes are escaped by doubling them.
*
* Output records are separated using a single line feed character ('\n').
* Empty fields are preserved and serialized as empty values between
* delimiters (for example: @c a,,c).
*
* @tparam OutputCapacity Maximum capacity of the destination buffer.
* @tparam MaxRows Maximum number of rows in the source document.
* @tparam MaxColumns Maximum number of columns per row in the source document.
* @tparam MaxFieldLength Maximum field length in the source document.
*
* @param input Document to serialize.
* @param[out] output Destination buffer receiving the serialized CSV text.
* Existing contents are cleared before serialization.
* @param serialize_options Serialization configuration options.
*
* @return A result object describing the outcome of the operation.
*
* @retval castle::status::ok
* Serialization completed successfully.
* @retval castle::status::invalid_argument
* The configured delimiter is invalid or a field contains an invalid
* character.
* @retval castle::status::full
* The destination buffer does not have sufficient capacity to hold
* the serialized output.
*
* @note Output always uses LF ('\n') line endings regardless of the original
* input format.
* @note Fields are quoted only when they contain the delimiter, double quotes,
* carriage returns, or line feeds.
* @note Embedded double quotes are escaped as two consecutive double quote
* characters.
*/
template <castle::size_type OutputCapacity,
          castle::size_type MaxRows,
          castle::size_type MaxColumns,
          castle::size_type MaxFieldLength>
CASTLE_NODISCARD result serialize(
    CASTLE_CONST document<MaxRows, MaxColumns, MaxFieldLength>& input,
    castle::container::string<OutputCapacity>& output,
    options serialize_options = options()) CASTLE_NOEXCEPT
{
    result result_value;
    output.clear();

    if (!detail::is_valid_delimiter(serialize_options.delimiter))
    {
        result_value.status = castle::status::invalid_argument;
        result_value.code = error_code::invalid_delimiter;
        return result_value;
    }

    for (castle::size_type row = 0U; row < input.rows(); ++row)
    {
        for (castle::size_type column = 0U; column < input.column_count(row); ++column)
        {
            if (column > 0U && output.push_back(serialize_options.delimiter) != castle::status::ok)
            {
                result_value.status = castle::status::full;
                result_value.code = error_code::output_full;
                return result_value;
            }

            CASTLE_CONST string_view field = input.get(row, column);
            CASTLE_CONST castle::status field_status = detail::append_serialized_field(
                field,
                serialize_options.delimiter,
                output);
            if (field_status != castle::status::ok)
            {
                result_value.status = field_status;
                result_value.code = field_status == castle::status::invalid_argument
                                     ? error_code::invalid_character
                                     : error_code::output_full;
                return result_value;
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
* @brief Serializes a CSV document into text.
*
* Convenience wrapper around serialize() that writes the contents of
* @p input to the destination buffer using the specified serialization
* options.
*
* @tparam OutputCapacity Maximum capacity of the destination buffer.
* @tparam MaxRows Maximum number of rows in the source document.
* @tparam MaxColumns Maximum number of columns per row in the source document.
* @tparam MaxFieldLength Maximum field length in the source document.
*
* @param input Document to serialize.
* @param[out] output Destination buffer receiving the serialized CSV text.
* @param serialize_options Serialization configuration options.
*
* @return A result object describing the outcome of the serialization
* operation.
*
* @see serialize()
*/
template <castle::size_type OutputCapacity,
          castle::size_type MaxRows,
          castle::size_type MaxColumns,
          castle::size_type MaxFieldLength>
CASTLE_NODISCARD result write(
    CASTLE_CONST document<MaxRows, MaxColumns, MaxFieldLength>& input,
    castle::container::string<OutputCapacity>& output,
    options serialize_options = options()) CASTLE_NOEXCEPT
{
    return serialize(input, output, serialize_options);
}

} // namespace csv
} // namespace serialization
} // namespace castle

#endif // CASTLE_SERIALIZATION_CSV_HPP
