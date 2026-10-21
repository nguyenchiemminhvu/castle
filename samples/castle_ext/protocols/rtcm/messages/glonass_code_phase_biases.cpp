#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp"

int main()
{
    using namespace castle::protocols::rtcm;
    using namespace castle::protocols::rtcm::messages;

    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::accepts(RTCM_TYPE_GLO_BIASES));
    CASTLE_SAMPLE_CHECK(!glonass_code_phase_biases::accepts(RTCM_TYPE_STATION_ARP));
    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::MASK_L1_CA == 0x08U);
    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::MASK_L1_P == 0x04U);
    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::MASK_L2_CA == 0x02U);
    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::MASK_L2_P == 0x01U);
    CASTLE_SAMPLE_CHECK(glonass_code_phase_biases::RESOLUTION_M == 0.02);

    // Biases are 16-bit signed values in 0.02 m; only those selected by signals_mask are sent.
    glonass_code_phase_biases biases{};
    biases.station_id = 1234U;
    biases.code_phase_bias_indicator = true;
    biases.signals_mask = glonass_code_phase_biases::MASK_L1_CA | glonass_code_phase_biases::MASK_L1_P
                        | glonass_code_phase_biases::MASK_L2_P;
    biases.l1_ca_bias = -100;
    biases.l1_p_bias = 200;
    biases.l2_ca_bias = 1234; // not selected, never transmitted
    biases.l2_p_bias = -32768;

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(biases, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(10U)); // 4 header bytes + 3 biases of 2 bytes
    CASTLE_SAMPLE_CHECK(frame[3U] == 0x4CU && frame[4U] == 0xE4U); // 1230 = 0x4CE, station id 1234
    CASTLE_SAMPLE_CHECK(frame[6U] == 0x8DU);                       // indicator, 3 reserved zeros, mask 0b1101
    CASTLE_SAMPLE_CHECK(frame[7U] == 0xFFU && frame[8U] == 0x9CU); // -100

    // An empty mask carries no bias at all.
    biases.signals_mask = 0U;
    result = encode_message(biases, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(4U));

    // The mask is four bits wide.
    biases.signals_mask = 16U;
    result = encode_message(biases, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(written == 0U);

    // Exact-fit and one-byte-short buffers.
    biases.signals_mask = 15U;
    result = encode_message(biases, frame, frame_size(12U), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::ok);
    result = encode_message(biases, frame, frame_size(12U) - 1U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);

    return 0;
}
