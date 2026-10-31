// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file message.hpp
 * @brief RFC 1035 DNS question encoder and bounded response decoder.
 */
#ifndef CASTLE_EXT_PROTOCOLS_DNS_MESSAGE_HPP
#define CASTLE_EXT_PROTOCOLS_DNS_MESSAGE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/string_view.hpp"
#include "castle/utility/bytes.hpp"
#include "castle_ext/protocols/dns/types.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace dns
{

/** @brief Allocation-free RFC 1035 message codec. */
class message
{
public:
    static CASTLE_INLINE CASTLE_CONSTEXPR size_type header_size = 12U;
    static CASTLE_INLINE CASTLE_CONSTEXPR size_type max_message_size = 65535U;
    static CASTLE_INLINE CASTLE_CONSTEXPR size_type classic_udp_limit = 512U;

    /**
     * @brief Encodes one recursive IN-class query into caller-owned storage.
     * @param name ASCII domain name, with an optional trailing dot; root may be ".".
     * @param type Supported query type.
     * @param id Transaction identifier selected by the caller.
     * @param output Destination buffer.
     * @param capacity Destination capacity.
     * @param written Receives the encoded message length on success, otherwise zero.
     * @return `ok`, `invalid_argument` for malformed names/types, or `full` when capacity is insufficient.
     * @note Complexity is O(name.size()); no allocation occurs.
     */
    static castle::status encode_query(castle::container::string_view name,
                                       record_type type,
                                       uint16_t id,
                                       uint8_t* output,
                                       size_type capacity,
                                       size_type& written) CASTLE_NOEXCEPT
    {
        written = 0U;
        if (output == nullptr || capacity < header_size)
        {
            return output == nullptr ? castle::status::invalid_argument : castle::status::full;
        }
        if (!valid_query_type(type) || name.data() == nullptr || name.empty())
        {
            return castle::status::invalid_argument;
        }

        write_be16(output, id);
        output[2U] = 0x01U; // RD=1, QR=0, opcode=0
        output[3U] = 0x00U;
        output[4U] = 0x00U;
        output[5U] = 0x01U; // QDCOUNT
        output[6U] = 0U; output[7U] = 0U;
        output[8U] = 0U; output[9U] = 0U;
        output[10U] = 0U; output[11U] = 0U;

        size_type at = header_size;
        size_type begin = 0U;
        size_type end = name.size();
        if (end == 1U && name[0U] == '.')
        {
            if (at + 1U + 4U > capacity)
            {
                return castle::status::full;
            }
            output[at++] = 0U;
        }
        else
        {
            if (name[end - 1U] == '.')
            {
                --end;
            }
            // LCOV_EXCL_START
            // A name reduced to zero octets has already been handled by the root case above.
            if (end == 0U)
            {
                return castle::status::invalid_argument;
            }
            // LCOV_EXCL_STOP
            size_type wire_name_size = 1U;
            for (size_type i = 0U; i < end; ++i)
            {
                if (name[i] == '.')
                {
                    size_type label_size = i - begin;
                    if (label_size == 0U || label_size > 63U)
                    {
                        return castle::status::invalid_argument;
                    }
                    if (wire_name_size > 255U - (label_size + 1U))
                    {
                        return castle::status::out_of_range;
                    }
                    wire_name_size += label_size + 1U;
                    begin = i + 1U;
                }
                else if (static_cast<uint8_t>(name[i]) == 0U || static_cast<uint8_t>(name[i]) > 127U)
                {
                    return castle::status::invalid_argument;
                }
            }
            size_type last_label = end - begin;
            if (last_label == 0U || last_label > 63U || wire_name_size > 255U - (last_label + 1U))
            {
                return castle::status::invalid_argument;
            }
            wire_name_size += last_label + 1U;
            if (at + wire_name_size + 4U > capacity)
            {
                return castle::status::full;
            }

            begin = 0U;
            for (size_type i = 0U; i <= end; ++i)
            {
                if (i == end || name[i] == '.')
                {
                    size_type label_size = i - begin;
                    output[at++] = static_cast<uint8_t>(label_size);
                    for (size_type j = begin; j < i; ++j)
                    {
                        char c = name[j];
                        // Domain names are case-insensitive. Preserve caller spelling on the wire.
                        output[at++] = static_cast<uint8_t>(c);
                    }
                    begin = i + 1U;
                }
            }
            output[at++] = 0U;
        }

        write_be16(output + at, static_cast<uint16_t>(type)); at += 2U;
        write_be16(output + at, static_cast<uint16_t>(record_class::internet)); at += 2U;
        written = at;
        return castle::status::ok;
    }

    /** @brief Checks the DNS TC bit without requiring a complete DNS response. */
    static bool is_truncated_packet(CASTLE_CONST uint8_t* data, size_type size) CASTLE_NOEXCEPT
    {
        return data != nullptr && size >= 4U && (read_be16(data + 2U) & 0x0200U) != 0U;
    }

    /**
     * @brief Decodes one DNS response, including compressed owner and name-bearing RDATA fields.
     * @param data Complete DNS message.
     * @param size Message size.
     * @param expected_id Transaction ID that must match.
     * @param output Fixed-capacity response destination; cleared before parsing.
     * @return `ok`, `invalid_argument` for malformed/mismatched packets, or `full` when record capacity is exceeded.
     */
    template <size_type MaxRecords>
    static castle::status parse_response(CASTLE_CONST uint8_t* data,
                                         size_type size,
                                         uint16_t expected_id,
                                         response<MaxRecords>& output) CASTLE_NOEXCEPT
    {
        output.clear();
        if (data == nullptr || size < header_size || size > max_message_size)
        {
            return castle::status::invalid_argument;
        }
        output.transaction_id = read_be16(data);
        if (output.transaction_id != expected_id)
        {
            return castle::status::invalid_argument;
        }

        output.flags = read_be16(data + 2U);
        if ((output.flags & 0x8000U) == 0U || (output.flags & 0x7800U) != 0U ||
            (output.flags & 0x0070U) != 0U)
        {
            return castle::status::invalid_argument; // not a response, unsupported opcode, or reserved Z bits set
        }
        output.authoritative = (output.flags & 0x0400U) != 0U;
        output.truncated = (output.flags & 0x0200U) != 0U;
        output.recursion_desired = (output.flags & 0x0100U) != 0U;
        output.recursion_available = (output.flags & 0x0080U) != 0U;
        output.rcode = static_cast<response_code>(output.flags & 0x000FU);

        output.question_count = read_be16(data + 4U);
        output.answer_count = read_be16(data + 6U);
        output.authority_count = read_be16(data + 8U);
        output.additional_count = read_be16(data + 10U);
        if (output.question_count != 1U)
        {
            return castle::status::invalid_argument;
        }

        size_type at = header_size;
        size_type consumed = 0U;
        castle::status result = decode_name(data, size, at, output.question_name,
                                            sizeof(output.question_name), consumed);
        if (!castle::succeeded(result))
        {
            return result;
        }
        at += consumed;
        if (at > size || size - at < 4U) // LCOV_EXCL_BR_LINE
        {
            return castle::status::invalid_argument;
        }
        uint16_t question_type_value = read_be16(data + at); at += 2U;
        uint16_t question_class_value = read_be16(data + at); at += 2U;
        output.question_type = static_cast<record_type>(question_type_value);
        output.question_class = static_cast<record_class>(question_class_value);

        uint32_t total_records = static_cast<uint32_t>(output.answer_count) +
                                 static_cast<uint32_t>(output.authority_count) +
                                 static_cast<uint32_t>(output.additional_count);
        for (uint32_t index = 0U; index < total_records; ++index)
        {
            section source = section::answer;
            if (index >= output.answer_count + static_cast<uint32_t>(output.authority_count))
            {
                source = section::additional;
            }
            else if (index >= output.answer_count)
            {
                source = section::authority;
            }

            char owner[256U]{};
            result = decode_name(data, size, at, owner, sizeof(owner), consumed);
            if (!castle::succeeded(result))
            {
                return result;
            }
            at += consumed;
            if (at > size || size - at < 10U) // LCOV_EXCL_BR_LINE
            {
                return castle::status::invalid_argument;
            }
            uint16_t type_value = read_be16(data + at); at += 2U;
            uint16_t class_value = read_be16(data + at); at += 2U;
            uint32_t ttl = read_be32(data + at); at += 4U;
            uint16_t rdlength = read_be16(data + at); at += 2U;
            if (at > size || static_cast<size_type>(rdlength) > size - at) // LCOV_EXCL_BR_LINE
            {
                return castle::status::invalid_argument;
            }
            size_type rdata = at;
            at += rdlength;

            if (class_value != static_cast<uint16_t>(record_class::internet))
            {
                continue;
            }
            if (type_value != static_cast<uint16_t>(record_type::a) &&
                type_value != static_cast<uint16_t>(record_type::aaaa) &&
                type_value != static_cast<uint16_t>(record_type::cname) &&
                type_value != static_cast<uint16_t>(record_type::ns) &&
                type_value != static_cast<uint16_t>(record_type::mx))
            {
                continue; // Unknown RR types remain safely skippable by RDLENGTH.
            }
            if (output.record_count >= MaxRecords)
            {
                return castle::status::full;
            }

            resource_record& record = output.records[output.record_count];
            record.type = static_cast<record_type>(type_value);
            record.klass = record_class::internet;
            record.source_section = source;
            record.ttl = ttl;
            // LCOV_EXCL_START
            // Owner text is at most 255 octets, so it always fits record.owner (256 octets).
            if (!copy_text(owner, record.owner, sizeof(record.owner)))
            {
                return castle::status::out_of_range;
            }
            // LCOV_EXCL_STOP

            if (record.type == record_type::a)
            {
                if (rdlength != 4U)
                {
                    return castle::status::invalid_argument;
                }
                record.address = net::endpoint::ipv4(data[rdata], data[rdata + 1U],
                                                      data[rdata + 2U], data[rdata + 3U], 0U);
            }
            else if (record.type == record_type::aaaa)
            {
                if (rdlength != 16U)
                {
                    return castle::status::invalid_argument;
                }
                record.address = net::endpoint::ipv6(data + rdata, 0U);
            }
            else if (record.type == record_type::mx)
            {
                if (rdlength < 3U)
                {
                    return castle::status::invalid_argument;
                }
                record.preference = read_be16(data + rdata);
                result = decode_name(data, size, rdata + 2U, record.target,
                                     sizeof(record.target), consumed);
                if (!castle::succeeded(result) || consumed + 2U != rdlength)
                {
                    return castle::status::invalid_argument;
                }
            }
            else
            {
                result = decode_name(data, size, rdata, record.target,
                                     sizeof(record.target), consumed);
                if (!castle::succeeded(result) || consumed != rdlength)
                {
                    return castle::status::invalid_argument;
                }
            }
            ++output.record_count;
        }
        return at == size ? castle::status::ok : castle::status::invalid_argument;
    }

    /**
     * @brief Decodes a possibly compressed domain name from an arbitrary packet offset.
     * @param data Complete containing DNS message.
     * @param size Message size.
     * @param offset Offset of the name in the message.
     * @param output Null-terminated output buffer.
     * @param output_capacity Output buffer capacity, including terminator.
     * @param consumed Number of original-stream octets consumed (pointer targets are not counted).
     * @return `ok`, `invalid_argument` for malformed pointers/labels, or `full` for insufficient output capacity.
     */
    static castle::status decode_name(CASTLE_CONST uint8_t* data,
                                      size_type size,
                                      size_type offset,
                                      char* output,
                                      size_type output_capacity,
                                      size_type& consumed) CASTLE_NOEXCEPT
    {
        consumed = 0U;
        if (data == nullptr || output == nullptr || output_capacity == 0U || offset >= size)
        {
            return castle::status::invalid_argument;
        }
        size_type cursor = offset;
        size_type output_size = 0U;
        size_type hops = 0U;
        size_type expanded_wire_size = 0U;
        bool jumped = false;
        bool finished = false;
        while (!finished) // LCOV_EXCL_BR_LINE
        {
            if (cursor >= size || ++hops > 128U)
            {
                return castle::status::invalid_argument;
            }
            uint8_t length = data[cursor];
            if ((length & 0xC0U) == 0xC0U)
            {
                if (cursor + 1U >= size)
                {
                    return castle::status::invalid_argument;
                }
                uint16_t pointer = static_cast<uint16_t>(((length & 0x3FU) << 8U) | data[cursor + 1U]);
                if (pointer >= size || pointer >= cursor)
                {
                    return castle::status::invalid_argument;
                }
                if (!jumped)
                {
                    consumed += 2U;
                }
                cursor = pointer;
                jumped = true;
                continue;
            }
            if ((length & 0xC0U) != 0U || length > 63U) // LCOV_EXCL_BR_LINE
            {
                return castle::status::invalid_argument;
            }
            ++cursor;
            if (!jumped)
            {
                ++consumed;
            }
            if (expanded_wire_size >= 255U)
            {
                return castle::status::invalid_argument;
            }
            ++expanded_wire_size;
            if (length == 0U)
            {
                finished = true;
                break;
            }
            if (cursor > size || static_cast<size_type>(length) > size - cursor || // LCOV_EXCL_BR_LINE
                expanded_wire_size + static_cast<size_type>(length) > 255U)
            {
                return castle::status::invalid_argument;
            }
            expanded_wire_size += length;
            if (output_size != 0U)
            {
                if (output_size + 1U >= output_capacity)
                {
                    return castle::status::full;
                }
                output[output_size++] = '.';
            }
            if (static_cast<size_type>(length) > output_capacity - 1U - output_size)
            {
                return castle::status::full;
            }
            for (uint8_t i = 0U; i < length; ++i)
            {
                uint8_t c = data[cursor + i];
                if (c == 0U || c > 127U)
                {
                    return castle::status::invalid_argument;
                }
                output[output_size++] = static_cast<char>(c);
            }
            cursor += length;
            if (!jumped)
            {
                consumed += length;
            }
        }
        output[output_size] = '\0';
        return castle::status::ok;
    }

private:
    static bool valid_query_type(record_type type) CASTLE_NOEXCEPT
    {
        return type == record_type::a || type == record_type::aaaa || type == record_type::cname ||
               type == record_type::ns || type == record_type::mx;
    }

    static bool copy_text(CASTLE_CONST char* source, char* destination, size_type capacity) CASTLE_NOEXCEPT
    {
        size_type i = 0U;
        while (source[i] != '\0')
        {
            // LCOV_EXCL_START
            // Callers pass at most 255 octets, so the destination can never run out.
            if (i + 1U >= capacity)
            {
                return false;
            }
            // LCOV_EXCL_STOP
            destination[i] = source[i];
            ++i;
        }
        destination[i] = '\0';
        return true;
    }
};

} // namespace dns
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_DNS_MESSAGE_HPP
