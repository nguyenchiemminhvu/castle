#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_geofence.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;
    using castle::container::array_view;

    // Receiver output with two active fences: inside fence 1, outside fence 2.
    nav_geofence<4U> status{};
    status.i_tow = 345600000U;
    status.status = nav_geofence<4U>::STATUS_ACTIVE;
    status.comb_state = nav_geofence_state::outside;

    nav_geofence_fence home{};
    home.state = nav_geofence_state::inside;
    home.id = 1U;
    nav_geofence_fence depot{};
    depot.state = nav_geofence_state::outside;
    depot.id = 2U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(status.fences.push_back(home)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(status.fences.push_back(depot)));

    // Encoding writes numFences from fences.size().
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(status, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == frame_size(nav_geofence<4U>::header_length + (2U * nav_geofence<4U>::block_length)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_geofence<4U>::matches(raw));

    nav_geofence<4U> decoded{};
    CASTLE_SAMPLE_CHECK(nav_geofence<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.status == nav_geofence<4U>::STATUS_ACTIVE);
    CASTLE_SAMPLE_CHECK(decoded.num_fences == 2U);
    CASTLE_SAMPLE_CHECK(decoded.comb_state == nav_geofence_state::outside);
    CASTLE_SAMPLE_CHECK(decoded.fences.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.fences[0U].state == nav_geofence_state::inside);
    CASTLE_SAMPLE_CHECK(decoded.fences[1U].id == 2U);

    // A smaller model keeps the wire count but stores only what fits.
    nav_geofence<1U> compact{};
    CASTLE_SAMPLE_CHECK(nav_geofence<1U>::decode(raw, compact));
    CASTLE_SAMPLE_CHECK(compact.num_fences == 2U);
    CASTLE_SAMPLE_CHECK(compact.fences.size() == 1U);

    // Poll request: empty payload.
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_poll<nav_geofence<>>(frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);

    return 0;
}
