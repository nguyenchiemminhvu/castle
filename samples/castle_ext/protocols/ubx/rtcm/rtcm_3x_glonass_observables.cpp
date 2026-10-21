/**
 * @file rtcm_3x_glonass_observables.cpp
 * @brief u-blox RTCM-3X-TYPE1009..1012 (Class/ID 0xf5 0x09, 0x0a, 0xa1, 0xa2): legacy GLONASS RTK observables.
 *
 * | u-blox message      | Class/ID  | Castle model                |
 * |---------------------|-----------|-----------------------------|
 * | RTCM-3X-TYPE1009    | 0xf5 0x09 | `glonass_rtk_l1_obs`        |
 * | RTCM-3X-TYPE1010    | 0xf5 0x0a | `glonass_rtk_l1_obs_ext`    |
 * | RTCM-3X-TYPE1011    | 0xf5 0xa1 | `glonass_rtk_l1_l2_obs`     |
 * | RTCM-3X-TYPE1012    | 0xf5 0xa2 | `glonass_rtk_l1_l2_obs_ext` |
 *
 * The aliases hold up to 31 satellites; a smaller capacity is
 * `glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 8U>`.
 * The u-blox IDs are not `type - 1000` here (0xa1, 0xa2): they are catalogue
 * labels only and never appear on the wire. The frame starts with DF002 = 1009..1012.
 */
#include "rtcm_input_support.hpp"

#include "castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;
using namespace rtcm_sample;

/** @brief Message header (DF002, DF003, DF034..DF037) in bits. */
CASTLE_CONSTEXPR castle::size_type HEADER_BITS = 61U;

/** @brief GLONASS_EPOCH as `Message`. */
template <typename Message>
Message make_epoch()
{
    Message message{};
    message.station_id = STATION_ID;
    message.time_of_day_ms = GLONASS_TIME_OF_DAY_MS; // DF034: GLONASS epoch time tk
    message.synchronous = false;

    for (const observation& obs : GLONASS_EPOCH)
    {
        glonass_rtk_satellite sat{};
        sat.satellite_id = obs.id;                                          // DF038: slot number
        sat.l1_code = 0U;                                                   // DF039: C/A
        sat.frequency_channel = static_cast<uint8_t>(obs.frequency_channel + 7); // DF040: k + 7
        set_legacy_l1<Message>(sat, obs);
        sat.l2_code = 0U;                                                   // DF046: C/A
        set_legacy_l2<Message>(sat, obs);
        CASTLE_SAMPLE_CHECK(castle::succeeded(message.satellites.push_back(sat)));
    }
    return message;
}

template <typename Message>
void send(castle::size_type bits_per_satellite)
{
    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(make_epoch<Message>())));
    check_rtcm_frame(uart1, Message::MESSAGE_TYPE, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size((HEADER_BITS + (3U * bits_per_satellite) + 7U) / 8U));
}

} // namespace

int main()
{
    // RTCM-3X-TYPE1009 (0xf5 0x09): L1-only GLONASS RTK observables.
    send<glonass_rtk_l1_obs>(64U);

    // RTCM-3X-TYPE1010 (0xf5 0x0a): extended L1-only; adds DF044 ambiguity (2 light-ms units) and DF045 CNR.
    send<glonass_rtk_l1_obs_ext>(79U);

    // RTCM-3X-TYPE1011 (0xf5 0xa1): L1&L2; adds DF046..DF049.
    send<glonass_rtk_l1_l2_obs>(107U);

    // RTCM-3X-TYPE1012 (0xf5 0xa2): extended L1&L2; adds DF044, DF045 and DF050.
    send<glonass_rtk_l1_l2_obs_ext>(130U);

    // DF040 carries k + 7, so channels -7..+6 travel as 0..13.
    const glonass_rtk_l1_l2_obs_ext message = make_epoch<glonass_rtk_l1_l2_obs_ext>();
    CASTLE_SAMPLE_CHECK(message.satellites[0U].frequency_channel == 12U); // k = +5
    CASTLE_SAMPLE_CHECK(message.satellites[1U].frequency_channel == 0U);  // k = -7
    CASTLE_SAMPLE_CHECK(message.satellites[0U].l1_ambiguity == 33U);      // 19876543 m / 599584.916 m

    const double error_m =
        l1_pseudorange_m<glonass_rtk_l1_l2_obs_ext>(message.satellites[0U]) - GLONASS_EPOCH[0U].pseudorange_m[0U];
    CASTLE_SAMPLE_CHECK((error_m < 0.0101) && (error_m > -0.0101));

    // DF034 is 27 bits: a time of day of 24 h or more does not fit and is reported, not truncated.
    glonass_rtk_l1_l2_obs_ext late = message;
    late.time_of_day_ms = 1U << 27U;
    CASTLE_SAMPLE_CHECK(uart1.send_rtcm(late) == castle::status::out_of_range);
    return 0;
}
