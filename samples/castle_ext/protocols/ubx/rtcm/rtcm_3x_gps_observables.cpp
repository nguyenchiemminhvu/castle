/**
 * @file rtcm_3x_gps_observables.cpp
 * @brief u-blox RTCM-3X-TYPE1001..1004 (Class/ID 0xf5 0x01..0x04): legacy GPS RTK observables.
 *
 * | u-blox message      | Class/ID  | Castle model            |
 * |---------------------|-----------|-------------------------|
 * | RTCM-3X-TYPE1001    | 0xf5 0x01 | `gps_rtk_l1_obs`        |
 * | RTCM-3X-TYPE1002    | 0xf5 0x02 | `gps_rtk_l1_obs_ext`    |
 * | RTCM-3X-TYPE1003    | 0xf5 0x03 | `gps_rtk_l1_l2_obs`     |
 * | RTCM-3X-TYPE1004    | 0xf5 0x04 | `gps_rtk_l1_l2_obs_ext` |
 *
 * Each message number is its own type; it fixes which satellite fields reach
 * the wire. The aliases hold up to 31 satellites; a smaller capacity is
 * `gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS_EXT, 8U>`. The frame
 * `rtcm::encode_message()` produces is the whole u-blox payload table
 * (`0xd3`, length, data, CRC-24Q) and is written to the receiver port as is.
 */
#include "rtcm_input_support.hpp"

#include "castle_ext/protocols/rtcm/messages/gps_rtk_observables.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;
using namespace rtcm_sample;

/** @brief Message header (DF002..DF008) in bits. */
CASTLE_CONSTEXPR castle::size_type HEADER_BITS = 64U;

/** @brief GPS_EPOCH as `Message`. Every field is filled; the encoder writes only those the message number carries. */
template <typename Message>
Message make_epoch()
{
    Message message{};
    message.station_id = STATION_ID;
    message.tow_ms = GPS_TOW_MS;
    message.synchronous = false; // DF005: no further GNSS observables follow for this epoch.

    for (const observation& obs : GPS_EPOCH)
    {
        gps_rtk_satellite sat{};
        sat.satellite_id = obs.id; // DF009: PRN
        sat.l1_code = 0U;          // DF010: C/A
        set_legacy_l1<Message>(sat, obs);
        sat.l2_code = 0U;          // DF016: C/A or L2C
        set_legacy_l2<Message>(sat, obs);
        CASTLE_SAMPLE_CHECK(castle::succeeded(message.satellites.push_back(sat)));
    }
    return message;
}

/** @brief Sends `Message` and checks the frame on the port, including its payload size. */
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
    // RTCM-3X-TYPE1001 (0xf5 0x01): L1-only GPS RTK observables.
    // DF009 PRN, DF010 code, DF011 pseudorange, DF012 phase - pseudorange, DF013 lock time.
    send<gps_rtk_l1_obs>(58U);

    // RTCM-3X-TYPE1002 (0xf5 0x02): extended L1-only; adds DF014 ambiguity and DF015 CNR.
    send<gps_rtk_l1_obs_ext>(74U);

    // RTCM-3X-TYPE1003 (0xf5 0x03): L1/L2; adds DF016..DF019 (L2 code, range difference, phase, lock time).
    send<gps_rtk_l1_l2_obs>(101U);

    // RTCM-3X-TYPE1004 (0xf5 0x04): extended L1/L2; DF014/DF015 plus DF020 L2 CNR. The usual choice of the four.
    send<gps_rtk_l1_l2_obs_ext>(125U);

    // The rover rebuilds the full L1 pseudorange from ambiguity and remainder: within half an LSB (0.01 m).
    const gps_rtk_l1_l2_obs_ext message = make_epoch<gps_rtk_l1_l2_obs_ext>();
    const double rebuilt_m = l1_pseudorange_m<gps_rtk_l1_l2_obs_ext>(message.satellites[0U]);
    const double error_m = rebuilt_m - GPS_EPOCH[0U].pseudorange_m[0U];
    CASTLE_SAMPLE_CHECK((error_m < 0.0101) && (error_m > -0.0101));
    CASTLE_SAMPLE_CHECK(message.satellites[0U].l1_ambiguity == 71U); // 21345678 m / 299792.458 m
    CASTLE_SAMPLE_CHECK(message.satellites[0U].l1_cnr == 189U);      // 47.25 dB-Hz / 0.25

    // DF004 is 30 bits: a time of week beyond it is reported, not truncated, and nothing is queued on the port.
    gps_rtk_l1_l2_obs_ext late = message;
    late.tow_ms = 1U << 30U;
    const castle::size_type queued = uart1.size;
    CASTLE_SAMPLE_CHECK(uart1.send_rtcm(late) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(uart1.size == queued);
    return 0;
}
