/**
 * @file rtcm_3x_msm.cpp
 * @brief u-blox RTCM-3X-TYPE1074..1127: GPS, GLONASS, Galileo and BeiDou MSM4 / MSM5 / MSM7.
 *
 * | System  | MSM4 (u-blox Class/ID) | MSM5               | MSM7               | Castle aliases |
 * |---------|------------------------|--------------------|--------------------|----------------|
 * | GPS     | 1074 (0xf5 0x4a)       | 1075 (0xf5 0x4b)   | 1077 (0xf5 0x4d)   | `gps_msm4/5/7` |
 * | GLONASS | 1084 (0xf5 0x54)       | 1085 (0xf5 0x55)   | 1087 (0xf5 0x57)   | `glonass_msm4/5/7` |
 * | Galileo | 1094 (0xf5 0x5e)       | 1095 (0xf5 0x5f)   | 1097 (0xf5 0x61)   | `galileo_msm4/5/7` |
 * | BeiDou  | 1124 (0xf5 0x7c)       | 1125 (0xf5 0x7d)   | 1127 (0xf5 0x7f)   | `beidou_msm4/5/7` |
 *
 * The u-blox ID is `type - 1000` truncated to 8 bits; like every 0xf5 ID it is a
 * catalogue label, the wire carries DF002 inside the RTCM frame.
 *
 * MSM is the format to prefer for a multi-band, multi-GNSS rover such as the
 * ZED-F9R: one message per constellation per epoch, carrying every satellite
 * and signal. MSM4 is the compact choice; MSM7 adds high resolution and
 * Doppler. All but the last message of one epoch set DF393 (`multiple_message`).
 */
#include "rtcm_input_support.hpp"

#include "castle_ext/protocols/rtcm/messages/msm.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;
using namespace rtcm_sample;

/**
 * @brief One epoch of `System`, encoded at level `Msm`, sent and checked.
 *
 * @param more_follow DF393: true unless this is the last MSM of the epoch.
 */
template <typename Msm, castle::size_type N>
void send(const observation (&epoch)[N], const castle::size_type (&signal_ids)[2U], uint32_t epoch_time, bool more_follow)
{
    Msm msm{};
    msm.station_id = STATION_ID;
    msm.epoch_time = epoch_time;
    msm.multiple_message = more_follow;
    msm.clock_steering = 1U; // DF411: clock steering not applied
    fill_msm(msm, epoch, signal_ids);

    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(msm)));
    check_rtcm_frame(uart1, Msm::MESSAGE_TYPE, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size(msm_payload_bytes<Msm>(N)));
}

/** @brief GLONASS MSM: day-of-week epoch and, for MSM5/7, the frequency channel in the extended info. */
template <typename Msm, castle::size_type N>
void send_glonass(const observation (&epoch)[N], bool more_follow)
{
    Msm msm{};
    msm.station_id = STATION_ID;
    msm.set_glonass_epoch(GLONASS_DAY_OF_WEEK, GLONASS_TIME_OF_DAY_MS); // DF416 + DF034
    msm.multiple_message = more_follow;
    msm.clock_steering = 1U;
    fill_msm(msm, epoch, GLONASS_SIGNALS);
    CASTLE_IF_CONSTEXPR (Msm::HAS_RATE)
    {
        for (castle::size_type index = 0U; index < N; ++index)
        {
            msm.satellites[index].extended_info = static_cast<uint8_t>(epoch[index].frequency_channel + 7);
        }
    }

    CASTLE_SAMPLE_CHECK(castle::succeeded(uart1.send_rtcm(msm)));
    check_rtcm_frame(uart1, Msm::MESSAGE_TYPE, STATION_ID);
    CASTLE_SAMPLE_CHECK(uart1.last_frame().size() == frame_size(msm_payload_bytes<Msm>(N)));
}

/** @brief One complete epoch at MSM level `Level`: GPS, GLONASS, Galileo, then BeiDou (last, DF393 = 0). */
template <uint8_t Level>
void send_epoch()
{
    send<msm_message<msm_system::gps, Level>>(GPS_EPOCH, GPS_SIGNALS, GPS_TOW_MS, true);
    send_glonass<msm_message<msm_system::glonass, Level>>(GLONASS_EPOCH, true);
    send<msm_message<msm_system::galileo, Level>>(GALILEO_EPOCH, GALILEO_SIGNALS, GALILEO_TOW_MS, true);
    send<msm_message<msm_system::beidou, Level>>(BEIDOU_EPOCH, BEIDOU_SIGNALS, BEIDOU_TOW_MS, false);
}

} // namespace

int main()
{
    // MSM4: RTCM-3X-TYPE1074 / 1084 / 1094 / 1124 (0xf5 0x4a / 0x54 / 0x5e / 0x7c).
    send_epoch<4U>();

    // MSM5: RTCM-3X-TYPE1075 / 1085 / 1095 / 1125 (0xf5 0x4b / 0x55 / 0x5f / 0x7d); adds rates and extended info.
    send_epoch<5U>();

    // MSM7: RTCM-3X-TYPE1077 / 1087 / 1097 / 1127 (0xf5 0x4d / 0x57 / 0x61 / 0x7f); high resolution.
    send_epoch<7U>();

    // The aliases name exactly the twelve numbers of the u-blox list.
    static_assert(gps_msm4::MESSAGE_TYPE == 1074U && gps_msm5::MESSAGE_TYPE == 1075U && gps_msm7::MESSAGE_TYPE == 1077U, "GPS");
    static_assert(glonass_msm4::MESSAGE_TYPE == 1084U && glonass_msm7::MESSAGE_TYPE == 1087U, "GLONASS");
    static_assert(galileo_msm4::MESSAGE_TYPE == 1094U && galileo_msm7::MESSAGE_TYPE == 1097U, "Galileo");
    static_assert(beidou_msm4::MESSAGE_TYPE == 1124U && beidou_msm7::MESSAGE_TYPE == 1127U, "BeiDou");

    // Masks and vectors must agree, otherwise nothing is queued.
    gps_msm7 broken{};
    broken.station_id = STATION_ID;
    fill_msm(broken, GPS_EPOCH, GPS_SIGNALS);
    broken.signals.pop_back();
    const castle::size_type queued = uart1.size;
    CASTLE_SAMPLE_CHECK(uart1.send_rtcm(broken) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(uart1.size == queued);
    return 0;
}
