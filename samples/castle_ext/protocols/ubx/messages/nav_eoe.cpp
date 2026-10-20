#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_eoe.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // End-of-epoch marker for the epoch at iTOW 345600000 ms.
    nav_eoe marker{};
    marker.i_tow = 345600000U;

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_eoe::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(marker, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_eoe::matches(raw));

    nav_eoe decoded{};
    CASTLE_SAMPLE_CHECK(nav_eoe::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == marker.i_tow);

    // A host collecting an epoch closes it when the marker's iTOW matches.
    const uint32_t epoch_i_tow = 345600000U;
    CASTLE_SAMPLE_CHECK(decoded.i_tow == epoch_i_tow);

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_eoe::matches(other));

    return 0;
}
