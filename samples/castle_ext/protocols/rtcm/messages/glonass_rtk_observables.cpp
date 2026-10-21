#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp"
#include "castle_ext/checksums/crc24q.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

/**
 * @brief Sends the satellites of `source` as `Message` and checks the frame size.
 *
 * Every alias has the capacity 31, so their satellite vectors are one type and
 * copy as a whole. The frame is a 61 bit header plus `bits_per_satellite` per satellite.
 */
template <typename Message, typename Source>
void check_layout(const Source& source, castle::size_type bits_per_satellite)
{
    Message message;
    message.satellites = source.satellites;

    uint8_t frame[RTCM_MAX_FRAME_SIZE] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(message, frame, sizeof(frame), written)));
    const castle::size_type bits = 61U + (message.satellites.size() * bits_per_satellite);
    CASTLE_SAMPLE_CHECK(written == frame_size((bits + 7U) / 8U));
    // DF002 is the alias' message number.
    CASTLE_SAMPLE_CHECK(((frame[RTCM_FRAME_INDEX_PAYLOAD] << 4U) | (frame[RTCM_FRAME_INDEX_PAYLOAD + 1U] >> 4U))
                        == Message::MESSAGE_TYPE);
}

} // namespace

int main()
{
    // 1. One type per message number (compile-time constants).
    static_assert(glonass_rtk_l1_obs::MESSAGE_TYPE == 1009U, "1009 is the GLONASS L1 observables message");
    static_assert(glonass_rtk_l1_obs_ext::MESSAGE_TYPE == RTCM_TYPE_GLO_L1_OBS_EXT, "1010");
    static_assert(glonass_rtk_l1_l2_obs::MESSAGE_TYPE == RTCM_TYPE_GLO_L1L2_OBS, "1011");
    static_assert(glonass_rtk_l1_l2_obs_ext::MESSAGE_TYPE == RTCM_TYPE_GLO_L1L2_OBS_EXT, "1012");
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_obs::accepts(RTCM_TYPE_GLO_L1_OBS));
    CASTLE_SAMPLE_CHECK(!glonass_rtk_l1_obs::accepts(RTCM_TYPE_GLO_L1_OBS_EXT)); // another alias
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_l2_obs_ext::accepts(RTCM_TYPE_GLO_L1L2_OBS_EXT));
    CASTLE_SAMPLE_CHECK(!glonass_rtk_l1_l2_obs_ext::accepts(1008U));
    CASTLE_SAMPLE_CHECK(!glonass_rtk_l1_l2_obs_ext::accepts(RTCM_TYPE_GPS_L1_OBS));

    // The number fixes which optional satellite fields are written.
    static_assert(!glonass_rtk_l1_obs::IS_EXTENDED && !glonass_rtk_l1_obs::HAS_L2, "1009: L1 only");
    static_assert(glonass_rtk_l1_obs_ext::IS_EXTENDED && !glonass_rtk_l1_obs_ext::HAS_L2, "1010: L1, extended");
    static_assert(!glonass_rtk_l1_l2_obs::IS_EXTENDED && glonass_rtk_l1_l2_obs::HAS_L2, "1011: L1 and L2");
    static_assert(glonass_rtk_l1_l2_obs_ext::IS_EXTENDED && glonass_rtk_l1_l2_obs_ext::HAS_L2,
                  "1012: L1 and L2, extended");

    // 2. Physical scales of the raw fields (the same for all four numbers).
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_obs_ext::PSEUDORANGE_RESOLUTION_M == 0.02);
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_obs_ext::PHASE_RESOLUTION_M == 0.0005);
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_obs_ext::AMBIGUITY_M == 599584.916);
    CASTLE_SAMPLE_CHECK(glonass_rtk_l1_obs_ext::CNR_RESOLUTION_DBHZ == 0.25);

    // A pseudorange of 21 000 000.46 m is sent as ambiguity count (7 bit) + remainder in 0.02 m units (25 bit).
    const double range_m = 21000000.46;
    const uint8_t ambiguity = static_cast<uint8_t>(range_m / glonass_rtk_l1_obs_ext::AMBIGUITY_M);
    const double remainder_m = range_m - (static_cast<double>(ambiguity) * glonass_rtk_l1_obs_ext::AMBIGUITY_M);
    const uint32_t pseudorange_units =
        static_cast<uint32_t>((remainder_m / glonass_rtk_l1_obs_ext::PSEUDORANGE_RESOLUTION_M) + 0.5);
    CASTLE_SAMPLE_CHECK(ambiguity == 35U);
    CASTLE_SAMPLE_CHECK(pseudorange_units < (1UL << 25U));

    // 3. Build message 1010 (L1, extended) with two satellites; values are raw wire integers.
    glonass_rtk_l1_obs_ext message;
    message.station_id = 77U;
    message.time_of_day_ms = 12345678U;
    message.synchronous = false;
    message.divergence_free = true;
    message.smoothing_interval = 5U;
    for (uint8_t index = 0U; index < 2U; ++index)
    {
        glonass_rtk_satellite satellite;
        satellite.satellite_id = static_cast<uint8_t>(5U + index);
        satellite.l1_code = index;
        satellite.frequency_channel = static_cast<uint8_t>(3U + index); // Channel number + 7.
        satellite.l1_pseudorange = 22345678U + index;
        satellite.l1_phase_minus_pseudorange = -12345 + static_cast<int32_t>(index);
        satellite.l1_lock_time = static_cast<uint8_t>(100U + index);
        satellite.l1_ambiguity = static_cast<uint8_t>(70U + index);
        satellite.l1_cnr = static_cast<uint8_t>(160U + index);
        satellite.l2_code = 1U; // Not carried by 1010: ignored.
        CASTLE_SAMPLE_CHECK(castle::succeeded(message.satellites.push_back(satellite)));
    }

    uint8_t frame[RTCM_MAX_FRAME_SIZE] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(message, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));

    // Frame produced by the encoder and decoded independently with pyrtcm.
    CASTLE_CONSTEXPR uint8_t expected[] = {
        0xD3U, 0x00U, 0x1CU, 0x3FU, 0x20U, 0x4DU, 0x17U, 0x8CU, 0x29U, 0xC1U, 0x68U, 0xA1U,
        0xD5U, 0x3DU, 0xF3U, 0xBFU, 0x3FU, 0x1FU, 0x24U, 0x6AU, 0x01U, 0xA4U, 0xAAU, 0x7BU,
        0xE7U, 0xFEU, 0x7EU, 0x46U, 0x58U, 0xF4U, 0x20U, 0x9CU, 0x51U, 0x41U};
    CASTLE_SAMPLE_CHECK(written == sizeof(expected));
    for (castle::size_type index = 0U; index < written; ++index)
    {
        CASTLE_SAMPLE_CHECK(frame[index] == expected[index]);
    }
    CASTLE_SAMPLE_CHECK(frame[RTCM_FRAME_INDEX_PREAMBLE] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(castle::checksums::crc24q::calculate(frame, written) == 0U); // CRC covers itself: residue 0.

    // 4. The same satellites sent as each of the four numbers.
    //    Bits per satellite are 64, 79, 107 and 130 after a 61 bit header.
    check_layout<glonass_rtk_l1_obs>(message, 64U);
    check_layout<glonass_rtk_l1_obs_ext>(message, 79U);
    check_layout<glonass_rtk_l1_l2_obs>(message, 107U);
    check_layout<glonass_rtk_l1_l2_obs_ext>(message, 130U);

    // 5. The message can also write into a bit_writer directly (payload only, no framing).
    glonass_rtk_l1_obs l1_only;
    l1_only.satellites = message.satellites;
    uint8_t payload[32U] = {};
    bit_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    l1_only.encode(writer);
    CASTLE_SAMPLE_CHECK(writer.ok());
    CASTLE_SAMPLE_CHECK(writer.bit_size() == 61U + (2U * 64U));
    CASTLE_SAMPLE_CHECK(writer.size() == 24U);
    CASTLE_SAMPLE_CHECK(payload[0U] == 0x3FU); // 1009 = 0x3F1: first byte of DF002.
    CASTLE_SAMPLE_CHECK((payload[1U] >> 4U) == 0x1U);

    // 6. Failures: nothing is transmitted and bytes_written is 0.
    glonass_rtk_l1_l2_obs_ext dual;
    dual.satellites = message.satellites;
    dual.time_of_day_ms = 0x8000000U; // DF034 is 27 bit.
    result = encode_message(dual, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(written == 0U);
    dual.time_of_day_ms = 12345678U;

    dual.satellites[0U].l1_ambiguity = 128U; // DF044 is 7 bit and carried by 1010 and 1012.
    result = encode_message(dual, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::out_of_range);
    l1_only.satellites = dual.satellites; // Ignored by 1009.
    result = encode_message(l1_only, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    dual.satellites[0U].l1_ambiguity = 70U;

    result = encode_message(dual, frame, frame_size(15U), written); // 1012 needs a 47 byte frame.
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);
    result = encode_message(dual, static_cast<uint8_t*>(nullptr), sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::invalid_argument);

    // 7. The aliases hold 31 satellites (the 5 bit count); a smaller capacity is the second template parameter.
    glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 1U> single;
    CASTLE_SAMPLE_CHECK(castle::succeeded(single.satellites.push_back(glonass_rtk_satellite{})));
    CASTLE_SAMPLE_CHECK(single.satellites.push_back(glonass_rtk_satellite{}) == castle::status::full);
    result = encode_message(single, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == frame_size(24U)); // 61 + 130 bits.

    // 8. No satellites is valid: a header-only message of 61 bits.
    glonass_rtk_l1_obs empty;
    result = encode_message(empty, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == frame_size(8U));

    return 0;
}
