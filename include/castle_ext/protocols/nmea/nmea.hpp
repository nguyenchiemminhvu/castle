// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nmea.hpp
 * @brief Header-only NMEA 0183 text protocol primitives for Castle.
 *
 * This module contains only protocol-level data and algorithms:
 *  - NMEA framing constants and well-known talker/sentence identifiers;
 *  - XOR checksum calculation and hex encode/decode helpers;
 *  - non-owning tokenized sentence views;
 *  - deterministic, caller-provided-buffer sentence encoding and decoding;
 *  - generic textual field decode helpers (lat/lon, UTC time/date, numbers).
 *
 * @code
 * #include "castle_ext/protocols/nmea/nmea.hpp"
 *
 * char sentence[96U];
 * castle::container::string_view fields_data[2U] = {
 *     castle::container::string_view("1"),
 *     castle::container::string_view("08")
 * };
 * castle::container::array_view<const castle::container::string_view> fields(fields_data, 2U);
 *
 * castle::size_type written = 0U;
 * castle::protocols::nmea::encode_sentence(
 *     castle::container::string_view(castle::protocols::nmea::TALKER_GP),
 *     castle::container::string_view(castle::protocols::nmea::SENTENCE_GSA),
 *     fields,
 *     sentence,
 *     sizeof(sentence),
 *     written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_NMEA_NMEA_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_NMEA_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/string_view.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace castle
{
namespace protocols
{
namespace nmea
{

/**
 * @brief NMEA sentence start delimiter.
 */
static CASTLE_CONSTEXPR char NMEA_START_CHAR = '$';

/**
 * @brief NMEA checksum field delimiter.
 */
static CASTLE_CONSTEXPR char NMEA_CHECKSUM_CHAR = '*';

/**
 * @brief NMEA comma-separated field delimiter.
 */
static CASTLE_CONSTEXPR char NMEA_FIELD_SEP = ',';

/**
 * @brief Carriage return terminator byte.
 */
static CASTLE_CONSTEXPR char NMEA_CR = '\r';

/**
 * @brief Line feed terminator byte.
 */
static CASTLE_CONSTEXPR char NMEA_LF = '\n';

/**
 * @brief NMEA 0183 standard maximum sentence length, including '$' and `<CR><LF>`.
 */
static CASTLE_CONSTEXPR castle::size_type NMEA_MAX_SENTENCE_LEN = 82U;

/**
 * @brief Default practical content-buffer ceiling for deterministic embedded parsing.
 *
 * Wider than the standard to accommodate proprietary messages (e.g. u-blox
 * PUBX) which may exceed 82 characters.
 */
static CASTLE_CONSTEXPR castle::size_type NMEA_SAFE_MAX_SENTENCE_LEN = 256U;

/**
 * @brief Default maximum number of comma-separated fields tracked per sentence.
 */
static CASTLE_CONSTEXPR castle::size_type NMEA_DEFAULT_MAX_FIELDS = 32U;

/**
 * @brief Upper bound on the characters accepted by the numeric field helpers.
 */
static CASTLE_CONSTEXPR castle::size_type NMEA_MAX_NUMERIC_FIELD_LEN = 31U;

// -----------------------------------------------------------------------------
// Talker ID string constants
// -----------------------------------------------------------------------------

static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_GP = "GP"; /**< GPS */
static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_GL = "GL"; /**< GLONASS */
static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_GA = "GA"; /**< Galileo */
static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_GB = "GB"; /**< BeiDou */
static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_GQ = "GQ"; /**< QZSS */
static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_GN = "GN"; /**< Combined GNSS */
static CASTLE_CONSTEXPR CASTLE_CONST char* TALKER_P  = "P";  /**< Proprietary (e.g. PUBX) */

// -----------------------------------------------------------------------------
// Sentence type string constants
// -----------------------------------------------------------------------------

static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GGA = "GGA"; /**< Global Positioning System Fix Data */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_RMC = "RMC"; /**< Recommended Minimum Specific GNSS Data */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GSA = "GSA"; /**< GNSS DOP and Active Satellites */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GSV = "GSV"; /**< GNSS Satellites in View */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GLL = "GLL"; /**< Geographic Position Latitude/Longitude */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_VTG = "VTG"; /**< Course Over Ground and Ground Speed */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GBS = "GBS"; /**< GNSS Satellite Fault Detection */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GNS = "GNS"; /**< GNSS Fix Data */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GST = "GST"; /**< GNSS Pseudo Range Error Statistics */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_ZDA = "ZDA"; /**< Time and Date */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_DTM = "DTM"; /**< Datum Reference */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_GRS = "GRS"; /**< GNSS Range Residuals */
static CASTLE_CONSTEXPR CASTLE_CONST char* SENTENCE_TXT = "TXT"; /**< Text Transmission */

/**
 * @brief Non-owning view of a fully tokenized NMEA sentence.
 *
 * `talker`, `type`, and every entry in `fields` alias slices of the producer's
 * own content buffer. In the parser this is the parser's fixed-capacity
 * inline buffer. The view is only valid while that storage remains unchanged.
 */
struct message_view
{
    castle::container::string_view talker{};
    castle::container::string_view type{};
    castle::container::array_view<CASTLE_CONST castle::container::string_view> fields{};
    uint8_t checksum = 0U;
    bool checksum_present = false;

    /**
     * @brief Returns the field at `index`, or an empty view when out of range.
     */
    CASTLE_NODISCARD castle::container::string_view field(castle::size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (index < fields.size()) ? fields[index] : castle::container::string_view();
    }

    /**
     * @brief Reports whether the field at `index` exists and is non-empty.
     */
    CASTLE_NODISCARD bool has_field(castle::size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (index < fields.size()) && !fields[index].empty();
    }

    /**
     * @brief Returns the number of stored fields.
     */
    CASTLE_NODISCARD castle::size_type field_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return fields.size();
    }

    /**
     * @brief Reports whether this sentence has the requested type (e.g. "GGA").
     */
    CASTLE_NODISCARD bool is(castle::container::string_view type_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return type == type_value;
    }

    /**
     * @brief Reports whether this sentence has the requested talker (e.g. "GP").
     */
    CASTLE_NODISCARD bool is_talker(castle::container::string_view talker_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return talker == talker_value;
    }
};

/**
 * @brief Streaming XOR checksum accumulator.
 *
 * NMEA checksums cover every byte strictly between '$' and '*'. The parser
 * feeds each content byte once as it arrives, avoiding a second pass.
 */
class checksum_accumulator CASTLE_FINAL
{
public:
    /** @brief Constructs an accumulator with the checksum cleared. */
    checksum_accumulator() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Clears the checksum state.
     */
    void reset() CASTLE_NOEXCEPT
    {
        value_ = 0U;
    }

    /**
     * @brief Folds one content byte into the running XOR checksum.
     */
    void update(char value) CASTLE_NOEXCEPT
    {
        value_ = static_cast<uint8_t>(value_ ^ static_cast<uint8_t>(value));
    }

    /**
     * @brief Returns the current checksum value.
     */
    CASTLE_NODISCARD uint8_t value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value_;
    }

private:
    uint8_t value_ = 0U;
};

/**
 * @brief Reports whether `c` is an ASCII hexadecimal digit.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_hex_digit(char c) CASTLE_NOEXCEPT
{
    return ((c >= '0') && (c <= '9'))
        || ((c >= 'A') && (c <= 'F'))
        || ((c >= 'a') && (c <= 'f'));
}

/**
 * @brief Converts a single ASCII hex digit to its 4-bit value.
 * @warning `c` must satisfy `is_hex_digit(c)`.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR uint8_t hex_nibble(char c) CASTLE_NOEXCEPT
{
    return (c <= '9')
        ? static_cast<uint8_t>(c - '0')
        : static_cast<uint8_t>((((c >= 'a') ? (c - 'a') : (c - 'A')) + 10));
}

/**
 * @brief Decodes two ASCII hex characters into a byte value.
 * @return `true` when both characters are valid hex digits.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_hex_byte(char hi, char lo, uint8_t& out) CASTLE_NOEXCEPT
{
    if (!is_hex_digit(hi) || !is_hex_digit(lo)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }
    out = static_cast<uint8_t>((hex_nibble(hi) << 4U) | hex_nibble(lo));
    return true;
}

/**
 * @brief Writes a byte as two uppercase ASCII hex characters (no null terminator).
 * @warning `out` must reference at least two characters.
 */
CASTLE_INLINE void write_hex_byte(uint8_t value, char* out) CASTLE_NOEXCEPT
{
    static CASTLE_CONSTEXPR char digits[] = "0123456789ABCDEF";
    out[0U] = digits[(value >> 4U) & 0x0FU];
    out[1U] = digits[value & 0x0FU];
}

/**
 * @brief Computes the NMEA XOR checksum over a byte range.
 */
CASTLE_NODISCARD CASTLE_INLINE uint8_t compute_checksum(castle::container::string_view data) CASTLE_NOEXCEPT
{
    checksum_accumulator accumulator;
    for (castle::size_type index = 0U; index < data.size(); ++index) // LCOV_EXCL_BR_LINE
    {
        accumulator.update(data[index]);
    }
    return accumulator.value();
}

/**
 * @brief Splits a combined talker+type identifier (e.g. "GNGGA", "PUBX") into
 * its talker and type parts.
 *
 * Proprietary identifiers starting with 'P' yield talker "P". Standard
 * 5-character identifiers split into a 2-character talker and the remaining
 * type. Shorter identifiers are treated as talker-less (`talker` left empty).
 */
CASTLE_INLINE void split_identifier(
    castle::container::string_view id,
    castle::container::string_view& talker,
    castle::container::string_view& type) CASTLE_NOEXCEPT
{
    talker = castle::container::string_view();
    type = castle::container::string_view();

    if (id.empty()) // LCOV_EXCL_BR_LINE
    {
        return;
    }

    if (id.front() == 'P') // LCOV_EXCL_BR_LINE
    {
        talker = id.substr(0U, 1U);
        type = id.substr(1U);
    }
    else if (id.size() >= 5U) // LCOV_EXCL_BR_LINE
    {
        talker = id.substr(0U, 2U);
        type = id.substr(2U);
    }
    else
    {
        type = id;
    }
}

/**
 * @brief Tokenizes sentence content (everything between '$' and '*', exclusive)
 * into a talker, a type, and up to `field_capacity` comma-separated fields.
 *
 * @param content Raw sentence content, e.g. "GNGGA,092725.00,...".
 * @param field_storage Caller-provided backing storage for the decoded fields.
 * @param field_capacity Number of elements available in `field_storage`.
 * @param[out] talker Decoded talker ID, or empty when talker-less/malformed.
 * @param[out] type Decoded sentence type, or empty when malformed.
 * @return Number of fields written to `field_storage` (clamped to `field_capacity`).
 * @note Fields beyond `field_capacity` are silently dropped; the returned
 * count reflects only the stored subset.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::size_type tokenize_fields(
    castle::container::string_view content,
    castle::container::string_view* field_storage,
    castle::size_type field_capacity,
    castle::container::string_view& talker,
    castle::container::string_view& type) CASTLE_NOEXCEPT
{
    talker = castle::container::string_view();
    type = castle::container::string_view();

    if (content.empty()) // LCOV_EXCL_BR_LINE
    {
        return 0U;
    }

    CASTLE_CONST castle::size_type comma = content.find(NMEA_FIELD_SEP);

    if (comma == castle::container::string_view::npos) // LCOV_EXCL_BR_LINE
    {
        split_identifier(content, talker, type);
        return 0U;
    }

    split_identifier(content.substr(0U, comma), talker, type);

    if ((field_storage == nullptr) || (field_capacity == 0U)) // LCOV_EXCL_BR_LINE
    {
        return 0U;
    }

    CASTLE_CONST castle::container::string_view remaining = content.substr(comma + 1U);
    CASTLE_CONST castle::size_type rlen = remaining.size();

    castle::size_type count = 0U;
    castle::size_type start = 0U;
    while ((start <= rlen) && (count < field_capacity)) // LCOV_EXCL_BR_LINE
    {
        castle::size_type end = remaining.find(NMEA_FIELD_SEP, start);
        if (end == castle::container::string_view::npos) // LCOV_EXCL_BR_LINE
        {
            end = rlen;
        }
        field_storage[count] = remaining.substr(start, end - start);
        ++count;
        start = end + 1U;
    }

    return count;
}

/**
 * @brief Decodes and validates one complete, standalone NMEA sentence line.
 *
 * Accepts an optional leading '$' and an optional trailing CR/LF. When a
 * '*HH' checksum suffix is present, it is decoded and compared against the
 * computed checksum of the content.
 *
 * @param line Complete sentence text.
 * @param field_storage Caller-provided backing storage for the decoded fields.
 * @param field_capacity Number of elements available in `field_storage`.
 * @param[out] output Decoded sentence view referencing `line` and `field_storage`.
 * @return `true` when the sentence has a non-empty type and, if present, a
 * matching checksum.
 * @note This is a pure, stateless convenience built on `tokenize_fields()` and
 * `compute_checksum()` for one-shot decoding and unit testing. The streaming
 * parser in `castle_ext/parsers/nmea_parser` reuses the same building blocks.
 */
CASTLE_NODISCARD CASTLE_INLINE bool decode_sentence(
    castle::container::string_view line,
    castle::container::string_view* field_storage,
    castle::size_type field_capacity,
    message_view& output) CASTLE_NOEXCEPT
{
    output = message_view{};

    if (!line.empty() && (line.front() == NMEA_START_CHAR)) // LCOV_EXCL_BR_LINE
    {
        line = line.substr(1U);
    }

    while (!line.empty() && ((line.back() == NMEA_CR) || (line.back() == NMEA_LF))) // LCOV_EXCL_BR_LINE
    {
        line = line.substr(0U, line.size() - 1U);
    }

    CASTLE_CONST castle::size_type star = line.find(NMEA_CHECKSUM_CHAR);
    castle::container::string_view content = line;
    bool checksum_present = false;
    uint8_t received_checksum = 0U;

    if (star != castle::container::string_view::npos) // LCOV_EXCL_BR_LINE
    {
        content = line.substr(0U, star);
        CASTLE_CONST castle::container::string_view cs_field = line.substr(star + 1U);
        if ((cs_field.size() < 2U) || !parse_hex_byte(cs_field[0U], cs_field[1U], received_checksum)) // LCOV_EXCL_BR_LINE
        {
            return false;
        }
        checksum_present = true;
    }

    castle::container::string_view talker;
    castle::container::string_view type;
    CASTLE_CONST castle::size_type field_count =
        tokenize_fields(content, field_storage, field_capacity, talker, type);

    output.talker = talker;
    output.type = type;
    output.fields = castle::container::array_view<CASTLE_CONST castle::container::string_view>(field_storage, field_count);
    output.checksum = received_checksum;
    output.checksum_present = checksum_present;

    if (type.empty()) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    if (checksum_present && (compute_checksum(content) != received_checksum)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    return true;
}

/**
 * @brief Encodes one complete NMEA sentence into a caller-provided buffer.
 *
 * Produces `$<talker><type>,<field0>,<field1>,...*HH\r\n`.
 *
 * @param talker Talker ID (may be empty for talker-less messages).
 * @param type Sentence type. Must be non-empty.
 * @param fields Ordered field values to join with commas.
 * @param output Destination buffer.
 * @param output_capacity Size of the destination buffer.
 * @param[out] bytes_written Number of bytes generated on success, otherwise zero.
 * @return Castle status code.
 *
 * @note This function never allocates and never writes outside `output`.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_sentence(
    castle::container::string_view talker,
    castle::container::string_view type,
    castle::container::array_view<CASTLE_CONST castle::container::string_view> fields,
    char* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;

    if (type.empty() || (output == nullptr)) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_argument;
    }

    castle::size_type content_length = talker.size() + type.size();
    for (castle::size_type index = 0U; index < fields.size(); ++index) // LCOV_EXCL_BR_LINE
    {
        content_length += 1U + fields[index].size();
    }

    CASTLE_CONST castle::size_type required = 1U  // '$'
        + content_length
        + 1U                                      // '*'
        + 2U                                       // checksum hex digits
        + 2U;                                      // "\r\n"

    if (output_capacity < required) // LCOV_EXCL_BR_LINE
    {
        return castle::status::full;
    }

    castle::size_type pos = 0U;
    output[pos] = NMEA_START_CHAR;
    ++pos;

    for (castle::size_type index = 0U; index < talker.size(); ++index) // LCOV_EXCL_BR_LINE
    {
        output[pos] = talker[index];
        ++pos;
    }
    for (castle::size_type index = 0U; index < type.size(); ++index) // LCOV_EXCL_BR_LINE
    {
        output[pos] = type[index];
        ++pos;
    }
    for (castle::size_type field_index = 0U; field_index < fields.size(); ++field_index) // LCOV_EXCL_BR_LINE
    {
        output[pos] = NMEA_FIELD_SEP;
        ++pos;
        CASTLE_CONST castle::container::string_view field = fields[field_index];
        for (castle::size_type index = 0U; index < field.size(); ++index) // LCOV_EXCL_BR_LINE
        {
            output[pos] = field[index];
            ++pos;
        }
    }

    CASTLE_CONST castle::container::string_view content(&output[1U], content_length);
    CASTLE_CONST uint8_t check = compute_checksum(content);

    output[pos] = NMEA_CHECKSUM_CHAR;
    ++pos;
    write_hex_byte(check, &output[pos]);
    pos += 2U;
    output[pos] = NMEA_CR;
    ++pos;
    output[pos] = NMEA_LF;
    ++pos;

    bytes_written = pos;
    return castle::status::ok;
}

/**
 * @brief Parses a textual field as a double.
 * @return `true` when `text` is non-empty, within the supported length, and numeric.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_double(castle::container::string_view text, double& out) CASTLE_NOEXCEPT
{
    if (text.empty() || (text.size() > NMEA_MAX_NUMERIC_FIELD_LEN)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    char buffer[NMEA_MAX_NUMERIC_FIELD_LEN + 1U]{};
    memcpy(buffer, text.data(), text.size());
    buffer[text.size()] = '\0';

    char* end = nullptr;
    CASTLE_CONST double value = strtod(buffer, &end);
    if (end == buffer) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    out = value;
    return true;
}

/**
 * @brief Parses a textual field as a signed integer.
 * @return `true` when `text` is non-empty, within the supported length, and numeric.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_int(castle::container::string_view text, int& out, int base = 10) CASTLE_NOEXCEPT
{
    if (text.empty() || (text.size() > NMEA_MAX_NUMERIC_FIELD_LEN)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    char buffer[NMEA_MAX_NUMERIC_FIELD_LEN + 1U]{};
    memcpy(buffer, text.data(), text.size());
    buffer[text.size()] = '\0';

    char* end = nullptr;
    CASTLE_CONST long value = strtol(buffer, &end, base);
    if (end == buffer) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    out = static_cast<int>(value);
    return true;
}

/**
 * @brief Parses a textual field as an unsigned 32-bit integer.
 * @return `true` when `text` is non-empty, within the supported length, and numeric.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_uint(castle::container::string_view text, uint32_t& out, int base = 10) CASTLE_NOEXCEPT
{
    if (text.empty() || (text.size() > NMEA_MAX_NUMERIC_FIELD_LEN)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    char buffer[NMEA_MAX_NUMERIC_FIELD_LEN + 1U]{};
    memcpy(buffer, text.data(), text.size());
    buffer[text.size()] = '\0';

    char* end = nullptr;
    CASTLE_CONST unsigned long value = strtoul(buffer, &end, base);
    if (end == buffer) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    out = static_cast<uint32_t>(value);
    return true;
}

/**
 * @brief Extracts the first character of a textual field.
 * @return `true` when `text` is non-empty.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_char(castle::container::string_view text, char& out) CASTLE_NOEXCEPT
{
    if (text.empty()) // LCOV_EXCL_BR_LINE
    {
        return false;
    }
    out = text.front();
    return true;
}

/**
 * @brief Converts an NMEA `DDmm.mmmm`/`DDDmm.mmmm` value plus direction
 * character into signed decimal degrees.
 * @param value Latitude or longitude magnitude field.
 * @param direction Direction field ("N"/"S"/"E"/"W"), or empty for unsigned.
 * @return `true` when `value` is numeric.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_latlon(
    castle::container::string_view value,
    castle::container::string_view direction,
    double& out) CASTLE_NOEXCEPT
{
    double raw = 0.0;
    if (!parse_double(value, raw)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    // NMEA lat/lon magnitudes are always non-negative, so truncation equals floor().
    CASTLE_CONST double degrees = static_cast<double>(static_cast<long>(raw / 100.0));
    CASTLE_CONST double minutes = raw - (degrees * 100.0);
    out = degrees + (minutes / 60.0);

    if (!direction.empty()) // LCOV_EXCL_BR_LINE
    {
        CASTLE_CONST char dir = direction.front();
        if ((dir == 'S') || (dir == 's') || (dir == 'W') || (dir == 'w')) // LCOV_EXCL_BR_LINE
        {
            out = -out;
        }
    }
    return true;
}

/**
 * @brief Parses an NMEA `hhmmss.ss` UTC time field into a raw numeric value.
 *
 * Callers needing structured time should compute:
 * `hour = floor(utc_time / 10000)`, `minute = floor(fmod(utc_time, 10000) / 100)`,
 * `second = fmod(utc_time, 100)`.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_utc_time(castle::container::string_view text, double& out) CASTLE_NOEXCEPT
{
    return parse_double(text, out);
}

/**
 * @brief Parses an NMEA `ddmmyy` UTC date field into a raw numeric value.
 */
CASTLE_NODISCARD CASTLE_INLINE bool parse_utc_date(castle::container::string_view text, uint32_t& out) CASTLE_NOEXCEPT
{
    return parse_uint(text, out);
}

} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_NMEA_HPP
