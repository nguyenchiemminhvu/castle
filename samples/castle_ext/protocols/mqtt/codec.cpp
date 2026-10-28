#include "sample_support.hpp"
#include "castle_ext/protocols/mqtt/codec.hpp"

using namespace castle::protocols::mqtt;

int main()
{
    const uint32_t values[] = {0U, 127U, 128U, 16383U, 16384U, 2097151U, 2097152U, 268435455U};
    for (castle::size_type i = 0U; i < sizeof(values) / sizeof(values[0U]); ++i)
    {
        uint8_t encoded[4U]{}; castle::size_type used = 0U; uint32_t decoded = 0U; castle::size_type consumed = 0U;
        CASTLE_SAMPLE_CHECK(encode_remaining_length(values[i], encoded, used) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(decode_remaining_length(encoded, used, decoded, consumed) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(decoded == values[i] && consumed == used);
    }
    uint8_t partial[] = {0x80U}; uint32_t decoded = 0U; castle::size_type consumed = 0U;
    CASTLE_SAMPLE_CHECK(decode_remaining_length(partial, sizeof(partial), decoded, consumed) == castle::status::empty);
    uint8_t malformed[] = {0x80U, 0x80U, 0x80U, 0x80U};
    CASTLE_SAMPLE_CHECK(decode_remaining_length(malformed, sizeof(malformed), decoded, consumed) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(!valid_fixed_header(packet_type::pubrel, 0U));
    CASTLE_SAMPLE_CHECK(valid_fixed_header(packet_type::pubrel, 0x02U));
    CASTLE_SAMPLE_CHECK(!valid_fixed_header(packet_type::publish, 0x06U));

    uint8_t body[] = {0x01U, 0x02U, 0x03U}; uint8_t packet_bytes[16U]{}; castle::size_type packet_size = 0U;
    CASTLE_SAMPLE_CHECK(encode_packet(packet_type::pingreq, 0U, nullptr, 0U, packet_bytes, sizeof(packet_bytes), packet_size) == castle::status::ok);
    packet_view packet{};
    CASTLE_SAMPLE_CHECK(inspect_packet(packet_bytes, packet_size, sizeof(packet_bytes), packet) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(packet.type == packet_type::pingreq && packet.remaining_length == 0U);
    CASTLE_SAMPLE_CHECK(encode_packet(packet_type::publish, 0U, body, sizeof(body), packet_bytes, sizeof(packet_bytes), packet_size) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(inspect_packet(packet_bytes, packet_size - 1U, sizeof(packet_bytes), packet) == castle::status::empty);

    property_list<3U, 24U, 8U> properties;
    CASTLE_SAMPLE_CHECK(properties.add_integer(property_id::session_expiry_interval, 120U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(properties.add_string(property_id::reason_string, "ok") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(properties.add_user_property("key", "value") == castle::status::ok);
    uint8_t encoded_properties[128U]{}; castle::size_type properties_size = 0U;
    CASTLE_SAMPLE_CHECK(encode_properties(properties, encoded_properties, sizeof(encoded_properties), properties_size) == castle::status::ok);
    packet_reader reader(encoded_properties, properties_size);
    property_list<3U, 24U, 8U> round_trip;
    CASTLE_SAMPLE_CHECK(decode_properties(reader, round_trip) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(round_trip.count == 3U && round_trip.find(property_id::session_expiry_interval)->integer == 120U);
    return 0;
}
