#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_rxr.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_rxr;

    // The receiver reports that it left backup mode.
    mon_rxr status{};
    status.flags = mon_rxr::FLAGS_AWAKE;

    uint8_t frame[16U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_rxr::payload_length));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_rxr::matches(raw));

    mon_rxr decoded{};
    CASTLE_SAMPLE_CHECK(mon_rxr::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.awake());

    // Entering backup mode clears the awake bit.
    decoded.flags = 0U;
    CASTLE_SAMPLE_CHECK(!decoded.awake());

    // Too small a buffer is reported, never overrun.
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, UBX_FRAME_OVERHEAD, written) == castle::status::full);
    return 0;
}
