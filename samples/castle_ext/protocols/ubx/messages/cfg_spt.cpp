#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/cfg_spt.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Start the production self-test of sensor 3 (Bosch BMI160, see UBX-MON-SPT).
    cfg_spt test{};
    test.sensor_id = 3U;

    uint8_t frame[UBX_FRAME_OVERHEAD + cfg_spt::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(test, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));
    CASTLE_SAMPLE_CHECK(frame[UBX_FRAME_INDEX_PAYLOAD] == cfg_spt::version);

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_spt::matches(raw));

    cfg_spt decoded{};
    CASTLE_SAMPLE_CHECK(cfg_spt::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.sensor_id == 3U);

    // A version this spec does not define is rejected.
    uint8_t payload[cfg_spt::payload_length] = {0x01U, 0U, 3U, 0U};
    message_view future{{UBX_CLASS_CFG, UBX_ID_CFG_SPT, cfg_spt::payload_length},
                        castle::container::array_view<const uint8_t>(payload, cfg_spt::payload_length), checksum{}};
    CASTLE_SAMPLE_CHECK(!cfg_spt::decode(future, decoded));
    CASTLE_SAMPLE_CHECK(decoded.sensor_id == 0U);

    return 0;
}
