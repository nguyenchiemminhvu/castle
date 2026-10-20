#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Host -> receiver: feed one speed measurement (data type 11) and one
    // rear-left wheel tick count (data type 8) for the fusion filter.
    esf_meas<4U> input{};
    input.time_tag = 1000U;
    input.id = 7U;
    esf_meas_datum speed{};
    speed.data_type = 11U;
    speed.data_value = -1234; // signed 24-bit value (e.g. reversing)
    esf_meas_datum ticks{};
    ticks.data_type = 8U;
    ticks.data_value = 0x7FFFFF; // largest positive dataField
    CASTLE_SAMPLE_CHECK(input.data.push_back(speed) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(input.data.push_back(ticks) == castle::status::ok);

    // numMeas in flags bits 15..11 is written from data.size(); no calibTtag.
    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(input, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(esf_meas<4U>::header_length + (2U * esf_meas<4U>::block_length)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(esf_meas<4U>::matches(raw));

    esf_meas<4U> decoded;
    CASTLE_SAMPLE_CHECK(esf_meas<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.time_tag == 1000U);
    CASTLE_SAMPLE_CHECK(decoded.id == 7U);
    CASTLE_SAMPLE_CHECK(decoded.num_meas() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.data.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.data[0U].data_type == 11U);
    CASTLE_SAMPLE_CHECK(decoded.data[0U].data_value == -1234);
    CASTLE_SAMPLE_CHECK(decoded.data[1U].data_type == 8U);
    CASTLE_SAMPLE_CHECK(decoded.data[1U].data_value == 0x7FFFFF);
    CASTLE_SAMPLE_CHECK(!decoded.calib_ttag_valid());
    CASTLE_SAMPLE_CHECK(decoded.time_mark_sent() == esf_meas<4U>::TIME_MARK_NONE);
    CASTLE_SAMPLE_CHECK(!decoded.time_mark_edge());

    // Receiver output with a time mark on EXT1 (falling edge) and a calibrated time tag.
    esf_meas<4U> output{};
    output.time_tag = 2000U;
    output.flags = static_cast<uint16_t>(esf_meas<4U>::TIME_MARK_EXT1
                                         | esf_meas<4U>::FLAGS_TIME_MARK_EDGE
                                         | esf_meas<4U>::FLAGS_CALIB_TTAG_VALID);
    output.calib_ttag = 9999U;
    CASTLE_SAMPLE_CHECK(output.data.push_back(speed) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(encode_message(output, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == 16U);
    CASTLE_SAMPLE_CHECK(esf_meas<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.time_mark_sent() == esf_meas<4U>::TIME_MARK_EXT1);
    CASTLE_SAMPLE_CHECK(decoded.time_mark_edge());
    CASTLE_SAMPLE_CHECK(decoded.calib_ttag_valid());
    CASTLE_SAMPLE_CHECK(decoded.calib_ttag == 9999U);

    // A dataField outside the signed 24-bit range is refused, never truncated.
    output.data[0U].data_value = 0x00800000;
    CASTLE_SAMPLE_CHECK(encode_message(output, frame, sizeof(frame), written) == castle::status::out_of_range);

    return 0;
}
