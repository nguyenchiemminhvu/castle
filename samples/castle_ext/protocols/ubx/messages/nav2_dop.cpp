#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // UBX-NAV2-DOP payload (18 bytes).
    //
    // iTOW = 1000
    // gDOP = 110
    // pDOP = 120
    // tDOP = 130
    // vDOP = 140
    // hDOP = 150
    // nDOP = 160
    // eDOP = 170
    uint8_t payload_data[18U] =
    {
        0xE8U, 0x03U, 0x00U, 0x00U, // iTOW = 1000

        0x6EU, 0x00U, // gDOP = 110
        0x78U, 0x00U, // pDOP = 120
        0x82U, 0x00U, // tDOP = 130
        0x8CU, 0x00U, // vDOP = 140
        0x96U, 0x00U, // hDOP = 150
        0xA0U, 0x00U, // nDOP = 160
        0xAAU, 0x00U  // eDOP = 170
    };

    message_view raw;
    raw.header.msg_class = UBX_CLASS_NAV2;
    raw.header.msg_id = UBX_ID_NAV2_DOP;
    raw.header.payload_length = sizeof(payload_data);
    raw.payload =
        castle::container::array_view<const uint8_t>(
            payload_data,
            sizeof(payload_data));

    CASTLE_SAMPLE_CHECK(nav2_dop::matches(raw));

    nav2_dop message;
    bool decoded = nav2_dop::decode(raw, message);

    CASTLE_SAMPLE_CHECK(decoded);

    CASTLE_SAMPLE_CHECK(message.i_tow == 1000U);
    CASTLE_SAMPLE_CHECK(message.g_dop == 110U);
    CASTLE_SAMPLE_CHECK(message.p_dop == 120U);
    CASTLE_SAMPLE_CHECK(message.t_dop == 130U);
    CASTLE_SAMPLE_CHECK(message.v_dop == 140U);
    CASTLE_SAMPLE_CHECK(message.h_dop == 150U);
    CASTLE_SAMPLE_CHECK(message.n_dop == 160U);
    CASTLE_SAMPLE_CHECK(message.e_dop == 170U);

    // Wrong payload length must be rejected.
    message_view invalid = raw;
    invalid.header.payload_length = 17U;

    nav2_dop rejected;
    CASTLE_SAMPLE_CHECK(!nav2_dop::decode(invalid, rejected));

    return 0;
}
