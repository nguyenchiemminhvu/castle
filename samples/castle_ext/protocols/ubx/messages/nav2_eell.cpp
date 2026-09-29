#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // UBX-NAV2-EELL payload (16 bytes).
    //
    // iTOW      = 1000
    // version   = 1
    // errMaj    = 250
    // errMin    = 150
    // errOrient = 45
    uint8_t payload_data[16U] =
    {
        0xE8U, 0x03U, 0x00U, 0x00U, // iTOW = 1000

        0x01U, // version
        0x00U, 0x00U, 0x00U, // reserved

        0xFAU, 0x00U, // errMaj = 250
        0x96U, 0x00U, // errMin = 150
        0x2DU, 0x00U, // errOrient = 45

        0x00U, 0x00U // reserved
    };

    message_view raw;
    raw.header.msg_class = UBX_CLASS_NAV2;
    raw.header.msg_id = UBX_ID_NAV2_EELL;
    raw.header.payload_length = sizeof(payload_data);
    raw.payload =
        castle::container::array_view<const uint8_t>(
            payload_data,
            sizeof(payload_data));

    CASTLE_SAMPLE_CHECK(nav2_eell::matches(raw));

    nav2_eell message;
    bool decoded = nav2_eell::decode(raw, message);

    CASTLE_SAMPLE_CHECK(decoded);

    CASTLE_SAMPLE_CHECK(message.i_tow == 1000U);
    CASTLE_SAMPLE_CHECK(message.version == 1U);
    CASTLE_SAMPLE_CHECK(message.err_maj == 250U);
    CASTLE_SAMPLE_CHECK(message.err_min == 150U);
    CASTLE_SAMPLE_CHECK(message.err_orient == 45U);

    // Wrong payload length must be rejected.
    message_view invalid = raw;
    invalid.header.payload_length = 15U;

    nav2_eell rejected;
    CASTLE_SAMPLE_CHECK(!nav2_eell::decode(invalid, rejected));

    return 0;
}
