#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/messages/gbs.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::messages;

    // No generator exists for GBS; the checksum suffix is simply omitted
    // rather than hand-computing one (decode_sentence() treats '*HH' as optional).
    CASTLE_CONST castle::container::string_view literal(
        "$GPGBS,235959.00,1.4,1.1,2.9,5,0.50,0.20,0.30\r\n");

    castle::container::string_view field_storage[16U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(literal, field_storage, 16U, raw));
    CASTLE_SAMPLE_CHECK(!raw.checksum_present);

    CASTLE_SAMPLE_CHECK(gbs::matches(raw));

    gbs fault;
    CASTLE_SAMPLE_CHECK(gbs::decode(raw, fault));
    CASTLE_SAMPLE_CHECK(fault.valid);
    CASTLE_SAMPLE_CHECK((fault.err_lat > 1.3) && (fault.err_lat < 1.5));
    CASTLE_SAMPLE_CHECK((fault.err_lon > 1.0) && (fault.err_lon < 1.2));
    CASTLE_SAMPLE_CHECK((fault.err_alt > 2.8) && (fault.err_alt < 3.0));
    CASTLE_SAMPLE_CHECK(fault.failed_sv_id == 5);
    CASTLE_SAMPLE_CHECK((fault.probability > 0.49) && (fault.probability < 0.51));
    CASTLE_SAMPLE_CHECK((fault.bias > 0.19) && (fault.bias < 0.21));
    CASTLE_SAMPLE_CHECK((fault.std_dev > 0.29) && (fault.std_dev < 0.31));

    return 0;
}
