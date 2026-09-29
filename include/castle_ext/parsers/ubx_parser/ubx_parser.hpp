// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ubx_parser.hpp
 * @brief Fixed-storage, byte-streaming UBX parser for Castle.
 *
 * The parser implements the UBX framing state machine directly over caller-
 * supplied bytes:
 *
 * `WAIT_SYNC_1 -> WAIT_SYNC_2 -> CLASS -> ID -> LEN_LO -> LEN_HI ->
 *  PAYLOAD -> CK_A -> CK_B`.
 *
 * It keeps only one fixed-size payload buffer whose capacity is selected by
 * template parameter. Unexpected bytes are discarded while searching for the
 * next UBX synchronization sequence. Invalid checksum and oversized frames
 * are discarded without allocation.
 *
 * Successful frames are reported synchronously through a single message
 * callback as a non-owning `message_view`. A separate error callback reports
 * bounded parser errors. The callback wrappers are Castle's vtable-less
 * fixed-storage `callbacks::function` type.
 *
 * @code
 * using parser_type = castle::parsers::ubx_parser::ubx_parser<>;
 * parser_type parser;
 *
 * parser.set_message_callback(
 *     [](const castle::protocols::ubx::message_view& message)
 *     {
 *         if (message.is(
 *                 castle::protocols::ubx::UBX_CLASS_NAV,
 *                 castle::protocols::ubx::UBX_ID_NAV_PVT))
 *         {
 *             // Decode NAV-PVT fields directly from message.payload.
 *         }
 *     });
 *
 * parser.feed(bytes, count);
 * @endcode
 */
#ifndef CASTLE_EXT_PARSERS_UBX_PARSER_UBX_PARSER_HPP
#define CASTLE_EXT_PARSERS_UBX_PARSER_UBX_PARSER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"

#include <stdint.h>

namespace castle
{
namespace parsers
{
namespace ubx_parser
{

/**
 * @brief State machine states exposed for deterministic unit testing.
 */
enum class parse_state : uint8_t
{
    wait_sync_1 = 0U,
    wait_sync_2,
    wait_class,
    wait_id,
    wait_length_low,
    wait_length_high,
    read_payload,
    wait_checksum_a,
    wait_checksum_b
};

/**
 * @brief Error categories emitted by the UBX parser.
 */
enum class parser_error_code : uint8_t
{
    none = 0U,
    invalid_sync = 1U,
    payload_too_large = 2U,
    checksum_mismatch = 3U,
    invalid_argument = 4U
};

/**
 * @brief Deterministic diagnostic record for parser errors.
 *
 * No textual allocation is performed. Applications may map the code to their
 * own diagnostic/logging layer.
 */
struct parse_error
{
    parser_error_code code = parser_error_code::none;
    uint8_t msg_class = 0U;
    uint8_t msg_id = 0U;
    uint16_t payload_length = 0U;
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
        case parser_error_code::invalid_sync:
            return "invalid UBX synchronization sequence";
        case parser_error_code::payload_too_large:
            return "UBX payload exceeds parser capacity";
        case parser_error_code::checksum_mismatch:
            return "UBX checksum mismatch";
        case parser_error_code::invalid_argument:
            return "invalid parser input";
        default:
            return "unknown UBX parser error";
    }
}

/**
 * @brief Deterministic streaming UBX parser with embedded payload storage.
 *
 * @tparam MaxPayload Maximum payload length accepted by this parser instance.
 * @tparam CallbackStorageSize Inline storage reserved for each callback.
 * @tparam CallbackStorageAlignment Alignment used for callback storage.
 *
 * The parser is intentionally non-copyable and non-movable so callback state
 * and parser storage cannot be accidentally duplicated or relocated.
 *
 * @note All parser operations are single-threaded and non-reentrant.
 * @warning `message_view::payload` is valid only until the parser receives
 * another byte or feed call.
 */
template <
    castle::size_type MaxPayload = protocols::ubx::UBX_SAFE_MAX_PAYLOAD_LEN,
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class ubx_parser CASTLE_FINAL
{
    static_assert(MaxPayload > 0U,
                  "ubx_parser requires MaxPayload > 0");
    static_assert(MaxPayload <= static_cast<castle::size_type>(protocols::ubx::UBX_MAX_PAYLOAD_LEN),
                  "ubx_parser MaxPayload exceeds the UBX 16-bit length field");
    static_assert(CallbackStorageSize > 0U,
                  "ubx_parser callback storage must be non-zero");
    static_assert(CallbackStorageAlignment > 0U
                  && (CallbackStorageAlignment & (CallbackStorageAlignment - 1U)) == 0U,
                  "ubx_parser callback storage alignment must be a power of two");

public:
    using size_type = castle::size_type;
    using message_type = protocols::ubx::message_view;
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
    ubx_parser() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Destroys the parser and its inline callback wrappers.
     */
    ~ubx_parser() CASTLE_DEFAULT;

    ubx_parser(CASTLE_CONST ubx_parser&) CASTLE_DELETE;
    ubx_parser& operator=(CASTLE_CONST ubx_parser&) CASTLE_DELETE;
    ubx_parser(ubx_parser&&) CASTLE_DELETE;
    ubx_parser& operator=(ubx_parser&&) CASTLE_DELETE;

    /**
     * @brief Sets the callback invoked for every checksum-valid UBX message.
     *
     * The callback receives a zero-copy view into parser-owned payload storage.
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
     * @brief Sets the callback invoked for discarded/invalid frames.
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
    void feed(uint8_t byte) CASTLE_NOEXCEPT
    {
        process_byte(byte);
    }

    /**
     * @brief Feeds a contiguous byte range.
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
            process_byte(data[index]);
        }
    }

    /**
     * @brief Feeds a Castle array view.
     */
    void feed(castle::container::array_view<CASTLE_CONST uint8_t> data) CASTLE_NOEXCEPT
    {
        feed(data.data(), data.size());
    }

    /**
     * @brief Resets parser state and frame counters.
     *
     * Installed callbacks are retained.
     */
    void reset() CASTLE_NOEXCEPT
    {
        state_ = parse_state::wait_sync_1;
        header_ = protocols::ubx::message_header{};
        checksum_.reset();
        payload_size_ = 0U;
        frames_decoded_ = 0U;
        frames_discarded_ = 0U;
    }

    /**
     * @brief Returns the compile-time payload capacity.
     */
    static CASTLE_CONSTEXPR size_type payload_capacity() CASTLE_NOEXCEPT
    {
        return MaxPayload;
    }

    /**
     * @brief Returns the current parser state.
     */
    CASTLE_NODISCARD parse_state state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_;
    }

    /**
     * @brief Returns the number of checksum-valid frames delivered.
     */
    CASTLE_NODISCARD size_type frames_decoded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return frames_decoded_;
    }

    /**
     * @brief Returns the number of frames discarded after a framing/checksum error.
     */
    CASTLE_NODISCARD size_type frames_discarded() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return frames_discarded_;
    }

    /**
     * @brief Returns the number of bytes currently stored in the partial payload.
     */
    CASTLE_NODISCARD size_type payload_size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return payload_size_;
    }

private:
    void reset_frame() CASTLE_NOEXCEPT
    {
        state_ = parse_state::wait_sync_1;
        header_ = protocols::ubx::message_header{};
        checksum_.reset();
        payload_size_ = 0U;
    }

    void emit_error(parser_error_code code) CASTLE_NOEXCEPT
    {
        error_type error;
        error.code = code;
        error.msg_class = header_.msg_class;
        error.msg_id = header_.msg_id;
        error.payload_length = header_.payload_length;

        ++frames_discarded_;

        reset_frame();

        if (error_callback_)
        {
            error_callback_(error);
        }
    }

    void on_frame_complete() CASTLE_NOEXCEPT
    {
        CASTLE_CONST protocols::ubx::checksum received{
            received_ck_a_,
            received_ck_b_
        };

        CASTLE_CONST protocols::ubx::message_view message{
            header_,
            castle::container::array_view<CASTLE_CONST uint8_t>(
                payload_.data(),
                payload_size_),
            received
        };

        ++frames_decoded_;

        // Reset only the state machine bookkeeping. The payload array remains
        // in place so the zero-copy view is valid throughout the callback.
        state_ = parse_state::wait_sync_1;
        header_ = protocols::ubx::message_header{};
        checksum_.reset();
        payload_size_ = 0U;

        if (message_callback_)
        {
            message_callback_(message);
        }
    }

    void process_byte(uint8_t byte) CASTLE_NOEXCEPT
    {
        // UBX synchronization bytes are part of the protocol module.

        switch (state_)
        {
            case parse_state::wait_sync_1:
                if (byte == protocols::ubx::UBX_SYNC_CHAR_1)
                {
                    state_ = parse_state::wait_sync_2;
                }
                break;

            case parse_state::wait_sync_2:
                if (byte == protocols::ubx::UBX_SYNC_CHAR_2)
                {
                    state_ = parse_state::wait_class;
                    checksum_.reset();
                }
                else if (byte == protocols::ubx::UBX_SYNC_CHAR_1)
                {
                    // Keep waiting for the second sync byte. This handles
                    // B5 B5 62 without losing the second B5.
                    state_ = parse_state::wait_sync_2;
                }
                else
                {
                    state_ = parse_state::wait_sync_1;
                }
                break;

            case parse_state::wait_class:
                header_.msg_class = byte;
                checksum_.update(byte);
                state_ = parse_state::wait_id;
                break;

            case parse_state::wait_id:
                header_.msg_id = byte;
                checksum_.update(byte);
                state_ = parse_state::wait_length_low;
                break;

            case parse_state::wait_length_low:
                header_.payload_length =
                    static_cast<uint16_t>(byte);
                checksum_.update(byte);
                state_ = parse_state::wait_length_high;
                break;

            case parse_state::wait_length_high:
            {
                header_.payload_length = static_cast<uint16_t>(
                    header_.payload_length
                  | static_cast<uint16_t>(static_cast<uint16_t>(byte) << 8U));

                checksum_.update(byte);

                if (header_.payload_length > static_cast<uint16_t>(MaxPayload))
                {
                    emit_error(parser_error_code::payload_too_large);
                    break;
                }

                payload_size_ = 0U;

                if (header_.payload_length == 0U)
                {
                    state_ = parse_state::wait_checksum_a;
                }
                else
                {
                    state_ = parse_state::read_payload;
                }
                break;
            }

            case parse_state::read_payload:
                payload_[payload_size_] = byte;
                ++payload_size_;
                checksum_.update(byte);

                if (payload_size_ == header_.payload_length)
                {
                    state_ = parse_state::wait_checksum_a;
                }
                break;

            case parse_state::wait_checksum_a:
                received_ck_a_ = byte;
                state_ = parse_state::wait_checksum_b;
                break;

            case parse_state::wait_checksum_b:
                received_ck_b_ = byte;
                if (checksum_.value().ck_a != received_ck_a_
                 || checksum_.value().ck_b != received_ck_b_)
                {
                    emit_error(parser_error_code::checksum_mismatch);
                }
                else
                {
                    on_frame_complete();
                }
                break;

            default: // LCOV_EXCL_LINE
                // Defensive recovery if state storage is corrupted.
                emit_error(parser_error_code::invalid_sync); // LCOV_EXCL_LINE
                break; // LCOV_EXCL_LINE
        }
    }

    parse_state state_ = parse_state::wait_sync_1;
    protocols::ubx::message_header header_{};
    protocols::ubx::checksum_accumulator checksum_{};

    castle::container::array<uint8_t, MaxPayload> payload_{};
    size_type payload_size_ = 0U;

    uint8_t received_ck_a_ = 0U;
    uint8_t received_ck_b_ = 0U;

    size_type frames_decoded_ = 0U;
    size_type frames_discarded_ = 0U;

    message_callback_type message_callback_{};
    error_callback_type error_callback_{};
};

} // namespace ubx_parser
} // namespace parsers
} // namespace castle

#endif // CASTLE_EXT_PARSERS_UBX_PARSER_UBX_PARSER_HPP
