// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nmea_parser.hpp
 * @brief Fixed-storage, byte-streaming NMEA 0183 parser for Castle.
 *
 * The parser implements the NMEA framing state machine directly over caller-
 * supplied bytes:
 *
 * `WAIT_DOLLAR -> ACCUMULATE -> [WAIT_CHECKSUM_HI -> WAIT_CHECKSUM_LO] -> WAIT_LF`.
 *
 * It keeps one fixed-size content buffer (whose capacity is selected by
 * template parameter) and one fixed-size field table, and reuses the
 * tokenizing/checksum building blocks from `castle_ext/protocols/nmea`
 * instead of re-implementing them. Non-NMEA noise bytes are discarded while
 * searching for the next '$'. Oversized messages and checksum mismatches are
 * discarded without allocation.
 *
 * Successful messages are reported synchronously through a single message
 * callback as a non-owning `message_view`. A separate error callback reports
 * bounded parser errors. The callback wrappers are Castle's vtable-less
 * fixed-storage `callbacks::function` type.
 *
 * @code
 * using parser_type = castle_ext::parsers::nmea_parser::nmea_parser<>;
 * parser_type parser;
 *
 * parser.set_message_callback(
 *     [](const castle_ext::protocols::nmea::message_view& sentence)
 *     {
 *         if (sentence.is(castle_ext::protocols::nmea::SENTENCE_GGA))
 *         {
 *             // Decode fields directly via castle_ext::protocols::nmea::parse_*.
 *         }
 *     });
 *
 * parser.feed(bytes, count);
 * @endcode
 */
#ifndef CASTLE_EXT_PARSERS_NMEA_PARSER_NMEA_PARSER_HPP
#define CASTLE_EXT_PARSERS_NMEA_PARSER_NMEA_PARSER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/string_view.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace parsers
{
namespace nmea_parser
{

/**
 * @brief State machine states exposed for deterministic unit testing.
 */
enum class parse_state : uint8_t
{
    wait_dollar = 0U,
    accumulate,
    wait_checksum_hi,
    wait_checksum_lo,
    wait_lf
};

/**
 * @brief Error categories emitted by the NMEA parser.
 */
enum class parser_error_code : uint8_t
{
    none = 0U,
    checksum_mismatch = 1U,
    sentence_too_long = 2U,
    unexpected_start_in_sentence = 3U,
    invalid_argument = 4U
};

/**
 * @brief Deterministic diagnostic record for parser errors.
 *
 * `talker`/`type` alias the parser's internal buffer and are only populated
 * for `checksum_mismatch` (raised after tokenizing); they are valid only
 * synchronously during the error callback. No textual allocation is performed.
 */
struct parse_error
{
    parser_error_code code = parser_error_code::none;
    castle::container::string_view talker{};
    castle::container::string_view type{};
};

/**
 * @brief Returns a stable human-readable description for a parser error.
 */
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONST char* error_message(parser_error_code code) CASTLE_NOEXCEPT
{
    switch (code) // LCOV_EXCL_BR_LINE
    {
        case parser_error_code::none:
            return "no error";
        case parser_error_code::checksum_mismatch:
            return "NMEA checksum mismatch";
        case parser_error_code::sentence_too_long:
            return "NMEA sentence exceeds parser capacity";
        case parser_error_code::unexpected_start_in_sentence:
            return "unexpected '$' before previous sentence terminated";
        case parser_error_code::invalid_argument:
            return "invalid parser input";
        default:
            return "unknown NMEA parser error";
    }
}

/**
 * @brief Deterministic streaming NMEA parser with embedded sentence storage.
 *
 * @tparam MaxSentenceLen Maximum content length (excluding '$', '*HH', CR/LF)
 * accepted by this parser instance.
 * @tparam MaxFields Maximum number of comma-separated fields tracked per sentence.
 * @tparam CallbackStorageSize Inline storage reserved for each callback.
 * @tparam CallbackStorageAlignment Alignment used for callback storage.
 *
 * The parser is intentionally non-copyable and non-movable so callback state
 * and parser storage cannot be accidentally duplicated or relocated.
 *
 * @note All parser operations are single-threaded and non-reentrant.
 * @warning `message_view::talker`/`type`/`fields` are valid only until the
 * parser receives another byte or feed call.
 */
template <
    castle::size_type MaxSentenceLen = protocols::nmea::NMEA_SAFE_MAX_SENTENCE_LEN,
    castle::size_type MaxFields = protocols::nmea::NMEA_DEFAULT_MAX_FIELDS,
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class nmea_parser CASTLE_FINAL
{
    static_assert(MaxSentenceLen > 0U,
                  "nmea_parser requires MaxSentenceLen > 0");
    static_assert(MaxFields > 0U,
                  "nmea_parser requires MaxFields > 0");
    static_assert(CallbackStorageSize > 0U,
                  "nmea_parser callback storage must be non-zero");
    static_assert(CallbackStorageAlignment > 0U
                  && (CallbackStorageAlignment & (CallbackStorageAlignment - 1U)) == 0U,
                  "nmea_parser callback storage alignment must be a power of two");

public:
    using size_type = castle::size_type;
    using message_type = protocols::nmea::message_view;
    using error_type = parse_error;
    using message_callback_type = castle::callbacks::function<
        void(CASTLE_CONST message_type&),
        CallbackStorageSize,
        CallbackStorageAlignment>;
    using error_callback_type = castle::callbacks::function<
        void(CASTLE_CONST error_type&),
        CallbackStorageSize,
        CallbackStorageAlignment>;

    /**
     * @brief Constructs an idle parser.
     */
    nmea_parser() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Destroys the parser and its inline callback wrappers.
     */
    ~nmea_parser() CASTLE_DEFAULT;

    nmea_parser(CASTLE_CONST nmea_parser&) CASTLE_DELETE;
    nmea_parser& operator=(CASTLE_CONST nmea_parser&) CASTLE_DELETE;
    nmea_parser(nmea_parser&&) CASTLE_DELETE;
    nmea_parser& operator=(nmea_parser&&) CASTLE_DELETE;

    /**
     * @brief Sets the callback invoked for every checksum-valid NMEA sentence.
     *
     * The callback receives a zero-copy view into parser-owned buffer storage.
     * It is invoked synchronously before `feed()` returns.
     */
    void set_message_callback(message_callback_type&& callback) CASTLE_NOEXCEPT
    {
        message_callback_ = CASTLE_MOVE(callback);
    }

    /**
     * @brief Stores any callable compatible with the message callback signature.
     */
    template <
        typename Callback,
        typename = castle::meta::enable_if_t<
            !castle::meta::is_same<
                castle::meta::decay_t<Callback>,
                message_callback_type>::value>>
    void set_message_callback(Callback&& callback)
    {
        message_callback_type wrapper(CASTLE_FORWARD<Callback>(callback));
        message_callback_ = CASTLE_MOVE(wrapper);
    }

    /**
     * @brief Clears the message callback.
     */
    void clear_message_callback() CASTLE_NOEXCEPT
    {
        message_callback_ = message_callback_type{};
    }

    /**
     * @brief Reports whether a message callback is installed.
     */
    CASTLE_NODISCARD bool has_message_callback() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<bool>(message_callback_);
    }

    /**
     * @brief Sets the callback invoked for discarded/invalid messages.
     */
    void set_error_callback(error_callback_type&& callback) CASTLE_NOEXCEPT
    {
        error_callback_ = CASTLE_MOVE(callback);
    }

    /**
     * @brief Stores any callable compatible with the error callback signature.
     */
    template <
        typename Callback,
        typename = castle::meta::enable_if_t<
            !castle::meta::is_same<
                castle::meta::decay_t<Callback>,
                error_callback_type>::value>>
    void set_error_callback(Callback&& callback)
    {
        error_callback_type wrapper(CASTLE_FORWARD<Callback>(callback));
        error_callback_ = CASTLE_MOVE(wrapper);
    }

    /**
     * @brief Clears the error callback.
     */
    void clear_error_callback() CASTLE_NOEXCEPT
    {
        error_callback_ = error_callback_type{};
    }

    /**
     * @brief Reports whether an error callback is installed.
     */
    CASTLE_NODISCARD bool has_error_callback() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<bool>(error_callback_);
    }

    /**
     * @brief Feeds exactly one byte into the state machine.
     *
     * @note This is the primitive operation used by the bulk feed functions.
     */
    void feed(char byte) CASTLE_NOEXCEPT
    {
        process_byte(byte);
    }

    /**
     * @brief Feeds a contiguous character range.
     *
     * @warning When `data == nullptr`, only `size == 0` is accepted.
     */
    void feed(CASTLE_CONST char* data, size_type size) CASTLE_NOEXCEPT
    {
        if ((data == nullptr) && (size != 0U))
        {
            emit_error(parser_error_code::invalid_argument);
            return;
        }

        for (size_type index = 0U; index < size; ++index)
        {
            process_byte(data[index]);
        }
    }

    /**
     * @brief Feeds a Castle string view.
     */
    void feed(castle::container::string_view data) CASTLE_NOEXCEPT
    {
        feed(data.data(), data.size());
    }

    /**
     * @brief Feeds a contiguous byte range from a raw transport buffer.
     *
     * @warning When `data == nullptr`, only `size == 0` is accepted.
     */
    void feed(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        if ((data == nullptr) && (size != 0U))
        {
            emit_error(parser_error_code::invalid_argument);
            return;
        }

        for (size_type index = 0U; index < size; ++index)
        {
            process_byte(static_cast<char>(data[index]));
        }
    }

    /**
     * @brief Feeds a Castle array view over raw transport bytes.
     */
    void feed(castle::container::array_view<CASTLE_CONST uint8_t> data) CASTLE_NOEXCEPT
    {
        feed(data.data(), data.size());
    }

    /**
     * @brief Resets parser state and sentence counters.
     *
     * Installed callbacks are retained.
     */
    void reset() CASTLE_NOEXCEPT
    {
        state_ = parse_state::wait_dollar;
        buffer_len_ = 0U;
        checksum_.reset();
        checksum_present_ = false;
        received_checksum_ = 0U;
        checksum_hi_ = '\0';
        messages_decoded_ = 0U;
        messages_discarded_ = 0U;
    }

    /**
     * @brief Returns the compile-time content-buffer capacity.
     */
    static CASTLE_CONSTEXPR size_type max_sentence_length() CASTLE_NOEXCEPT
    {
        return MaxSentenceLen;
    }

    /**
     * @brief Returns the compile-time field-table capacity.
     */
    static CASTLE_CONSTEXPR size_type max_fields() CASTLE_NOEXCEPT
    {
        return MaxFields;
    }

    /**
     * @brief Returns the current parser state.
     */
    CASTLE_NODISCARD parse_state state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_;
    }

    /**
     * @brief Returns the number of checksum-valid messages delivered.
     */
    CASTLE_NODISCARD size_type messages_decoded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return messages_decoded_;
    }

    /**
     * @brief Returns the number of messages discarded after a framing/checksum error.
     */
    CASTLE_NODISCARD size_type messages_discarded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return messages_discarded_;
    }

    /**
     * @brief Returns the number of bytes currently stored in the partial sentence.
     */
    CASTLE_NODISCARD size_type buffered_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return buffer_len_;
    }

private:
    void start_sentence() CASTLE_NOEXCEPT
    {
        buffer_len_ = 0U;
        checksum_.reset();
        checksum_present_ = false;
        state_ = parse_state::accumulate;
    }

    void emit_error(
        parser_error_code code,
        castle::container::string_view talker = castle::container::string_view(),
        castle::container::string_view type = castle::container::string_view()) CASTLE_NOEXCEPT
    {
        error_type error;
        error.code = code;
        error.talker = talker;
        error.type = type;

        ++messages_discarded_;

        buffer_len_ = 0U;
        checksum_.reset();
        checksum_present_ = false;
        state_ = parse_state::wait_dollar;

        if (error_callback_)
        {
            error_callback_(error);
        }
    }

    void on_sentence_complete() CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::container::string_view content(buffer_.data(), buffer_len_);

        castle::container::string_view talker;
        castle::container::string_view type;
        CASTLE_CONST size_type field_count = protocols::nmea::tokenize_fields(
            content, field_storage_.data(), MaxFields, talker, type);

        if (checksum_present_ && (checksum_.value() != received_checksum_))
        {
            emit_error(parser_error_code::checksum_mismatch, talker, type);
            return;
        }

        CASTLE_CONST message_type message{
            talker,
            type,
            castle::container::array_view<CASTLE_CONST castle::container::string_view>(field_storage_.data(), field_count),
            received_checksum_,
            checksum_present_
        };

        ++messages_decoded_;

        // Reset only the state machine bookkeeping. The buffer and field table
        // remain in place so the zero-copy view is valid throughout the callback.
        buffer_len_ = 0U;
        checksum_.reset();
        checksum_present_ = false;
        state_ = parse_state::wait_dollar;

        if (message_callback_)
        {
            message_callback_(message);
        }
    }

    void process_byte(char byte) CASTLE_NOEXCEPT
    {
        switch (state_)
        {
            case parse_state::wait_dollar:
                if (byte == protocols::nmea::NMEA_START_CHAR)
                {
                    start_sentence();
                }
                // All other bytes are noise (binary data, partial text, etc.).
                break;

            case parse_state::accumulate:
                if (byte == protocols::nmea::NMEA_START_CHAR)
                {
                    // A new '$' before termination means the previous sentence
                    // was truncated; report it, then resume on the new '$'.
                    emit_error(parser_error_code::unexpected_start_in_sentence);
                    start_sentence();
                }
                else if (byte == protocols::nmea::NMEA_CHECKSUM_CHAR)
                {
                    checksum_present_ = true;
                    state_ = parse_state::wait_checksum_hi;
                }
                else if (byte == protocols::nmea::NMEA_CR)
                {
                    // Some devices omit the checksum; treat CR as sentence end.
                    checksum_present_ = false;
                    state_ = parse_state::wait_lf;
                }
                else if (byte == protocols::nmea::NMEA_LF)
                {
                    checksum_present_ = false;
                    on_sentence_complete();
                }
                else if (buffer_len_ >= MaxSentenceLen)
                {
                    emit_error(parser_error_code::sentence_too_long);
                }
                else
                {
                    buffer_[buffer_len_] = byte;
                    ++buffer_len_;
                    checksum_.update(byte);
                }
                break;

            case parse_state::wait_checksum_hi:
                checksum_hi_ = byte;
                state_ = parse_state::wait_checksum_lo;
                break;

            case parse_state::wait_checksum_lo:
            {
                uint8_t parsed = 0U;
                if (!protocols::nmea::parse_hex_byte(checksum_hi_, byte, parsed))
                {
                    emit_error(parser_error_code::checksum_mismatch);
                    break;
                }
                received_checksum_ = parsed;
                state_ = parse_state::wait_lf;
                break;
            }

            case parse_state::wait_lf:
                if (byte == protocols::nmea::NMEA_CR)
                {
                    // Absorb optional CR before LF.
                }
                else if (byte == protocols::nmea::NMEA_LF)
                {
                    on_sentence_complete();
                }
                else if (byte == protocols::nmea::NMEA_START_CHAR)
                {
                    // Framing error: no LF received, but a new sentence is starting.
                    emit_error(parser_error_code::unexpected_start_in_sentence);
                    start_sentence();
                }
                // Any other stray byte is ignored while still waiting for LF.
                break;

            default: // LCOV_EXCL_LINE
                // Defensive recovery if state storage is corrupted.
                emit_error(parser_error_code::invalid_argument); // LCOV_EXCL_LINE
                break; // LCOV_EXCL_LINE
        }
    }

    parse_state state_ = parse_state::wait_dollar;

    castle::container::array<char, MaxSentenceLen> buffer_{};
    size_type buffer_len_ = 0U;

    protocols::nmea::checksum_accumulator checksum_{};
    bool checksum_present_ = false;
    uint8_t received_checksum_ = 0U;
    char checksum_hi_ = '\0';

    castle::container::array<castle::container::string_view, MaxFields> field_storage_{};

    size_type messages_decoded_ = 0U;
    size_type messages_discarded_ = 0U;

    message_callback_type message_callback_{};
    error_callback_type error_callback_{};
};

} // namespace nmea_parser
} // namespace parsers
} // namespace castle_ext

#endif // CASTLE_EXT_PARSERS_NMEA_PARSER_NMEA_PARSER_HPP
