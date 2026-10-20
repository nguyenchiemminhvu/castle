#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_raw.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // One gyroscope z sample (type 5) and one temperature sample (type 12).
    esf_raw<4U> raw_data{};
    esf_raw_sample gyro{};
    gyro.data_type = 5U;
    gyro.data_value = -4096;
    gyro.s_ttag = 0x00ABCDEFU;
    esf_raw_sample temperature{};
    temperature.data_type = 12U;
    temperature.data_value = 2500;
    temperature.s_ttag = 0x00ABCDF0U;
    CASTLE_SAMPLE_CHECK(raw_data.samples.push_back(gyro) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(raw_data.samples.push_back(temperature) == castle::status::ok);

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(raw_data, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(esf_raw<4U>::header_length + (2U * esf_raw<4U>::block_length)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(esf_raw<4U>::matches(raw));

    // The sample count is implied by the payload length (4 + N*8).
    esf_raw<4U> decoded;
    CASTLE_SAMPLE_CHECK(esf_raw<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.num_samples == 2U);
    CASTLE_SAMPLE_CHECK(decoded.samples.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.samples[0U].data_type == 5U);
    CASTLE_SAMPLE_CHECK(decoded.samples[0U].data_value == -4096);
    CASTLE_SAMPLE_CHECK(decoded.samples[0U].s_ttag == 0x00ABCDEFU);
    CASTLE_SAMPLE_CHECK(decoded.samples[1U].data_type == 12U);
    CASTLE_SAMPLE_CHECK(decoded.samples[1U].data_value == 2500);
    CASTLE_SAMPLE_CHECK(decoded.samples[1U].s_ttag == 0x00ABCDF0U);

    // A smaller store keeps the first sample and still reports the wire count.
    esf_raw<1U> clamped;
    CASTLE_SAMPLE_CHECK(esf_raw<1U>::decode(raw, clamped));
    CASTLE_SAMPLE_CHECK(clamped.num_samples == 2U);
    CASTLE_SAMPLE_CHECK(clamped.samples.size() == 1U);

    return 0;
}
