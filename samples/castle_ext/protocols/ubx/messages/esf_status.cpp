#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_status.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    esf_status<4U> status{};
    status.i_tow = 100000U;
    status.version = 0x02U;
    status.fusion_mode = esf_fusion_mode::fusion;

    // Gyroscope z (type 5): used, calibrated, time tag provided, 100 Hz, no faults.
    esf_status_sensor gyro{};
    gyro.sens_status1 = static_cast<uint8_t>(5U | esf_status_sensor::SENS_STATUS1_USED);
    gyro.sens_status2 = static_cast<uint8_t>(static_cast<uint8_t>(esf_calib_status::calibrated)
                                             | (static_cast<uint8_t>(esf_time_status::time_tag) << 2U));
    gyro.freq = 100U;
    // Speed (type 11): ready only, calibrating, noisy measurements.
    esf_status_sensor speed{};
    speed.sens_status1 = static_cast<uint8_t>(11U | esf_status_sensor::SENS_STATUS1_READY);
    speed.sens_status2 = static_cast<uint8_t>(esf_calib_status::calibrating);
    speed.freq = 10U;
    speed.faults = esf_status_sensor::FAULTS_NOISY_MEAS;
    CASTLE_SAMPLE_CHECK(status.sensors.push_back(gyro) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(status.sensors.push_back(speed) == castle::status::ok);

    // num_sens is written from sensors.size().
    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(esf_status<4U>::matches(raw));

    esf_status<4U> decoded;
    CASTLE_SAMPLE_CHECK(esf_status<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 100000U);
    CASTLE_SAMPLE_CHECK(decoded.version == 0x02U);
    CASTLE_SAMPLE_CHECK(decoded.fusion_mode == esf_fusion_mode::fusion);
    CASTLE_SAMPLE_CHECK(decoded.num_sens == 2U);
    CASTLE_SAMPLE_CHECK(decoded.sensors.size() == 2U);

    CASTLE_CONST esf_status_sensor& first = decoded.sensors[0U];
    CASTLE_SAMPLE_CHECK(first.type() == 5U);
    CASTLE_SAMPLE_CHECK(first.used());
    CASTLE_SAMPLE_CHECK(!first.ready());
    CASTLE_SAMPLE_CHECK(first.calibrated());
    CASTLE_SAMPLE_CHECK(first.calib_status() == esf_calib_status::calibrated);
    CASTLE_SAMPLE_CHECK(first.time_status() == esf_time_status::time_tag);
    CASTLE_SAMPLE_CHECK(first.freq == 100U);
    CASTLE_SAMPLE_CHECK(!first.bad_meas() && !first.bad_ttag() && !first.missing_meas() && !first.noisy_meas());

    CASTLE_CONST esf_status_sensor& second = decoded.sensors[1U];
    CASTLE_SAMPLE_CHECK(second.type() == 11U);
    CASTLE_SAMPLE_CHECK(!second.used());
    CASTLE_SAMPLE_CHECK(second.ready());
    CASTLE_SAMPLE_CHECK(!second.calibrated());
    CASTLE_SAMPLE_CHECK(second.calib_status() == esf_calib_status::calibrating);
    CASTLE_SAMPLE_CHECK(second.time_status() == esf_time_status::no_data);
    CASTLE_SAMPLE_CHECK(second.noisy_meas());

    return 0;
}
