#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"

int main()
{
    using namespace castle::protocols::nmea;

    // Encode a GSA sentence from a talker, a type, and an ordered field list.
    castle::container::string_view fields_data[2U] = {
        castle::container::string_view("1"),
        castle::container::string_view("08")
    };
    castle::container::array_view<const castle::container::string_view> fields(fields_data, 2U);

    char sentence[96U];
    castle::size_type written = 0U;
    castle::status encode_status = encode_sentence(
        castle::container::string_view(TALKER_GP),
        castle::container::string_view(SENTENCE_GSA),
        fields, sentence, sizeof(sentence), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode it back into a zero-copy view over the same buffer.
    castle::container::string_view field_storage[8U];
    message_view view;
    bool decoded = decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 8U, view);
    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(view.is_talker(TALKER_GP));
    CASTLE_SAMPLE_CHECK(view.is(SENTENCE_GSA));
    CASTLE_SAMPLE_CHECK(view.field_count() == 2U);

    // Decode a real-world GGA sentence and convert its lat/lon fields.
    castle::container::string_view gga_fields[16U];
    message_view gga_view;
    bool gga_decoded = decode_sentence(
        castle::container::string_view(
            "$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*5B\r\n"),
        gga_fields, 16U, gga_view);
    CASTLE_SAMPLE_CHECK(gga_decoded);
    CASTLE_SAMPLE_CHECK(gga_view.field_count() == 14U);

    double latitude = 0.0;
    bool latitude_ok = parse_latlon(gga_view.field(1U), gga_view.field(2U), latitude);
    CASTLE_SAMPLE_CHECK(latitude_ok);
    CASTLE_SAMPLE_CHECK((latitude > 47.28) && (latitude < 47.29));

    // A tampered checksum is rejected rather than silently accepted.
    castle::container::string_view rejected_fields[16U];
    message_view rejected_view;
    CASTLE_SAMPLE_CHECK(!decode_sentence(
        castle::container::string_view(
            "$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*00\r\n"),
        rejected_fields, 16U, rejected_view));

    return 0;
}
