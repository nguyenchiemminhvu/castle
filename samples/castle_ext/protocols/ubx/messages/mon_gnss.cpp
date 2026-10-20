#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_gnss.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_gnss;

    // MON-GNSS is polled: send an empty-payload request.
    uint8_t poll[UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_poll<mon_gnss>(poll, sizeof(poll), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);

    // Simulated answer: all four GNSS supported, GPS+Galileo+BeiDou enabled.
    mon_gnss answer{};
    answer.supported = static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_GLONASS |
                                            mon_gnss::GNSS_BEIDOU | mon_gnss::GNSS_GALILEO);
    answer.default_gnss = static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_GLONASS);
    answer.enabled = static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_BEIDOU | mon_gnss::GNSS_GALILEO);
    answer.simultaneous = 4U;

    uint8_t frame[32U] = {};
    CASTLE_SAMPLE_CHECK(encode_message(answer, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_gnss::matches(raw));

    mon_gnss decoded{};
    CASTLE_SAMPLE_CHECK(mon_gnss::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0x01U);
    CASTLE_SAMPLE_CHECK(decoded.simultaneous == 4U);
    CASTLE_SAMPLE_CHECK(decoded.is_supported(mon_gnss::GNSS_GLONASS));
    CASTLE_SAMPLE_CHECK(decoded.is_default(static_cast<uint8_t>(mon_gnss::GNSS_GPS | mon_gnss::GNSS_GLONASS)));
    CASTLE_SAMPLE_CHECK(decoded.is_enabled(mon_gnss::GNSS_GALILEO));
    CASTLE_SAMPLE_CHECK(!decoded.is_enabled(mon_gnss::GNSS_GLONASS));

    CASTLE_SAMPLE_CHECK(encode_message(answer, frame, frame_size(7U), written) == castle::status::full);
    return 0;
}
