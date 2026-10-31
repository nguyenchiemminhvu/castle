// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file codec.hpp
 * @brief MQTT fixed-header, Remaining Length, packet cursor and bounded property codecs.
 */
#ifndef CASTLE_EXT_PROTOCOLS_MQTT_CODEC_HPP
#define CASTLE_EXT_PROTOCOLS_MQTT_CODEC_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bytes.hpp"
#include "castle_ext/protocols/mqtt/types.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace mqtt
{

static CASTLE_CONSTEXPR uint32_t maximum_remaining_length = 268435455U;

/** @brief Decoded view of one complete MQTT packet in a caller-owned TCP byte stream. */
struct packet_view
{
    packet_type type = packet_type::reserved;
    uint8_t flags = 0U;
    uint32_t remaining_length = 0U;
    size_type header_size = 0U;
    size_type packet_size = 0U;
    uint8_t CASTLE_CONST* body = nullptr;
};

/**
 * @brief Encodes MQTT Remaining Length into its one-to-four-byte base-128 form.
 * @return `ok`, or `out_of_range` above the MQTT maximum.
 */
CASTLE_INLINE castle::status encode_remaining_length(uint32_t value, uint8_t output[4U], size_type& written) CASTLE_NOEXCEPT
{
    written = 0U;
    if (output == nullptr)
    {
        return castle::status::invalid_argument;
    }
    if (value > maximum_remaining_length)
    {
        return castle::status::out_of_range;
    }
    do
    {
        uint8_t digit = static_cast<uint8_t>(value % 128U);
        value /= 128U;
        if (value != 0U)
        {
            digit |= 0x80U;
        }
        output[written++] = digit;
    } while (value != 0U && written < 4U); // LCOV_EXCL_BR_LINE
    return castle::status::ok;
}

/**
 * @brief Decodes MQTT Remaining Length; `empty` means more stream bytes are needed.
 * @param consumed Number of encoded length bytes on success.
 */
CASTLE_INLINE castle::status decode_remaining_length(uint8_t CASTLE_CONST* input, size_type size,
                                               uint32_t& value, size_type& consumed) CASTLE_NOEXCEPT
{
    value = 0U;
    consumed = 0U;
    if (input == nullptr && size != 0U)
    {
        return castle::status::invalid_argument;
    }
    uint32_t multiplier = 1U;
    for (size_type i = 0U; i < 4U; ++i)
    {
        if (i >= size)
        {
            return castle::status::empty;
        }
        uint8_t digit = input[i];
        value += static_cast<uint32_t>(digit & 0x7FU) * multiplier;
        ++consumed;
        if ((digit & 0x80U) == 0U)
        {
            if (value > maximum_remaining_length) // LCOV_EXCL_BR_LINE
            {
                return castle::status::out_of_range;
            }
            if (i != 0U)
            {
                uint32_t minimum = 1U;
                for (size_type j = 0U; j < i; ++j)
                {
                    minimum *= 128U;
                }
                if (value < minimum)
                {
                    return castle::status::invalid_argument;
                }
            }
            return castle::status::ok;
        }
        multiplier *= 128U;
    }
    return castle::status::invalid_argument; // continuation bit on the fourth byte
}

/** @brief Checks that fixed-header flags are valid for a packet type. */
CASTLE_INLINE bool valid_fixed_header(packet_type type, uint8_t flags) CASTLE_NOEXCEPT
{
    if (type == packet_type::publish)
    {
        uint8_t quality = static_cast<uint8_t>((flags >> 1U) & 0x03U);
        return quality != 3U && !(quality == 0U && (flags & 0x08U) != 0U);
    }
    if (type == packet_type::pubrel || type == packet_type::subscribe || type == packet_type::unsubscribe)
    {
        return flags == 0x02U || flags == 0x0AU;
    }
    return flags == 0U;
}

/**
 * @brief Inspects a TCP stream and returns a packet view when a complete MQTT packet is available.
 * @return `empty` for an incomplete frame, `invalid_argument` for malformed headers, `out_of_range`
 *         for a frame beyond `maximum_packet_size`, otherwise `ok`.
 */
CASTLE_INLINE castle::status inspect_packet(uint8_t CASTLE_CONST* input, size_type size,
                                     size_type maximum_packet_size, packet_view& output) CASTLE_NOEXCEPT
{
    output = packet_view{};
    if (input == nullptr || maximum_packet_size < 2U)
    {
        return castle::status::invalid_argument;
    }
    if (size < 2U)
    {
        return castle::status::empty;
    }
    uint8_t type_value = static_cast<uint8_t>(input[0U] >> 4U);
    uint8_t flags = static_cast<uint8_t>(input[0U] & 0x0FU);
    if (type_value == 0U || !valid_fixed_header(static_cast<packet_type>(type_value), flags))
    {
        return castle::status::invalid_argument;
    }
    uint32_t remaining = 0U;
    size_type length_bytes = 0U;
    castle::status result = decode_remaining_length(input + 1U, size - 1U, remaining, length_bytes);
    if (!castle::succeeded(result))
    {
        return result;
    }
    size_type header_size = 1U + length_bytes;
    if (remaining > maximum_packet_size || header_size > maximum_packet_size - remaining)
    {
        return castle::status::out_of_range;
    }
    size_type packet_size = header_size + static_cast<size_type>(remaining);
    if (size < packet_size)
    {
        return castle::status::empty;
    }
    output.type = static_cast<packet_type>(type_value);
    output.flags = flags;
    output.remaining_length = remaining;
    output.header_size = header_size;
    output.packet_size = packet_size;
    output.body = input + header_size;
    return castle::status::ok;
}

/** @brief Fixed-capacity output cursor used by packet encoders. */
class packet_writer
{
public:
    packet_writer(uint8_t* output, size_type capacity) CASTLE_NOEXCEPT
        : output_(output), capacity_(capacity), size_(0U), result_(output == nullptr && capacity != 0U ? castle::status::invalid_argument : castle::status::ok) {}

    castle::status status() CASTLE_CONST CASTLE_NOEXCEPT { return result_; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    uint8_t CASTLE_CONST* data() CASTLE_CONST CASTLE_NOEXCEPT { return output_; }
    uint8_t* data() CASTLE_NOEXCEPT { return output_; }

    castle::status put_u8(uint8_t value) CASTLE_NOEXCEPT
    {
        if (!castle::succeeded(result_))
        {
            return result_;
        }
        if (size_ >= capacity_)
        {
            return result_ = castle::status::full;
        }
        output_[size_++] = value;
        return result_;
    }
    castle::status put_u16(uint16_t value) CASTLE_NOEXCEPT
    {
        if (capacity_ - size_ < 2U)
        {
            return result_ = castle::status::full;
        }
        write_be16(output_ + size_, value); size_ += 2U; return result_;
    }
    castle::status put_u32(uint32_t value) CASTLE_NOEXCEPT
    {
        if (capacity_ - size_ < 4U)
        {
            return result_ = castle::status::full;
        }
        write_be32(output_ + size_, value); size_ += 4U; return result_;
    }
    castle::status put_bytes(uint8_t CASTLE_CONST* bytes, size_type count) CASTLE_NOEXCEPT
    {
        if (bytes == nullptr && count != 0U)
        {
            return result_ = castle::status::invalid_argument;
        }
        if (count > capacity_ - size_)
        {
            return result_ = castle::status::full;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            output_[size_ + i] = bytes[i];
        }
        size_ += count; return result_;
    }
    castle::status put_utf8(castle::container::string_view value) CASTLE_NOEXCEPT
    {
        if (value.size() > 65535U)
        {
            return result_ = castle::status::out_of_range;
        }
        castle::status status = put_u16(static_cast<uint16_t>(value.size()));
        return castle::succeeded(status) ? put_bytes(reinterpret_cast<uint8_t CASTLE_CONST*>(value.data()), value.size()) : status;
    }
    castle::status put_binary(uint8_t CASTLE_CONST* value, size_type count) CASTLE_NOEXCEPT
    {
        if (count > 65535U)
        {
            return result_ = castle::status::out_of_range;
        }
        castle::status status = put_u16(static_cast<uint16_t>(count));
        return castle::succeeded(status) ? put_bytes(value, count) : status;
    }
    castle::status put_varint(uint32_t value) CASTLE_NOEXCEPT
    {
        uint8_t encoded[4U]{}; size_type count = 0U;
        castle::status status = encode_remaining_length(value, encoded, count);
        return castle::succeeded(status) ? put_bytes(encoded, count) : (result_ = status);
    }
    void set_size(size_type value) CASTLE_NOEXCEPT
    {
        if (value <= capacity_)
        {
            size_ = value;
        }
    }

private:
    uint8_t* output_;
    size_type capacity_;
    size_type size_;
    castle::status result_;
};

/** @brief Bounds-checked input cursor for packet bodies. */
class packet_reader
{
public:
    packet_reader(uint8_t CASTLE_CONST* input, size_type size) CASTLE_NOEXCEPT
        : input_(input), size_(size), position_(0U), result_(input == nullptr && size != 0U ? castle::status::invalid_argument : castle::status::ok) {}

    castle::status status() CASTLE_CONST CASTLE_NOEXCEPT { return result_; }
    size_type remaining() CASTLE_CONST CASTLE_NOEXCEPT { return position_ <= size_ ? size_ - position_ : 0U; } // LCOV_EXCL_BR_LINE
    size_type position() CASTLE_CONST CASTLE_NOEXCEPT { return position_; }
    uint8_t CASTLE_CONST* current() CASTLE_CONST CASTLE_NOEXCEPT { return input_ == nullptr ? nullptr : input_ + position_; }

    castle::status get_u8(uint8_t& value) CASTLE_NOEXCEPT
    {
        if (remaining() < 1U)
        {
            return result_ = castle::status::invalid_argument;
        }
        value = input_[position_++]; return result_;
    }
    castle::status get_u16(uint16_t& value) CASTLE_NOEXCEPT
    {
        if (remaining() < 2U)
        {
            return result_ = castle::status::invalid_argument;
        }
        value = read_be16(input_ + position_); position_ += 2U; return result_;
    }
    castle::status get_u32(uint32_t& value) CASTLE_NOEXCEPT
    {
        if (remaining() < 4U)
        {
            return result_ = castle::status::invalid_argument;
        }
        value = read_be32(input_ + position_); position_ += 4U; return result_;
    }
    castle::status get_utf8(char* output, size_type capacity, size_type& length) CASTLE_NOEXCEPT
    {
        length = 0U;
        uint16_t count = 0U;
        if (!castle::succeeded(get_u16(count)))
        {
            return result_;
        }
        if (count >= capacity || remaining() < count || (output == nullptr && capacity != 0U)) // LCOV_EXCL_BR_LINE
        {
            return result_ = count >= capacity ? castle::status::out_of_range : castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            output[i] = static_cast<char>(input_[position_ + i]);
        }
        output[count] = '\0'; position_ += count; length = count; return result_;
    }
    castle::status get_binary(uint8_t* output, size_type capacity, size_type& length) CASTLE_NOEXCEPT
    {
        length = 0U;
        uint16_t count = 0U;
        if (!castle::succeeded(get_u16(count)))
        {
            return result_;
        }
        if (count > capacity || remaining() < count || (output == nullptr && count != 0U))
        {
            return result_ = count > capacity ? castle::status::out_of_range : castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            output[i] = input_[position_ + i];
        }
        position_ += count; length = count; return result_;
    }
    castle::status get_varint(uint32_t& value) CASTLE_NOEXCEPT
    {
        size_type used = 0U;
        uint8_t CASTLE_CONST* start = input_ == nullptr ? nullptr : input_ + position_;
        castle::status status = decode_remaining_length(start, remaining(), value, used);
        if (!castle::succeeded(status))
        {
            return result_ = status;
        }
        position_ += used; return result_;
    }
    castle::status skip(size_type count) CASTLE_NOEXCEPT
    {
        if (count > remaining())
        {
            return result_ = castle::status::invalid_argument;
        }
        position_ += count; return result_;
    }

private:
    uint8_t CASTLE_CONST* input_;
    size_type size_;
    size_type position_;
    castle::status result_;
};

namespace detail
{
CASTLE_INLINE bool property_is_byte(property_id id) CASTLE_NOEXCEPT
{
    return id == property_id::payload_format_indicator || id == property_id::request_problem_information ||
           id == property_id::request_response_information || id == property_id::maximum_qos ||
           id == property_id::retain_available || id == property_id::wildcard_subscription_available ||
           id == property_id::subscription_identifier_available || id == property_id::shared_subscription_available;
}
CASTLE_INLINE bool property_is_u16(property_id id) CASTLE_NOEXCEPT
{
    return id == property_id::receive_maximum || id == property_id::topic_alias_maximum || id == property_id::topic_alias || id == property_id::server_keep_alive;
}
CASTLE_INLINE bool property_is_u32(property_id id) CASTLE_NOEXCEPT
{
    return id == property_id::message_expiry_interval || id == property_id::session_expiry_interval ||
           id == property_id::will_delay_interval || id == property_id::maximum_packet_size;
}
CASTLE_INLINE bool property_is_varint(property_id id) CASTLE_NOEXCEPT { return id == property_id::subscription_identifier; }
CASTLE_INLINE bool property_is_binary(property_id id) CASTLE_NOEXCEPT
{
    return id == property_id::correlation_data || id == property_id::authentication_data;
}
CASTLE_INLINE bool property_is_string(property_id id) CASTLE_NOEXCEPT
{
    return id == property_id::content_type || id == property_id::response_topic ||
           id == property_id::assigned_client_identifier || id == property_id::authentication_method ||
           id == property_id::response_information || id == property_id::server_reference || id == property_id::reason_string;
}
CASTLE_INLINE bool property_known(uint32_t id) CASTLE_NOEXCEPT
{
    switch (static_cast<property_id>(id))
    {
        case property_id::payload_format_indicator: case property_id::message_expiry_interval:
        case property_id::content_type: case property_id::response_topic: case property_id::correlation_data:
        case property_id::subscription_identifier: case property_id::session_expiry_interval:
        case property_id::assigned_client_identifier: case property_id::server_keep_alive:
        case property_id::authentication_method: case property_id::authentication_data:
        case property_id::request_problem_information: case property_id::will_delay_interval:
        case property_id::request_response_information: case property_id::response_information:
        case property_id::server_reference: case property_id::reason_string: case property_id::receive_maximum:
        case property_id::topic_alias_maximum: case property_id::topic_alias: case property_id::maximum_qos:
        case property_id::retain_available: case property_id::user_property: case property_id::maximum_packet_size:
        case property_id::wildcard_subscription_available: case property_id::subscription_identifier_available:
        case property_id::shared_subscription_available: return true;
        default: return false;
    }
}
CASTLE_INLINE bool property_is_pair(property_id id) CASTLE_NOEXCEPT { return id == property_id::user_property; }
} // namespace detail

/** @brief Encodes a bounded MQTT 5 property list with its leading Property Length. */
template <size_type Count, size_type StrCap, size_type BinCap>
CASTLE_INLINE castle::status encode_properties(property_list<Count, StrCap, BinCap> CASTLE_CONST& properties,
                                        uint8_t* output, size_type capacity, size_type& written) CASTLE_NOEXCEPT
{
    written = 0U;
    if (output == nullptr)
    {
        return castle::status::invalid_argument;
    }
    if (properties.count > Count)
    {
        return castle::status::invalid_argument;
    }
    if (capacity < 4U)
    {
        return castle::status::full;
    }
    uint8_t body[Count * (StrCap * 2U + BinCap + 12U) + 1U]{};
    packet_writer writer(body, sizeof(body));
    for (size_type i = 0U; i < properties.count; ++i)
    {
        auto CASTLE_CONST& item = properties.entries[i];
        castle::status result = writer.put_varint(static_cast<uint8_t>(item.id));
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }
        if (detail::property_is_byte(item.id))
        {
            result = writer.put_u8(static_cast<uint8_t>(item.integer));
        }
        else if (detail::property_is_u16(item.id))
        {
            result = writer.put_u16(static_cast<uint16_t>(item.integer));
        }
        else if (detail::property_is_u32(item.id))
        {
            result = writer.put_u32(item.integer);
        }
        else if (detail::property_is_varint(item.id))
        {
            result = writer.put_varint(item.integer);
        }
        else if (detail::property_is_binary(item.id))
        {
            result = writer.put_binary(item.binary_value, item.binary_size);
        }
        else if (detail::property_is_pair(item.id))
        {
            result = writer.put_utf8(castle::container::string_view(item.string_value));
            if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
            {
                result = writer.put_utf8(castle::container::string_view(item.user_value));
            }
        }
        else if (detail::property_is_string(item.id))
        {
            result = writer.put_utf8(castle::container::string_view(item.string_value));
        }
        else
        {
            return castle::status::invalid_argument;
        }
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }
    }
    packet_writer target(output, capacity);
    castle::status result = target.put_varint(static_cast<uint32_t>(writer.size()));
    if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
    {
        result = target.put_bytes(writer.data(), writer.size());
    }
    if (castle::succeeded(result))
    {
        written = target.size();
    }
    return result;
}

/** @brief Decodes a bounded MQTT 5 property section. Unknown property IDs are rejected by default. */
template <size_type Count, size_type StrCap, size_type BinCap>
CASTLE_INLINE castle::status decode_properties(packet_reader& reader,
                                        property_list<Count, StrCap, BinCap>& output,
                                        bool ignore_unknown = false) CASTLE_NOEXCEPT
{
    output.clear();
    uint32_t property_length = 0U;
    castle::status result = reader.get_varint(property_length);
    if (!castle::succeeded(result))
    {
        return result;
    }
    if (property_length > reader.remaining())
    {
        return castle::status::invalid_argument;
    }
    uint8_t CASTLE_CONST* start = reader.current();
    packet_reader properties(start, property_length);
    while (properties.remaining() != 0U)
    {
        uint32_t raw_id = 0U;
        result = properties.get_varint(raw_id);
        if (!castle::succeeded(result) || !detail::property_known(raw_id))
        {
            return ignore_unknown ? reader.skip(property_length) : castle::status::invalid_argument;
        }
        property_id id = static_cast<property_id>(raw_id);
        if (output.full())
        {
            return castle::status::full;
        }
        auto& item = output.entries[output.count];
        item.id = id;
        if (detail::property_is_byte(id))
        {
            uint8_t value = 0U;
            result = properties.get_u8(value);
            item.integer = value;
            if (castle::succeeded(result) &&
                ((id == property_id::payload_format_indicator && value > 1U) ||
                 (id != property_id::payload_format_indicator && id != property_id::maximum_qos && value > 1U) ||
                 (id == property_id::maximum_qos && value > 1U)))
            {
                return castle::status::invalid_argument;
            }
        }
        else if (detail::property_is_u16(id))
        {
            uint16_t value = 0U;
            result = properties.get_u16(value);
            item.integer = value;
            if (castle::succeeded(result) && value == 0U && (id == property_id::receive_maximum || id == property_id::topic_alias))
            {
                return castle::status::invalid_argument;
            }
        }
        else if (detail::property_is_u32(id))
        {
            result = properties.get_u32(item.integer);
        }
        else if (detail::property_is_varint(id))
        {
            result = properties.get_varint(item.integer);
            if (castle::succeeded(result) && item.integer == 0U)
            {
                return castle::status::invalid_argument;
            }
        }
        else if (detail::property_is_binary(id))
        {
            result = properties.get_binary(item.binary_value, BinCap, item.binary_size);
        }
        else if (detail::property_is_string(id) || detail::property_is_pair(id)) // LCOV_EXCL_BR_LINE
        {
            size_type size = 0U;
            result = properties.get_utf8(item.string_value, StrCap, size);
            if (castle::succeeded(result) && detail::property_is_pair(id))
            {
                result = properties.get_utf8(item.user_value, StrCap, size);
            }
        }
        // LCOV_EXCL_START
        // can not cover branches code: every identifier in property_known() is classified above.
        else
        {
            result = castle::status::invalid_argument;
        }
        // LCOV_EXCL_STOP
        if (!castle::succeeded(result))
        {
            return result;
        }
        ++output.count;
    }
    return reader.skip(property_length);
}

/** @brief Encodes a complete MQTT Control Packet to caller-owned storage. */
CASTLE_INLINE castle::status encode_packet(packet_type type, uint8_t flags,
                                    uint8_t CASTLE_CONST* body, size_type body_size,
                                    uint8_t* output, size_type capacity, size_type& written) CASTLE_NOEXCEPT
{
    written = 0U;
    if (output == nullptr || (body == nullptr && body_size != 0U))
    {
        return castle::status::invalid_argument;
    }
    if (!valid_fixed_header(type, flags))
    {
        return castle::status::invalid_argument;
    }
    if (body_size > maximum_remaining_length)
    {
        return castle::status::out_of_range;
    }
    uint8_t length[4U]{}; size_type length_size = 0U;
    castle::status result = encode_remaining_length(static_cast<uint32_t>(body_size), length, length_size);
    if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
    {
        return result;
    }
    if (capacity < 1U + length_size || body_size > capacity - 1U - length_size)
    {
        return castle::status::full;
    }
    output[0U] = static_cast<uint8_t>((static_cast<uint8_t>(type) << 4U) | flags);
    for (size_type i = 0U; i < length_size; ++i)
    {
        output[1U + i] = length[i];
    }
    for (size_type i = 0U; i < body_size; ++i)
    {
        output[1U + length_size + i] = body[i];
    }
    written = 1U + length_size + body_size;
    return castle::status::ok;
}

} // namespace mqtt
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_MQTT_CODEC_HPP
