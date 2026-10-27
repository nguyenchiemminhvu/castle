#include <gtest/gtest.h>
#include "castle_ext/protocols/dns/message.hpp"

namespace
{
using castle::size_type;
using castle::status;
using castle::protocols::dns::message;
using castle::protocols::dns::record_type;
using castle::protocols::dns::response;

size_type make_question(uint8_t* packet, char const* domain = "www.example.com", record_type type = record_type::a)
{
    size_type size = 0U;
    EXPECT_EQ(message::encode_query(domain, type, 0x1234U, packet, 512U, size), status::ok);
    packet[2U] = 0x81U;
    packet[3U] = 0x80U;
    return size;
}

size_type append_a(uint8_t* packet, size_type at, uint8_t last = 34U)
{
    packet[at++] = 0xC0U; packet[at++] = 0x0CU;
    packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 60U;
    packet[at++] = 0U; packet[at++] = 4U;
    packet[at++] = 192U; packet[at++] = 0U; packet[at++] = 2U; packet[at++] = last;
    packet[7U] = 1U;
    return at;
}
}

TEST(DnsMessage, EncodesAQuestionAndRejectsInvalidInput)
{
    uint8_t packet[512U]{};
    size_type size = 0U;
    EXPECT_EQ(message::encode_query("www.example.com", record_type::a, 0xBEEFU,
                                    packet, sizeof(packet), size), status::ok);
    EXPECT_EQ(size, 33U);
    EXPECT_EQ(packet[0U], 0xBEU);
    EXPECT_EQ(packet[1U], 0xEFU);
    EXPECT_EQ(packet[2U], 1U);
    EXPECT_EQ(packet[5U], 1U);
    EXPECT_EQ(packet[12U], 3U);
    EXPECT_EQ(packet[16U], 7U);
    EXPECT_EQ(packet[24U], 3U);
    EXPECT_EQ(packet[28U], 0U);
    EXPECT_EQ(packet[29U], 0U);
    EXPECT_EQ(packet[30U], 1U);
    EXPECT_EQ(packet[32U], 1U);
    EXPECT_EQ(message::encode_query("", record_type::a, 1U, packet, sizeof(packet), size), status::invalid_argument);
    EXPECT_EQ(message::encode_query("example..com", record_type::a, 1U, packet, sizeof(packet), size), status::invalid_argument);
    EXPECT_EQ(message::encode_query("a", static_cast<record_type>(99U), 1U, packet, sizeof(packet), size), status::invalid_argument);
    EXPECT_EQ(message::encode_query("example.com", record_type::a, 1U, packet, 12U, size), status::full);
    EXPECT_EQ(message::encode_query("example.com", record_type::a, 1U, nullptr, 512U, size), status::invalid_argument);
}

TEST(DnsMessage, EncodesRootAndRejectsLongLabels)
{
    uint8_t packet[512U]{};
    size_type size = 0U;
    EXPECT_EQ(message::encode_query(".", record_type::aaaa, 9U, packet, sizeof(packet), size), status::ok);
    EXPECT_EQ(size, 17U);
    EXPECT_EQ(packet[12U], 0U);
    char too_long[65U];
    for (size_type i = 0U; i < 64U; ++i) { too_long[i] = 'a'; }
    too_long[64U] = '\0';
    EXPECT_EQ(message::encode_query(too_long, record_type::a, 1U, packet, sizeof(packet), size), status::invalid_argument);
}

TEST(DnsMessage, ParsesCompressedAAnswerAndChecksTransaction)
{
    uint8_t packet[512U]{};
    size_type size = make_question(packet);
    size = append_a(packet, size, 34U);
    response<4U> parsed;
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::ok);
    EXPECT_EQ(parsed.record_count, 1U);
    EXPECT_EQ(parsed.answer_count, 1U);
    EXPECT_STREQ(parsed.question_name, "www.example.com");
    EXPECT_STREQ(parsed.records[0U].owner, "www.example.com");
    EXPECT_EQ(parsed.records[0U].type, record_type::a);
    EXPECT_EQ(parsed.records[0U].ttl, 60U);
    EXPECT_EQ(parsed.records[0U].address.address_bytes()[0U], 192U);
    EXPECT_EQ(parsed.records[0U].address.address_bytes()[3U], 34U);
    EXPECT_EQ(message::parse_response(packet, size, 0x9999U, parsed), status::invalid_argument);
}

TEST(DnsMessage, DecodesCompressedCnameNsAndMxRdata)
{
    uint8_t packet[512U]{};
    size_type at = make_question(packet, "www.example.com", record_type::cname);
    // Three records, one in each RR section.
    packet[7U] = 1U; packet[9U] = 1U; packet[11U] = 1U;
    // CNAME www.example.com -> alias.example.com; RDATA uses a pointer to example.com at offset 16.
    packet[at++] = 0xC0U; packet[at++] = 0x0CU;
    packet[at++] = 0U; packet[at++] = 5U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 30U;
    packet[at++] = 0U; packet[at++] = 8U;
    packet[at++] = 5U; packet[at++] = 'a'; packet[at++] = 'l'; packet[at++] = 'i'; packet[at++] = 'a'; packet[at++] = 's';
    packet[at++] = 0xC0U; packet[at++] = 16U;
    // Authority NS, RDATA is a pointer to the question's example.com suffix.
    packet[at++] = 0xC0U; packet[at++] = 16U;
    packet[at++] = 0U; packet[at++] = 2U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 50U;
    packet[at++] = 0U; packet[at++] = 2U; packet[at++] = 0xC0U; packet[at++] = 16U;
    // Additional MX, preference 10 and compressed exchange name.
    packet[at++] = 0xC0U; packet[at++] = 16U;
    packet[at++] = 0U; packet[at++] = 15U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 40U;
    packet[at++] = 0U; packet[at++] = 4U; packet[at++] = 0U; packet[at++] = 10U; packet[at++] = 0xC0U; packet[at++] = 16U;

    response<4U> parsed;
    EXPECT_EQ(message::parse_response(packet, at, 0x1234U, parsed), status::ok);
    ASSERT_EQ(parsed.record_count, 3U);
    EXPECT_EQ(parsed.records[0U].type, record_type::cname);
    EXPECT_STREQ(parsed.records[0U].target, "alias.example.com");
    EXPECT_EQ(parsed.records[0U].source_section, castle::protocols::dns::section::answer);
    EXPECT_EQ(parsed.records[1U].type, record_type::ns);
    EXPECT_STREQ(parsed.records[1U].target, "example.com");
    EXPECT_EQ(parsed.records[1U].source_section, castle::protocols::dns::section::authority);
    EXPECT_EQ(parsed.records[2U].type, record_type::mx);
    EXPECT_EQ(parsed.records[2U].preference, 10U);
    EXPECT_STREQ(parsed.records[2U].target, "example.com");
    EXPECT_EQ(parsed.records[2U].source_section, castle::protocols::dns::section::additional);
}

TEST(DnsMessage, ParsesAaaaAndSkipsUnknownAndNonInternetRecords)
{
    uint8_t packet[512U]{};
    size_type at = make_question(packet, "example.com", record_type::aaaa);
    packet[7U] = 3U;
    // Unknown RR type 65000 with two bytes of RDATA.
    packet[at++] = 0xC0U; packet[at++] = 0x0CU;
    packet[at++] = 0xFDU; packet[at++] = 0xE8U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 2U; packet[at++] = 0xAAU; packet[at++] = 0xBBU;
    // A RR in class CHAOS is ignored.
    packet[at++] = 0xC0U; packet[at++] = 0x0CU;
    packet[at++] = 0U; packet[at++] = 1U; packet[at++] = 0U; packet[at++] = 3U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 4U; packet[at++] = 1U; packet[at++] = 2U; packet[at++] = 3U; packet[at++] = 4U;
    // AAAA record.
    packet[at++] = 0xC0U; packet[at++] = 0x0CU;
    packet[at++] = 0U; packet[at++] = 28U; packet[at++] = 0U; packet[at++] = 1U;
    packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 0U; packet[at++] = 5U;
    packet[at++] = 0U; packet[at++] = 16U;
    for (uint8_t i = 0U; i < 16U; ++i) { packet[at++] = i; }

    response<2U> parsed;
    EXPECT_EQ(message::parse_response(packet, at, 0x1234U, parsed), status::ok);
    ASSERT_EQ(parsed.record_count, 1U);
    EXPECT_EQ(parsed.records[0U].type, record_type::aaaa);
    EXPECT_EQ(parsed.records[0U].address.family(), castle::net::address_family::ipv6);
    EXPECT_EQ(parsed.records[0U].address.address_bytes()[15U], 15U);
}

TEST(DnsMessage, ReportsRecordCapacityAndMalformedLengths)
{
    uint8_t packet[512U]{};
    size_type at = make_question(packet);
    at = append_a(packet, at, 1U);
    at = append_a(packet, at, 2U);
    packet[7U] = 2U;
    response<1U> small;
    EXPECT_EQ(message::parse_response(packet, at, 0x1234U, small), status::full);

    at = make_question(packet);
    at = append_a(packet, at, 1U);
    packet[at - 5U] = 3U; // A RDLENGTH becomes 3
    response<2U> malformed;
    EXPECT_EQ(message::parse_response(packet, at, 0x1234U, malformed), status::invalid_argument);
}

TEST(DnsMessage, RejectsMalformedHeadersNamesPointersAndTrailingBytes)
{
    uint8_t packet[128U]{};
    size_type size = make_question(packet);
    response<4U> parsed;
    EXPECT_EQ(message::parse_response(packet, 11U, 0x1234U, parsed), status::invalid_argument);
    packet[2U] = 0x01U; // QR clear
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::invalid_argument);
    packet[2U] = 0x89U; packet[3U] = 0x80U; // Opcode 1
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::invalid_argument);
    packet[2U] = 0x81U; packet[3U] = 0x80U; packet[5U] = 2U;
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::invalid_argument);
    packet[5U] = 1U;
    packet[2U] = 0xF1U; // reserved Z flags must be zero
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::invalid_argument);
    packet[2U] = 0x81U;
    packet[size] = 0xFFU;
    EXPECT_EQ(message::parse_response(packet, size + 1U, 0x1234U, parsed), status::invalid_argument);

    uint8_t names[] = {0xC0U, 0x00U};
    char name[16U]{};
    size_type consumed = 0U;
    EXPECT_EQ(message::decode_name(names, sizeof(names), 0U, name, sizeof(name), consumed), status::invalid_argument); // self pointer
    uint8_t reserved[] = {0x40U};
    EXPECT_EQ(message::decode_name(reserved, sizeof(reserved), 0U, name, sizeof(name), consumed), status::invalid_argument);
    uint8_t label[] = {3U, 'a'};
    EXPECT_EQ(message::decode_name(label, sizeof(label), 0U, name, sizeof(name), consumed), status::invalid_argument);
    uint8_t long_label[] = {3U, 'a', 'b', 'c', 0U};
    char tiny[2U]{};
    EXPECT_EQ(message::decode_name(long_label, sizeof(long_label), 0U, tiny, sizeof(tiny), consumed), status::full);
    EXPECT_EQ(message::decode_name(nullptr, 0U, 0U, name, sizeof(name), consumed), status::invalid_argument);
    uint8_t tc_header[] = {0U, 0U, 0x02U, 0U};
    EXPECT_TRUE(message::is_truncated_packet(tc_header, sizeof(tc_header)));
    EXPECT_FALSE(message::is_truncated_packet(nullptr, 4U));
    EXPECT_FALSE(message::is_truncated_packet(names, 2U));
}

TEST(DnsMessage, HandlesTruncatedFlagAndResponseCode)
{
    uint8_t packet[128U]{};
    size_type size = make_question(packet);
    packet[2U] = 0x83U;
    EXPECT_TRUE(message::is_truncated_packet(packet, size));
    response<4U> parsed;
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::ok);
    EXPECT_TRUE(parsed.truncated);
    packet[2U] = 0x81U; packet[3U] = 0x85U;
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::ok);
    EXPECT_EQ(parsed.rcode, castle::protocols::dns::response_code::refused);
}

namespace
{
// Writes `labels` labels of `label_size` 'a' octets separated by dots.
castle::container::string_view make_labels(char* buffer, size_type label_size, size_type labels)
{
    size_type at = 0U;
    for (size_type label = 0U; label < labels; ++label)
    {
        if (label != 0U) { buffer[at++] = '.'; }
        for (size_type i = 0U; i < label_size; ++i) { buffer[at++] = 'a'; }
    }
    return castle::container::string_view(buffer, at);
}

// Builds a response header with one question "a" A/IN followed by the given resource-record octets.
size_type make_single_record(uint8_t* packet, uint8_t const* record, size_type record_size)
{
    packet[2U] = 0x81U;
    packet[3U] = 0x80U;
    packet[5U] = 1U;
    packet[7U] = 1U;
    packet[12U] = 1U; packet[13U] = 'a'; packet[14U] = 0U;
    packet[15U] = 0U; packet[16U] = 1U; packet[17U] = 0U; packet[18U] = 1U;
    for (size_type i = 0U; i < record_size; ++i) { packet[19U + i] = record[i]; }
    return 19U + record_size;
}
}

TEST(DnsMessage, RejectsTightCapacityEmptyAndUnsafeNames)
{
    uint8_t packet[512U]{};
    size_type size = 0U;
    EXPECT_EQ(message::encode_query("a", record_type::a, 1U, packet, 11U, size), status::full);
    EXPECT_EQ(size, 0U);
    EXPECT_EQ(message::encode_query(castle::container::string_view(), record_type::a, 1U, packet, sizeof(packet), size),
              status::invalid_argument);
    char const embedded_nul[] = {'a', '\0', 'b'};
    EXPECT_EQ(message::encode_query(castle::container::string_view(embedded_nul, 3U), record_type::a, 1U,
                                    packet, sizeof(packet), size), status::invalid_argument);
    char const high_octet[] = {static_cast<char>(0xC3), static_cast<char>(0xA9), '.', 'c', 'o', 'm'};
    EXPECT_EQ(message::encode_query(castle::container::string_view(high_octet, 6U), record_type::a, 1U,
                                    packet, sizeof(packet), size), status::invalid_argument);
}

TEST(DnsMessage, EncodesSingleLabelTrailingDotRootAndNsMxQueries)
{
    uint8_t packet[512U]{};
    size_type size = 0U;
    EXPECT_EQ(message::encode_query("a", record_type::a, 1U, packet, sizeof(packet), size), status::ok);
    EXPECT_EQ(size, 19U);
    EXPECT_EQ(message::encode_query("example.com.", record_type::a, 1U, packet, sizeof(packet), size), status::ok);
    EXPECT_EQ(size, 29U);
    EXPECT_EQ(message::encode_query("example.com", record_type::ns, 1U, packet, sizeof(packet), size), status::ok);
    EXPECT_EQ(message::encode_query("example.com", record_type::mx, 1U, packet, sizeof(packet), size), status::ok);
    EXPECT_EQ(message::encode_query(".", record_type::a, 1U, packet, 16U, size), status::full);
    EXPECT_EQ(message::encode_query(".", record_type::a, 1U, packet, 17U, size), status::ok);
    EXPECT_EQ(size, 17U);
    EXPECT_EQ(message::encode_query("example..", record_type::a, 1U, packet, sizeof(packet), size),
              status::invalid_argument);
}

TEST(DnsMessage, RejectsOverlongAndOverflowingLabels)
{
    uint8_t packet[512U]{};
    size_type size = 0U;
    char name[320U]{};
    EXPECT_EQ(message::encode_query(make_labels(name, 64U, 2U), record_type::a, 1U, packet, sizeof(packet), size),
              status::invalid_argument);
    EXPECT_EQ(message::encode_query(make_labels(name, 63U, 5U), record_type::a, 1U, packet, sizeof(packet), size),
              status::out_of_range);
    EXPECT_EQ(message::encode_query(make_labels(name, 63U, 4U), record_type::a, 1U, packet, sizeof(packet), size),
              status::invalid_argument);
    EXPECT_EQ(message::encode_query(make_labels(name, 63U, 3U), record_type::a, 1U, packet, sizeof(packet), size),
              status::ok);
}

TEST(DnsMessage, RejectsNullAndOversizedResponses)
{
    uint8_t packet[512U]{};
    response<4U> parsed;
    EXPECT_EQ(message::parse_response(nullptr, 64U, 0x1234U, parsed), status::invalid_argument);
    EXPECT_EQ(message::parse_response(packet, 65536U, 0x1234U, parsed), status::invalid_argument);
}

TEST(DnsMessage, RejectsReservedFlagsAndMalformedQuestions)
{
    uint8_t packet[512U]{};
    size_type size = make_question(packet);
    response<4U> parsed;
    packet[3U] = 0x90U; // response with Z bit 0x10 set
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::invalid_argument);
    packet[3U] = 0xC0U; // response with Z bit 0x40 set
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::invalid_argument);
    packet[3U] = 0x80U;
    EXPECT_EQ(message::parse_response(packet, size, 0x1234U, parsed), status::ok);

    uint8_t bad_question[32U]{};
    bad_question[2U] = 0x81U;
    bad_question[3U] = 0x80U;
    bad_question[5U] = 1U;
    bad_question[12U] = 0x40U; // reserved label-length bits
    EXPECT_EQ(message::parse_response(bad_question, sizeof(bad_question), 0x0000U, parsed), status::invalid_argument);

    uint8_t short_question[17U]{};
    short_question[2U] = 0x81U;
    short_question[3U] = 0x80U;
    short_question[5U] = 1U;
    short_question[12U] = 1U;
    short_question[13U] = 'a';
    short_question[14U] = 0U;
    EXPECT_EQ(message::parse_response(short_question, sizeof(short_question), 0x0000U, parsed), status::invalid_argument);
}

TEST(DnsMessage, RejectsMalformedOwnersAndTruncatedResourceRecords)
{
    uint8_t packet[64U]{};
    response<4U> parsed;
    uint8_t bad_owner[] = {0x40U};
    size_type size = make_single_record(packet, bad_owner, sizeof(bad_owner));
    EXPECT_EQ(message::parse_response(packet, size, 0x0000U, parsed), status::invalid_argument);

    uint8_t owner_only[] = {0xC0U, 0x0CU};
    size = make_single_record(packet, owner_only, sizeof(owner_only));
    EXPECT_EQ(message::parse_response(packet, size, 0x0000U, parsed), status::invalid_argument);

    uint8_t rdlength_overflow[] = {0xC0U, 0x0CU, 0U, 1U, 0U, 1U, 0U, 0U, 0U, 45U, 0U, 100U};
    size = make_single_record(packet, rdlength_overflow, sizeof(rdlength_overflow));
    EXPECT_EQ(message::parse_response(packet, size, 0x0000U, parsed), status::invalid_argument);
}

TEST(DnsMessage, RejectsWrongRdataLengthsAndMalformedRdataNames)
{
    uint8_t packet[128U]{};
    response<4U> parsed;

    uint8_t aaaa_bad[12U + 15U] = {0xC0U, 0x0CU, 0U, 28U, 0U, 1U, 0U, 0U, 0U, 5U, 0U, 15U};
    EXPECT_EQ(message::parse_response(packet, make_single_record(packet, aaaa_bad, sizeof(aaaa_bad)), 0x0000U, parsed),
              status::invalid_argument);

    uint8_t mx_short[12U + 2U] = {0xC0U, 0x0CU, 0U, 15U, 0U, 1U, 0U, 0U, 0U, 5U, 0U, 2U, 0U, 10U};
    EXPECT_EQ(message::parse_response(packet, make_single_record(packet, mx_short, sizeof(mx_short)), 0x0000U, parsed),
              status::invalid_argument);

    uint8_t mx_bad_name[12U + 3U] = {0xC0U, 0x0CU, 0U, 15U, 0U, 1U, 0U, 0U, 0U, 5U, 0U, 3U, 0U, 10U, 0x40U};
    EXPECT_EQ(message::parse_response(packet, make_single_record(packet, mx_bad_name, sizeof(mx_bad_name)), 0x0000U, parsed),
              status::invalid_argument);

    uint8_t mx_trailing[12U + 5U] = {0xC0U, 0x0CU, 0U, 15U, 0U, 1U, 0U, 0U, 0U, 5U, 0U, 5U, 0U, 10U, 0U, 0xAAU, 0xBBU};
    EXPECT_EQ(message::parse_response(packet, make_single_record(packet, mx_trailing, sizeof(mx_trailing)), 0x0000U, parsed),
              status::invalid_argument);

    uint8_t ns_bad_name[12U + 1U] = {0xC0U, 0x0CU, 0U, 2U, 0U, 1U, 0U, 0U, 0U, 5U, 0U, 1U, 0x40U};
    EXPECT_EQ(message::parse_response(packet, make_single_record(packet, ns_bad_name, sizeof(ns_bad_name)), 0x0000U, parsed),
              status::invalid_argument);

    uint8_t ns_trailing[12U + 2U] = {0xC0U, 0x0CU, 0U, 2U, 0U, 1U, 0U, 0U, 0U, 5U, 0U, 2U, 0U, 0U};
    EXPECT_EQ(message::parse_response(packet, make_single_record(packet, ns_trailing, sizeof(ns_trailing)), 0x0000U, parsed),
              status::invalid_argument);
}

TEST(DnsMessage, DecodeNameRejectsBadBuffersPointersAndLimits)
{
    uint8_t data[8U]{};
    char name[512U]{};
    size_type consumed = 0U;
    EXPECT_EQ(message::decode_name(data, 3U, 3U, name, sizeof(name), consumed), status::invalid_argument);
    EXPECT_EQ(message::decode_name(data, 3U, 0U, nullptr, sizeof(name), consumed), status::invalid_argument);
    EXPECT_EQ(message::decode_name(data, 3U, 0U, name, 0U, consumed), status::invalid_argument);

    uint8_t unterminated[] = {0x01U, 'a'};
    EXPECT_EQ(message::decode_name(unterminated, sizeof(unterminated), 0U, name, sizeof(name), consumed),
              status::invalid_argument);

    uint8_t truncated_pointer[] = {0xC0U};
    EXPECT_EQ(message::decode_name(truncated_pointer, sizeof(truncated_pointer), 0U, name, sizeof(name), consumed),
              status::invalid_argument);
    uint8_t out_of_range_pointer[] = {0xC0U, 0x7FU};
    EXPECT_EQ(message::decode_name(out_of_range_pointer, sizeof(out_of_range_pointer), 0U, name, sizeof(name), consumed),
              status::invalid_argument);

    // Offset 5 -> pointer to 3 -> pointer to 0 -> "a"; only the first pointer consumes stream octets.
    uint8_t pointer_chain[] = {0x01U, 'a', 0x00U, 0xC0U, 0x00U, 0xC0U, 0x03U};
    EXPECT_EQ(message::decode_name(pointer_chain, sizeof(pointer_chain), 5U, name, sizeof(name), consumed), status::ok);
    EXPECT_STREQ(name, "a");
    EXPECT_EQ(consumed, 2U);

    // A chain of 129 pointers exceeds the hop limit before reaching the root label.
    uint8_t hops[258U]{};
    for (size_type k = 1U; k <= 128U; ++k)
    {
        hops[2U * k] = 0xC0U;
        hops[2U * k + 1U] = static_cast<uint8_t>(2U * (k - 1U));
    }
    EXPECT_EQ(message::decode_name(hops, sizeof(hops), 256U, name, sizeof(name), consumed), status::invalid_argument);

    // Four labels expand to exactly 255 octets, so the following length octet is rejected.
    uint8_t expanded[300U]{};
    size_type at = 0U;
    for (size_type label = 0U; label < 3U; ++label)
    {
        expanded[at++] = 63U;
        for (size_type i = 0U; i < 63U; ++i) { expanded[at++] = 'a'; }
    }
    expanded[at++] = 62U;
    for (size_type i = 0U; i < 62U; ++i) { expanded[at++] = 'a'; }
    expanded[at++] = 1U;
    expanded[at++] = 'z';
    expanded[at++] = 0U;
    EXPECT_EQ(message::decode_name(expanded, at, 0U, name, sizeof(name), consumed), status::invalid_argument);

    // 128 one-octet labels would expand past 255 octets.
    uint8_t singles[257U]{};
    for (size_type i = 0U; i < 128U; ++i)
    {
        singles[2U * i] = 1U;
        singles[2U * i + 1U] = 'a';
    }
    EXPECT_EQ(message::decode_name(singles, sizeof(singles), 0U, name, sizeof(name), consumed), status::invalid_argument);

    uint8_t two_labels[] = {1U, 'a', 1U, 'b', 0U};
    char tiny[2U]{};
    EXPECT_EQ(message::decode_name(two_labels, sizeof(two_labels), 0U, tiny, sizeof(tiny), consumed), status::full);

    uint8_t nul_octet[] = {1U, 0U, 0U};
    EXPECT_EQ(message::decode_name(nul_octet, sizeof(nul_octet), 0U, name, sizeof(name), consumed), status::invalid_argument);
    uint8_t high_octet[] = {1U, 0x80U, 0U};
    EXPECT_EQ(message::decode_name(high_octet, sizeof(high_octet), 0U, name, sizeof(name), consumed), status::invalid_argument);
}

