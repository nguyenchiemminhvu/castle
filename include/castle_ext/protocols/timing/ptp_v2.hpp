// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ptp_v2.hpp
 * @brief Fixed-storage IEEE 1588 PTPv2 end-to-end slave engine.
 *
 * The engine consumes a PTP message without Ethernet or UDP headers and the
 * timestamp captured at the transport timestamp point. It produces bounded
 * protocol output and measurements without owning a socket, buffer pool,
 * thread, or operating-system clock. This keeps the same implementation usable
 * from embedded Linux, Zephyr, or another RTOS network adapter.
 *
 * The implemented exchange is the ordinary end-to-end slave sequence:
 * Sync, optional Follow_Up, Delay_Req, and Delay_Resp. Both one-step and
 * two-step Sync are supported. Master selection, Announce/BMCA, peer-to-peer
 * delay, profile-specific TLVs, and transport framing remain outside the core.
 */
#ifndef CASTLE_EXT_PROTOCOLS_TIMING_PTP_V2_HPP
#define CASTLE_EXT_PROTOCOLS_TIMING_PTP_V2_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array_view.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace timing
{
namespace ptp
{

static CASTLE_CONSTEXPR uint8_t PTP_VERSION = 2U;
static CASTLE_CONSTEXPR castle::size_type PTP_HEADER_SIZE = 34U;
static CASTLE_CONSTEXPR castle::size_type PTP_TIMESTAMP_SIZE = 10U;
static CASTLE_CONSTEXPR castle::size_type PTP_DELAY_REQUEST_SIZE = 44U;
CASTLE_UNUSED static CASTLE_CONSTEXPR castle::size_type PTP_DELAY_RESPONSE_SIZE = 54U;
CASTLE_UNUSED static CASTLE_CONSTEXPR uint16_t PTP_ETHERTYPE = 0x88F7U;
CASTLE_UNUSED static CASTLE_CONSTEXPR uint16_t PTP_EVENT_PORT = 319U;
CASTLE_UNUSED static CASTLE_CONSTEXPR uint16_t PTP_GENERAL_PORT = 320U;
static CASTLE_CONSTEXPR uint16_t PTP_FLAG_TWO_STEP = 0x0200U;
static CASTLE_CONSTEXPR uint64_t PTP_MAX_SECONDS = 0x0000FFFFFFFFFFFFULL;
static CASTLE_CONSTEXPR uint32_t PTP_NANOSECONDS_PER_SECOND = 1000000000U;
static CASTLE_CONSTEXPR int64_t PTP_CORRECTION_SCALE = 65536LL;
static CASTLE_CONSTEXPR uint8_t PTP_ANY_TRANSPORT_SPECIFIC = 0xFFU;

/** @brief PTP message types used by the slave end-to-end exchange. */
enum class message_type : uint8_t
{
    sync = 0x0U,
    delay_request = 0x1U,
    pdelay_request = 0x2U,
    pdelay_response = 0x3U,
    follow_up = 0x8U,
    delay_response = 0x9U,
    pdelay_response_follow_up = 0xAU,
    announce = 0xBU,
    signaling = 0xCU,
    management = 0xDU
};

/** @brief PTP timestamp containing the 48-bit seconds field and nanoseconds. */
struct timestamp
{
    uint64_t seconds = 0U;
    uint32_t nanoseconds = 0U;

    CASTLE_CONSTEXPR timestamp() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    CASTLE_CONSTEXPR timestamp(uint64_t sec, uint32_t nsec) CASTLE_NOEXCEPT
        : seconds(sec)
        , nanoseconds(nsec)
    {
    }
};

/** @brief PTP clock/port identity. */
struct port_identity
{
    uint8_t clock_identity[8U] = {};
    uint16_t port_number = 0U;
};

/** @brief Common PTPv2 header decoded into host representation. */
struct message_header
{
    uint8_t transport_specific = 0U;
    message_type type = message_type::sync;
    uint8_t version = PTP_VERSION;
    uint16_t message_length = 0U;
    uint8_t domain_number = 0U;
    uint16_t flags = 0U;
    int64_t correction_field = 0LL;
    port_identity source_port{};
    uint16_t sequence_id = 0U;
    uint8_t control_field = 0U;
    int8_t log_message_interval = 0;

    CASTLE_NODISCARD bool is_two_step() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & PTP_FLAG_TWO_STEP) != 0U;
    }
};

/**
 * @brief Non-owning view of a validated PTP message.
 *
 * The payload aliases the supplied input buffer and must not outlive or mutate
 * independently of that buffer.
 */
struct message_view
{
    message_header header{};
    castle::container::array_view<CASTLE_CONST uint8_t> payload{};

    CASTLE_NODISCARD bool is(message_type value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return header.type == value;
    }
};

/** @brief Completed timestamps and delay/offset calculation for one exchange. */
struct measurement
{
    port_identity master_port{};
    uint16_t sync_sequence_id = 0U;
    uint16_t delay_request_sequence_id = 0U;
    bool two_step = false;
    timestamp master_sync{};
    timestamp slave_sync{};
    timestamp slave_delay_request{};
    timestamp master_delay_request{};
    int64_t offset_nanoseconds = 0LL;
    int64_t path_delay_nanoseconds = 0LL;
    int64_t correction_nanoseconds = 0LL;
};

/**
 * @brief Bounded output generated while consuming a packet.
 *
 * `send_delay_request` means the caller should transmit `delay_request` and
 * later provide its transport TX timestamp through `delay_request_transmitted`.
 * `measurement_ready` is asserted when all timestamps required by the selected
 * exchange are present and the offset/path delay have been calculated.
 */
struct slave_action
{
    bool send_delay_request = false;
    uint8_t delay_request[PTP_DELAY_REQUEST_SIZE] = {};
    castle::size_type delay_request_size = 0U;
    uint16_t delay_request_sequence = 0U;
    bool measurement_ready = false;
    measurement sample{};
};

/** @brief Fixed configuration for one PTP slave instance. */
struct slave_config
{
    uint8_t domain_number = 0U;
    uint8_t transport_specific = 0U;
    uint16_t initial_delay_request_sequence = 0U;
};

CASTLE_NODISCARD CASTLE_CONSTEXPR bool valid_timestamp(
    timestamp CASTLE_CONST& value) CASTLE_NOEXCEPT
{
    return (value.seconds <= PTP_MAX_SECONDS)
        && (value.nanoseconds < PTP_NANOSECONDS_PER_SECOND);
}

CASTLE_NODISCARD CASTLE_CONSTEXPR bool same_port(
    port_identity CASTLE_CONST& left,
    port_identity CASTLE_CONST& right) CASTLE_NOEXCEPT
{
    if (left.port_number != right.port_number)
    {
        return false;
    }

    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        if (left.clock_identity[index] != right.clock_identity[index])
        {
            return false;
        }
    }
    return true;
}

/** @brief Signed nanosecond difference `left - right` for nearby timestamps. */
CASTLE_NODISCARD CASTLE_CONSTEXPR int64_t timestamp_difference(
    timestamp CASTLE_CONST& left,
    timestamp CASTLE_CONST& right) CASTLE_NOEXCEPT
{
    return (static_cast<int64_t>(left.seconds) - static_cast<int64_t>(right.seconds))
         * static_cast<int64_t>(PTP_NANOSECONDS_PER_SECOND)
         + static_cast<int64_t>(left.nanoseconds)
         - static_cast<int64_t>(right.nanoseconds);
}

/**
 * @brief Computes a checked signed nanosecond difference.
 * @return `true` when the result fits in int64_t, otherwise `false`.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR bool timestamp_difference_checked(
    timestamp CASTLE_CONST& left,
    timestamp CASTLE_CONST& right,
    int64_t& result) CASTLE_NOEXCEPT
{
    CASTLE_CONST int64_t seconds = static_cast<int64_t>(left.seconds)
                                 - static_cast<int64_t>(right.seconds);
    CASTLE_CONST int64_t nanoseconds = static_cast<int64_t>(left.nanoseconds)
                                     - static_cast<int64_t>(right.nanoseconds);
    CASTLE_CONST int64_t scale = static_cast<int64_t>(PTP_NANOSECONDS_PER_SECOND);

    if ((seconds > (INT64_MAX / scale)) || (seconds < (INT64_MIN / scale)))
    {
        return false;
    }

    CASTLE_CONST int64_t whole_seconds = seconds * scale;
    if (nanoseconds > 0LL && whole_seconds > INT64_MAX - nanoseconds)
    {
        return false;
    }
    if (nanoseconds < 0LL && whole_seconds < INT64_MIN - nanoseconds)
    {
        return false;
    }

    result = whole_seconds + nanoseconds;
    return true;
}

/** @brief Converts PTP's 2^-16 ns correction field to whole nanoseconds. */
CASTLE_NODISCARD CASTLE_CONSTEXPR int64_t correction_to_nanoseconds(
    int64_t correction_field) CASTLE_NOEXCEPT
{
    return correction_field / PTP_CORRECTION_SCALE;
}

namespace detail
{

CASTLE_NODISCARD CASTLE_CONSTEXPR uint16_t read_be16(
    CASTLE_CONST uint8_t* data) CASTLE_NOEXCEPT
{
    return static_cast<uint16_t>(static_cast<uint16_t>(data[0U]) << 8U)
         | static_cast<uint16_t>(data[1U]);
}

CASTLE_NODISCARD CASTLE_CONSTEXPR uint32_t read_be32(
    CASTLE_CONST uint8_t* data) CASTLE_NOEXCEPT
{
    return (static_cast<uint32_t>(data[0U]) << 24U)
         | (static_cast<uint32_t>(data[1U]) << 16U)
         | (static_cast<uint32_t>(data[2U]) << 8U)
         | static_cast<uint32_t>(data[3U]);
}

CASTLE_NODISCARD CASTLE_CONSTEXPR uint64_t read_be64(
    CASTLE_CONST uint8_t* data) CASTLE_NOEXCEPT
{
    uint64_t value = 0U;
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        value = (value << 8U) | static_cast<uint64_t>(data[index]);
    }
    return value;
}

CASTLE_NODISCARD CASTLE_CONSTEXPR int64_t read_be64_signed(
    CASTLE_CONST uint8_t* data) CASTLE_NOEXCEPT
{
    CASTLE_CONST uint64_t value = read_be64(data);
    if ((value & 0x8000000000000000ULL) == 0U)
    {
        return static_cast<int64_t>(value);
    }

    CASTLE_CONST uint64_t magnitude = (~value) + 1ULL;
    // LCOV_EXCL_START
    if (magnitude == 0x8000000000000000ULL)
    {
        return INT64_MIN;
    }
    // LCOV_EXCL_STOP
    return -static_cast<int64_t>(magnitude);
}

CASTLE_INLINE void write_be16(
    uint8_t* data,
    uint16_t value) CASTLE_NOEXCEPT
{
    data[0U] = static_cast<uint8_t>(value >> 8U);
    data[1U] = static_cast<uint8_t>(value);
}

CASTLE_INLINE void write_be32(
    uint8_t* data,
    uint32_t value) CASTLE_NOEXCEPT
{
    data[0U] = static_cast<uint8_t>(value >> 24U);
    data[1U] = static_cast<uint8_t>(value >> 16U);
    data[2U] = static_cast<uint8_t>(value >> 8U);
    data[3U] = static_cast<uint8_t>(value);
}

CASTLE_INLINE void write_be64(
    uint8_t* data,
    uint64_t value) CASTLE_NOEXCEPT
{
    for (castle::size_type index = 0U; index < 8U; ++index) // LCOV_EXCL_BR_LINE
    {
        data[7U - index] = static_cast<uint8_t>(value >> (index * 8U));
    }
}

CASTLE_INLINE void write_be64_signed(
    uint8_t* data,
    int64_t value) CASTLE_NOEXCEPT
{
    write_be64(data, static_cast<uint64_t>(value));
}

CASTLE_NODISCARD CASTLE_CONSTEXPR bool read_timestamp(
    CASTLE_CONST uint8_t* data,
    timestamp& output) CASTLE_NOEXCEPT
{
    uint64_t seconds = 0U;
    for (castle::size_type index = 0U; index < 6U; ++index)
    {
        seconds = (seconds << 8U) | static_cast<uint64_t>(data[index]);
    }
    output = timestamp(seconds, read_be32(&data[6U]));
    return valid_timestamp(output);
}

CASTLE_INLINE void write_timestamp(
    uint8_t* data,
    timestamp value) CASTLE_NOEXCEPT
{
    for (castle::size_type index = 0U; index < 6U; ++index)
    {
        data[5U - index] = static_cast<uint8_t>(value.seconds >> (index * 8U));
    }
    data[6U] = static_cast<uint8_t>(value.nanoseconds >> 24U);
    data[7U] = static_cast<uint8_t>(value.nanoseconds >> 16U);
    data[8U] = static_cast<uint8_t>(value.nanoseconds >> 8U);
    data[9U] = static_cast<uint8_t>(value.nanoseconds);
}

CASTLE_INLINE void copy_port(
    uint8_t* output,
    port_identity CASTLE_CONST& source) CASTLE_NOEXCEPT
{
    for (castle::size_type index = 0U; index < 8U; ++index) // LCOV_EXCL_BR_LINE
    {
        output[index] = source.clock_identity[index];
    }
    write_be16(&output[8U], source.port_number);
}

CASTLE_NODISCARD CASTLE_CONSTEXPR bool valid_message_type(
    message_type type) CASTLE_NOEXCEPT
{
    return (type == message_type::sync)
        || (type == message_type::delay_request)
        || (type == message_type::pdelay_request) // LCOV_EXCL_BR_LINE
        || (type == message_type::pdelay_response) // LCOV_EXCL_BR_LINE
        || (type == message_type::follow_up)
        || (type == message_type::delay_response)
        || (type == message_type::pdelay_response_follow_up) // LCOV_EXCL_BR_LINE
        || (type == message_type::announce) // LCOV_EXCL_BR_LINE
        || (type == message_type::signaling) // LCOV_EXCL_BR_LINE
        || (type == message_type::management); // LCOV_EXCL_BR_LINE
}

CASTLE_NODISCARD CASTLE_INLINE castle::status encode_common_header(
    uint8_t* output,
    castle::size_type output_capacity,
    message_type type,
    uint8_t transport_specific,
    uint8_t domain_number,
    uint16_t message_length,
    uint16_t flags,
    int64_t correction_field,
    port_identity CASTLE_CONST& source,
    uint16_t sequence_id,
    uint8_t control_field,
    int8_t log_message_interval) CASTLE_NOEXCEPT
{
    if (output == nullptr)
    {
        return castle::status::invalid_argument;
    }
    if (output_capacity < message_length) // LCOV_EXCL_BR_LINE
    {
        return castle::status::full;
    }

    output[0U] = static_cast<uint8_t>(
        (static_cast<uint8_t>(transport_specific & 0x0FU) << 4U)
        | (static_cast<uint8_t>(type) & 0x0FU));
    output[1U] = PTP_VERSION;
    write_be16(&output[2U], message_length);
    output[4U] = domain_number;
    output[5U] = 0U;
    write_be16(&output[6U], flags);
    write_be64_signed(&output[8U], correction_field);
    write_be32(&output[16U], 0U);
    copy_port(&output[20U], source);
    write_be16(&output[30U], sequence_id);
    output[32U] = control_field;
    output[33U] = static_cast<uint8_t>(log_message_interval);
    return castle::status::ok;
}

} // namespace detail

/**
 * @brief Decode a complete PTP message from a transport payload.
 *
 * Ethernet and UDP headers must already have been removed. Extra bytes after
 * the PTP message are accepted, but the PTP message-length field must fit in
 * the supplied view.
 */
CASTLE_NODISCARD CASTLE_INLINE bool decode_message(
    castle::container::array_view<CASTLE_CONST uint8_t> frame,
    message_view& output) CASTLE_NOEXCEPT
{
    output = message_view{};
    if (frame.size() < PTP_HEADER_SIZE) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    CASTLE_CONST uint8_t version = static_cast<uint8_t>(frame[1U] & 0x0FU);
    CASTLE_CONST uint16_t message_length = detail::read_be16(&frame[2U]);
    if ((version != PTP_VERSION)
     || (message_length < PTP_HEADER_SIZE) // LCOV_EXCL_BR_LINE
     || (message_length > frame.size())) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    CASTLE_CONST message_type type = static_cast<message_type>(frame[0U] & 0x0FU);
    if (!detail::valid_message_type(type)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    output.header.transport_specific = static_cast<uint8_t>(frame[0U] >> 4U);
    output.header.type = type;
    output.header.version = version;
    output.header.message_length = message_length;
    output.header.domain_number = frame[4U];
    output.header.flags = detail::read_be16(&frame[6U]);
    output.header.correction_field = detail::read_be64_signed(&frame[8U]);

    for (castle::size_type index = 0U; index < 8U; ++index) // LCOV_EXCL_BR_LINE
    {
        output.header.source_port.clock_identity[index] = frame[20U + index];
    }
    output.header.source_port.port_number = detail::read_be16(&frame[28U]);
    output.header.sequence_id = detail::read_be16(&frame[30U]);
    output.header.control_field = frame[32U];
    output.header.log_message_interval = static_cast<int8_t>(frame[33U]);
    output.payload = castle::container::array_view<CASTLE_CONST uint8_t>(
        message_length == PTP_HEADER_SIZE ? nullptr : &frame[PTP_HEADER_SIZE],
        message_length - PTP_HEADER_SIZE); // LCOV_EXCL_BR_LINE
    return true;
}

/** @brief Read the message timestamp at the first payload octets. */
CASTLE_NODISCARD CASTLE_INLINE bool read_message_timestamp(
    message_view CASTLE_CONST& message,
    timestamp& output) CASTLE_NOEXCEPT
{
    if ((message.is(message_type::sync) // LCOV_EXCL_BR_LINE
      || message.is(message_type::follow_up) // LCOV_EXCL_BR_LINE
      || message.is(message_type::delay_response)) // LCOV_EXCL_BR_LINE
      && message.payload.size() >= PTP_TIMESTAMP_SIZE) // LCOV_EXCL_BR_LINE
    {
        return detail::read_timestamp(message.payload.data(), output);
    }
    return false;
}

/**
 * @brief Encode a Delay_Req event message.
 *
 * The origin timestamp is zero because this engine uses the hardware/software
 * transmit timestamp supplied by the transport boundary for the local t3 value.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_delay_request(
    uint8_t domain_number,
    port_identity CASTLE_CONST& source,
    uint16_t sequence_id,
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written,
    uint8_t transport_specific = 0U) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    castle::status result = detail::encode_common_header( // LCOV_EXCL_BR_LINE
        output,
        output_capacity,
        message_type::delay_request,
        transport_specific,
        domain_number,
        static_cast<uint16_t>(PTP_DELAY_REQUEST_SIZE),
        0U,
        0LL,
        source,
        sequence_id,
        1U,
        0x7F);

    if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
    {
        return result;
    }

    for (castle::size_type index = 0U; index < PTP_TIMESTAMP_SIZE; ++index) // LCOV_EXCL_BR_LINE
    {
        output[PTP_HEADER_SIZE + index] = 0U;
    }
    bytes_written = PTP_DELAY_REQUEST_SIZE;
    return castle::status::ok;
}

/**
 * @brief Fixed-storage PTPv2 end-to-end slave state machine.
 *
 * The class never performs communication or clock adjustment. The transport
 * adapter supplies RX/TX timestamps and sends the bounded Delay_Req buffer.
 * A separate servo can convert the resulting offset into a step or slew action.
 */
class slave CASTLE_FINAL
{
public:
    explicit slave(
        port_identity CASTLE_CONST& local_port,
        slave_config CASTLE_CONST& config = slave_config{}) CASTLE_NOEXCEPT
        : local_port_(local_port)
        , config_(config)
        , next_delay_sequence_(config.initial_delay_request_sequence)
    {
    }

    slave(slave CASTLE_CONST&) CASTLE_DELETE;
    slave& operator=(slave CASTLE_CONST&) CASTLE_DELETE;

    /** @brief Clears all in-flight exchange state while retaining configuration. */
    void reset() CASTLE_NOEXCEPT
    {
        pending_ = false;
        awaiting_follow_up_ = false;
        two_step_ = false;
        t1_valid_ = false;
        t2_valid_ = false;
        t3_valid_ = false;
        t4_valid_ = false;
        sync_correction_ = 0LL;
        delay_correction_ = 0LL;
        last_measurement_ = measurement{};
    }

    /**
     * @brief Consume one received PTP message.
     *
     * A matching Sync starts/restarts an exchange and generates one Delay_Req.
     * Matching Follow_Up and Delay_Resp messages are retained until every
     * timestamp required for the calculation is available.
     */
    CASTLE_NODISCARD castle::status receive(
        castle::container::array_view<CASTLE_CONST uint8_t> packet,
        timestamp CASTLE_CONST& receive_timestamp,
        slave_action& action) CASTLE_NOEXCEPT
    {
        action = slave_action{};
        if (!valid_timestamp(receive_timestamp))
        {
            return castle::status::invalid_argument;
        }

        message_view message{};
        if (!decode_message(packet, message))
        {
            return castle::status::invalid_argument;
        }
        if ((message.header.domain_number != config_.domain_number)
         || ((config_.transport_specific != PTP_ANY_TRANSPORT_SPECIFIC) // LCOV_EXCL_BR_LINE
         && (message.header.transport_specific != config_.transport_specific))) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_found;
        }

        switch (message.header.type) // LCOV_EXCL_BR_LINE
        {
            case message_type::sync:
            {
                return consume_sync(message, receive_timestamp, action);
            }
            case message_type::follow_up:
            {
                return consume_follow_up(message, action);
            }
            case message_type::delay_response:
            {
                return consume_delay_response(message, action);
            }
            default:
            {
                return castle::status::not_found;
            }
        }
    }

    /**
     * @brief Supply the TX timestamp of the most recently generated Delay_Req.
     *
     * Use the overload with `action` when the TX timestamp callback can arrive
     * after Delay_Resp, because the exchange can then complete from this call.
     */
    CASTLE_NODISCARD castle::status delay_request_transmitted(
        timestamp CASTLE_CONST& transmit_timestamp) CASTLE_NOEXCEPT
    {
        slave_action ignored{};
        return delay_request_transmitted(transmit_timestamp, ignored);
    }

    /** @brief Supply the TX timestamp and report a completion immediately if possible. */
    CASTLE_NODISCARD castle::status delay_request_transmitted(
        timestamp CASTLE_CONST& transmit_timestamp,
        slave_action& action) CASTLE_NOEXCEPT
    {
        action = slave_action{};
        if (!pending_)
        {
            return castle::status::not_configured;
        }
        if (!valid_timestamp(transmit_timestamp)) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }

        t3_ = transmit_timestamp;
        t3_valid_ = true;
        return try_complete(action);
    }

    /** @return `true` while one Sync-to-Delay_Resp exchange is outstanding. */
    CASTLE_NODISCARD bool pending() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return pending_;
    }

    /** @return The most recently completed measurement. */
    CASTLE_NODISCARD measurement CASTLE_CONST& last_measurement() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return last_measurement_;
    }

    /** @return The configured local PTP port identity. */
    CASTLE_NODISCARD port_identity CASTLE_CONST& local_port() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return local_port_;
    }

    /** @return The configured PTP domain number. */
    CASTLE_NODISCARD uint8_t domain_number() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return config_.domain_number;
    }

private:
    castle::status consume_sync(
        message_view CASTLE_CONST& message,
        timestamp CASTLE_CONST& receive_timestamp,
        slave_action& action) CASTLE_NOEXCEPT
    {
        pending_ = true;
        two_step_ = message.header.is_two_step();
        awaiting_follow_up_ = two_step_;
        t1_valid_ = false;
        t2_valid_ = true;
        t3_valid_ = false;
        t4_valid_ = false;
        master_ = message.header.source_port;
        sequence_id_ = message.header.sequence_id;
        t2_ = receive_timestamp;
        sync_correction_ = correction_to_nanoseconds(message.header.correction_field);
        delay_correction_ = 0LL;

        if (!message.header.is_two_step())
        {
            if (!read_message_timestamp(message, t1_))
            {
                pending_ = false;
                awaiting_follow_up_ = false;
                return castle::status::invalid_argument;
            }
            t1_valid_ = true;
        }

        action.send_delay_request = true;
        action.delay_request_sequence = next_delay_sequence_++;
        delay_sequence_id_ = action.delay_request_sequence;
        CASTLE_CONST castle::status result = encode_delay_request(
            config_.domain_number,
            local_port_,
            delay_sequence_id_,
            action.delay_request,
            sizeof(action.delay_request),
            action.delay_request_size,
            config_.transport_specific);
        if (!castle::succeeded(result))
        {
            pending_ = false;
            awaiting_follow_up_ = false;
            return result;
        }

        return try_complete(action);
    }

    castle::status consume_follow_up(
        message_view CASTLE_CONST& message,
        slave_action& action) CASTLE_NOEXCEPT
    {
        if (!pending_
         || !awaiting_follow_up_
         || !same_port(message.header.source_port, master_)
         || (message.header.sequence_id != sequence_id_))
        {
            return castle::status::not_found;
        }
        if (!read_message_timestamp(message, t1_))
        {
            return castle::status::invalid_argument;
        }

        t1_valid_ = true;
        awaiting_follow_up_ = false;
        return try_complete(action);
    }

    castle::status consume_delay_response(
        message_view CASTLE_CONST& message,
        slave_action& action) CASTLE_NOEXCEPT
    {
        if (!pending_
         || !same_port(message.header.source_port, master_)
         || (message.header.sequence_id != delay_sequence_id_)) // LCOV_EXCL_BR_LINE
        {
            return castle::status::not_found;
        }
        if (message.payload.size() < (PTP_TIMESTAMP_SIZE + PTP_TIMESTAMP_SIZE))
        {
            return castle::status::invalid_argument;
        }

        port_identity requesting_port{};
        for (castle::size_type index = 0U; index < 8U; ++index)
        {
            requesting_port.clock_identity[index] = message.payload[10U + index];
        }
        requesting_port.port_number = detail::read_be16(&message.payload[18U]);
        if (!same_port(requesting_port, local_port_))
        {
            return castle::status::not_found;
        }
        if (!detail::read_timestamp(message.payload.data(), t4_)) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }

        t4_valid_ = true;
        delay_correction_ = correction_to_nanoseconds(message.header.correction_field);
        return try_complete(action);
    }

    castle::status try_complete(slave_action& action) CASTLE_NOEXCEPT
    {
        if (!pending_ || !t1_valid_ || !t2_valid_ || !t3_valid_ || !t4_valid_) // LCOV_EXCL_BR_LINE
        {
            return castle::status::ok;
        }

        int64_t sync_interval = 0LL;
        int64_t delay_interval = 0LL;
        if (!timestamp_difference_checked(t2_, t1_, sync_interval)
         || !timestamp_difference_checked(t4_, t3_, delay_interval)) // LCOV_EXCL_BR_LINE
        {
            pending_ = false;
            awaiting_follow_up_ = false;
            return castle::status::out_of_range;
        }

        sync_interval -= sync_correction_;
        delay_interval -= delay_correction_;
        if ((delay_interval > 0LL && sync_interval > INT64_MAX - delay_interval)
         || (delay_interval < 0LL && sync_interval < INT64_MIN - delay_interval)) // LCOV_EXCL_BR_LINE
        {
            pending_ = false;
            awaiting_follow_up_ = false;
            return castle::status::out_of_range;
        }
        CASTLE_CONST int64_t total_path = sync_interval + delay_interval;
        if (total_path < 0LL)
        {
            pending_ = false;
            awaiting_follow_up_ = false;
            return castle::status::invalid_argument;
        }
        CASTLE_CONST int64_t path_delay = total_path / 2LL;
        CASTLE_CONST int64_t offset = sync_interval - path_delay;

        measurement sample{};
        sample.master_port = master_;
        sample.sync_sequence_id = sequence_id_;
        sample.delay_request_sequence_id = delay_sequence_id_;
        sample.two_step = two_step_;
        sample.master_sync = t1_;
        sample.slave_sync = t2_;
        sample.slave_delay_request = t3_;
        sample.master_delay_request = t4_;
        sample.offset_nanoseconds = offset;
        sample.path_delay_nanoseconds = path_delay;
        sample.correction_nanoseconds = sync_correction_ + delay_correction_;

        last_measurement_ = sample;
        action.measurement_ready = true;
        action.sample = sample;
        pending_ = false;
        awaiting_follow_up_ = false;
        t1_valid_ = false;
        t2_valid_ = false;
        t3_valid_ = false;
        t4_valid_ = false;
        return castle::status::ok;
    }

    port_identity local_port_{};
    port_identity master_{};
    slave_config config_{};
    uint16_t sequence_id_ = 0U;
    uint16_t delay_sequence_id_ = 0U;
    uint16_t next_delay_sequence_ = 0U;
    bool pending_ = false;
    bool awaiting_follow_up_ = false;
    bool two_step_ = false;
    bool t1_valid_ = false;
    bool t2_valid_ = false;
    bool t3_valid_ = false;
    bool t4_valid_ = false;
    timestamp t1_{};
    timestamp t2_{};
    timestamp t3_{};
    timestamp t4_{};
    int64_t sync_correction_ = 0LL;
    int64_t delay_correction_ = 0LL;
    measurement last_measurement_{};
};

} // namespace ptp
} // namespace timing
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_TIMING_PTP_V2_HPP
