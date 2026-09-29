#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/messages/dtm.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::messages;

    // No generator exists for DTM; the checksum suffix is simply omitted.
    CASTLE_CONST castle::container::string_view literal(
        "$GPDTM,W84,,0.0,N,0.0,E,0.0,W84\r\n");

    castle::container::string_view field_storage[8U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(literal, field_storage, 8U, raw));

    CASTLE_SAMPLE_CHECK(dtm::matches(raw));

    dtm datum;
    CASTLE_SAMPLE_CHECK(dtm::decode(raw, datum));
    CASTLE_SAMPLE_CHECK(datum.valid);
    CASTLE_SAMPLE_CHECK(datum.datum_code == castle::container::string_view("W84"));
    CASTLE_SAMPLE_CHECK(datum.datum_sub.empty());
    CASTLE_SAMPLE_CHECK(datum.ref_datum == castle::container::string_view("W84"));

    return 0;
}
