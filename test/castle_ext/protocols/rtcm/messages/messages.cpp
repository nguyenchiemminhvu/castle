#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/messages.hpp"

#include <stdlib.h>
#include <string.h>

namespace
{
using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

CASTLE_CONSTEXPR uint8_t FILL = 0xAAU;

struct golden_entry
{
    uint16_t type;
    const char* hex;
};

// One reference frame per message type, produced by the encoders below and decoded and
// CRC-checked independently with pyrtcm.
const golden_entry GOLDEN[] = {
    {1005,
     "d300133ed4d23b0960cfd99c80b70370c08b743c4cac19d1f1"},
    {1006,
     "d300153ee4d23b0960cfd99c80b70370c08b743c4cac3a9859bceb"},
    {1007,
     "d300193ef4d21454524d35393830302e303020202020204e4f4e4503f5d214"},
    {1033,
     "d3002a4094d203414e540205534e3132330d55424c4f58205a45442d4639500848504720312e3332043030343230571f"},
    {1230,
     "d3000a4ce4d28dff9c00c880006fbb59"},
    {1001,
     "d300173e904d1d6f34562d1578c29df9f8f906de30a7fe7e46509fd6ba"},
    {1002,
     "d3001b3ea04d1d6f34562d1578c29df9f8f911a806de30a7fe7e46547a106a85e9"},
    {1003,
     "d300223eb04d1d6f34562d1578c29df9f8f92b1e0350c6d0dbc614ffcfc8cb58f21a8656c0752736"},
    {1004,
     "d300283ec04d1d6f34562d1578c29df9f8f911a82b1e0350c6d4b0dbc614ffcfc8ca8f4358f21a8656e5c047f73c"},
    {1009,
     "d300183f104d178c29c168a1d53df3bf3f1f20d2553df3ff3f232848b73c"},
    {1010,
     "d3001c3f204d178c29c168a1d53df3bf3f1f246a01a4aa7be7fe7e4658f4209c5141"},
    {1011,
     "d300233f304d178c29c168a1d53df3bf3f1f2363c06a18da1a4aa7be7fe7e4656c790d432b60030f72"},
    {1012,
     "d300293f404d178c29c168a1d53df3bf3f1f246a06c780d431b52c34954f7cffcfc8cb1e85b1e4350cadcb803e34af"},
    {1074,
     "d300354324d21d6f3457404f100040020000000020008000728c96a03f40080215e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d355760d6bd74"},
    {1075,
     "d300434334d21d6f3457404f100040020000000020008000728c96a0f123f400807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36f86df0dbe1b615d7b2"},
    {1077,
     "d3004c4354d21d6f3457404f100040020000000020008000728c96a0f123f400807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e018080280cb4453154571c36f86df0dbe1b60aba972"},
    {1084,
     "d3003543c4d262f1853b404f100040020000000020008000728c96a03f40080215e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d3557604f10cf"},
    {1085,
     "d3004343d4d262f1853b404f100040020000000020008000728c96a0f123f400807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36f86df0dbe1b61b8091"},
    {1087,
     "d3004c43f4d262f1853b404f100040020000000020008000728c96a0f123f400807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e018080280cb4453154571c36f86df0dbe1b6034af55"},
    {1094,
     "d300354464d21d6f3457404f100040020000000020008000728c96a03f40080215e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d355760ed174a"},
    {1095,
     "d300434474d21d6f3457404f100040020000000020008000728c96a0f123f400807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36f86df0dbe1b6fb0524"},
    {1097,
     "d3004c4494d21d6f3457404f100040020000000020008000728c96a0f123f400807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e018080280cb4453154571c36f86df0dbe1b609fa849"},
    {1124,
     "d300354644d21d6f3457404f100040020000000020008000728c96a03f40080215e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d35576069f607"},
    {1125,
     "d300434654d21d6f3457404f100040020000000020008000728c96a0f123f400807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36f86df0dbe1b68406ba"},
    {1127,
     "d3004c4674d21d6f3457404f100040020000000020008000728c96a0f123f400807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e018080280cb4453154571c36f86df0dbe1b603776f3"},
};

const char* golden(uint16_t type)
{
    for (const golden_entry& entry : GOLDEN)
    {
        if (entry.type == type)
        {
            return entry.hex;
        }
    }
    return nullptr;
}

castle::size_type parse_hex(const char* text, uint8_t* out)
{
    castle::size_type count = 0U;
    for (; text[0] != '\0'; text += 2)
    {
        const char pair[3] = {text[0], text[1], '\0'};
        out[count++] = static_cast<uint8_t>(strtoul(pair, nullptr, 16));
    }
    return count;
}

/** @brief Reads the 12 bit message number of a frame. */
uint16_t message_number(const uint8_t* frame)
{
    return static_cast<uint16_t>((frame[RTCM_FRAME_INDEX_PAYLOAD] << 4U) | (frame[RTCM_FRAME_INDEX_PAYLOAD + 1U] >> 4U));
}

/** @brief Checks preamble, length field, message number and CRC-24Q residue of a complete frame. */
void expect_well_formed(const uint8_t* frame, castle::size_type size, uint16_t type)
{
    ASSERT_GE(size, RTCM_FRAME_OVERHEAD);
    EXPECT_EQ(frame[0U], RTCM_PREAMBLE);
    EXPECT_EQ((static_cast<castle::size_type>(frame[1U] & 0x03U) << 8U) | frame[2U], size - RTCM_FRAME_OVERHEAD);
    EXPECT_EQ(message_number(frame), type);
    EXPECT_EQ(castle::checksums::crc24q::calculate(frame, size), 0U);
}

/**
 * @brief Encodes `model`, compares it with the golden frame of `type`, then repeats the
 * encode with an exact-fit and a one-byte-short buffer.
 */
template <typename Model>
void expect_golden(const Model& model, uint16_t type)
{
    SCOPED_TRACE(type);
    const char* hex = golden(type);
    ASSERT_NE(hex, nullptr);
    uint8_t expected[RTCM_MAX_FRAME_SIZE];
    const castle::size_type length = parse_hex(hex, expected);

    uint8_t frame[RTCM_MAX_FRAME_SIZE];
    memset(frame, FILL, sizeof(frame));
    castle::size_type written = 12345U;
    ASSERT_EQ(encode_message(model, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, length);
    EXPECT_EQ(memcmp(frame, expected, length), 0);
    EXPECT_EQ(frame[length], FILL);
    expect_well_formed(frame, written, type);

    memset(frame, FILL, sizeof(frame));
    ASSERT_EQ(encode_message(model, frame, length, written), castle::status::ok);
    EXPECT_EQ(written, length);
    EXPECT_EQ(memcmp(frame, expected, length), 0);
    EXPECT_EQ(frame[length], FILL);

    written = 99U;
    EXPECT_EQ(encode_message(model, frame, length - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

template <typename Message>
Message make_station()
{
    Message value;
    value.station_id = 1234U;
    value.itrf_year = 14U;
    value.gps = true;
    value.glonass = true;
    value.galileo = false;
    value.reference_station_indicator = false;
    value.single_receiver_oscillator = true;
    value.quarter_cycle_indicator = 2U;
    value.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    if constexpr (Message::HAS_ANTENNA_HEIGHT)
    {
        value.set_antenna_height_m(1.5);
    }
    return value;
}

antenna_descriptor make_antenna()
{
    antenna_descriptor value;
    value.station_id = 1234U;
    EXPECT_EQ(value.descriptor.assign("TRM59800.00     NONE"), castle::status::ok);
    value.setup_id = 3U;
    return value;
}

receiver_antenna_descriptor make_receiver_antenna()
{
    receiver_antenna_descriptor value;
    value.station_id = 1234U;
    EXPECT_EQ(value.antenna.assign("ANT"), castle::status::ok);
    value.antenna_setup_id = 2U;
    EXPECT_EQ(value.antenna_serial.assign("SN123"), castle::status::ok);
    EXPECT_EQ(value.receiver_type.assign("UBLOX ZED-F9P"), castle::status::ok);
    EXPECT_EQ(value.firmware.assign("HPG 1.32"), castle::status::ok);
    EXPECT_EQ(value.receiver_serial.assign("0042"), castle::status::ok);
    return value;
}

glonass_code_phase_biases make_biases()
{
    glonass_code_phase_biases value;
    value.station_id = 1234U;
    value.code_phase_bias_indicator = true;
    value.signals_mask = 0x0DU;
    value.l1_ca_bias = -100;
    value.l1_p_bias = 200;
    value.l2_p_bias = -32768;
    return value;
}

template <typename Message>
Message make_gps()
{
    Message value;
    value.station_id = 77U;
    value.tow_ms = 123456789U;
    value.synchronous = true;
    value.divergence_free = true;
    value.smoothing_interval = 5U;
    for (uint32_t index = 0U; index < 2U; ++index)
    {
        gps_rtk_satellite sat;
        sat.satellite_id = static_cast<uint8_t>(5U + index);
        sat.l1_code = static_cast<uint8_t>(index);
        sat.l1_pseudorange = 12345678U + index;
        sat.l1_phase_minus_pseudorange = -12345 + static_cast<int32_t>(index);
        sat.l1_lock_time = static_cast<uint8_t>(100U + index);
        sat.l1_ambiguity = static_cast<uint8_t>(70U + index);
        sat.l1_cnr = static_cast<uint8_t>(160U + index);
        sat.l2_code = 2U;
        sat.l2_pseudorange_difference = static_cast<int16_t>(-5000 + static_cast<int32_t>(index));
        sat.l2_phase_minus_l1_pseudorange = 54321 + static_cast<int32_t>(index);
        sat.l2_lock_time = static_cast<uint8_t>(90U + index);
        sat.l2_cnr = static_cast<uint8_t>(150U + index);
        EXPECT_EQ(value.satellites.push_back(sat), castle::status::ok);
    }
    return value;
}

template <typename Message>
Message make_glonass()
{
    Message value;
    value.station_id = 77U;
    value.time_of_day_ms = 12345678U;
    value.synchronous = false;
    value.divergence_free = true;
    value.smoothing_interval = 5U;
    for (uint32_t index = 0U; index < 2U; ++index)
    {
        glonass_rtk_satellite sat;
        sat.satellite_id = static_cast<uint8_t>(5U + index);
        sat.l1_code = static_cast<uint8_t>(index);
        sat.frequency_channel = static_cast<uint8_t>(3U + index);
        sat.l1_pseudorange = 22345678U + index;
        sat.l1_phase_minus_pseudorange = -12345 + static_cast<int32_t>(index);
        sat.l1_lock_time = static_cast<uint8_t>(100U + index);
        sat.l1_ambiguity = static_cast<uint8_t>(70U + index);
        sat.l1_cnr = static_cast<uint8_t>(160U + index);
        sat.l2_code = 1U;
        sat.l2_pseudorange_difference = static_cast<int16_t>(-5000 + static_cast<int32_t>(index));
        sat.l2_phase_minus_l1_pseudorange = 54321 + static_cast<int32_t>(index);
        sat.l2_lock_time = static_cast<uint8_t>(90U + index);
        sat.l2_cnr = static_cast<uint8_t>(150U + index);
        EXPECT_EQ(value.satellites.push_back(sat), castle::status::ok);
    }
    return value;
}

/** @brief Three satellites, two signals, four cells: (s0: both), (s1: signal 0), (s2: signal 1). */
template <typename Message>
Message make_msm()
{
    Message value;
    value.station_id = 1234U;
    value.epoch_time = 123456789U;
    value.multiple_message = true;
    value.iods = 5U;
    value.clock_steering = 2U;
    value.external_clock = 1U;
    value.divergence_free = true;
    value.smoothing_interval = 6U;
    value.satellite_mask = Message::satellite_bit(3U) | Message::satellite_bit(17U) | Message::satellite_bit(30U);
    value.signal_mask = Message::signal_bit(2U) | Message::signal_bit(16U);
    value.cell_mask = Message::cell_bit(0U, 0U, 3U, 2U) | Message::cell_bit(0U, 1U, 3U, 2U)
        | Message::cell_bit(1U, 0U, 3U, 2U) | Message::cell_bit(2U, 1U, 3U, 2U);

    const double ranges[3] = {70.123456, 75.5, 80.25};
    for (uint32_t index = 0U; index < 3U; ++index)
    {
        msm_satellite sat{};
        if constexpr (Message::HAS_RATE)
        {
            Message::set_rough_range(sat, ranges[index], -123.4 + (50.0 * static_cast<double>(index)));
        }
        else
        {
            Message::set_rough_range(sat, ranges[index]);
        }
        sat.extended_info = static_cast<uint8_t>(7U + index);
        EXPECT_EQ(value.satellites.push_back(sat), castle::status::ok);
    }

    const uint32_t satellite_of_cell[4] = {0U, 0U, 1U, 2U};
    for (uint32_t cell = 0U; cell < 4U; ++cell)
    {
        const msm_satellite& sat = value.satellites[satellite_of_cell[cell]];
        const double range = ranges[satellite_of_cell[cell]] + (0.0001 * static_cast<double>(cell + 1U));
        msm_signal signal{};
        Message::set_pseudorange(sat, signal, range);
        Message::set_phase_range(sat, signal, range + 0.00002);
        signal.lock_time = static_cast<uint16_t>(3U + cell);
        signal.half_cycle = (cell & 1U) != 0U;
        Message::set_cnr(signal, 40.5 + static_cast<double>(cell));
        if constexpr (Message::HAS_RATE)
        {
            Message::set_phase_range_rate(
                sat, signal, -123.4 + (50.0 * static_cast<double>(satellite_of_cell[cell])) + 0.0123);
        }
        EXPECT_EQ(value.signals.push_back(signal), castle::status::ok);
    }
    return value;
}

template <typename Message>
Message make_glonass_msm()
{
    Message value = make_msm<Message>();
    value.set_glonass_epoch(3U, 12345678U);
    return value;
}

TEST(RtcmMessagesUmbrella, StationMessagesMatchGoldenFrames)
{
    expect_golden(make_station<station_arp>(), 1005U);
    expect_golden(make_station<station_arp_height>(), 1006U);
    expect_golden(make_antenna(), 1007U);
    expect_golden(make_receiver_antenna(), 1033U);
    expect_golden(make_biases(), 1230U);
}

TEST(RtcmMessagesUmbrella, LegacyObservablesMatchGoldenFrames)
{
    expect_golden(make_gps<gps_rtk_l1_obs>(), 1001U);
    expect_golden(make_gps<gps_rtk_l1_obs_ext>(), 1002U);
    expect_golden(make_gps<gps_rtk_l1_l2_obs>(), 1003U);
    expect_golden(make_gps<gps_rtk_l1_l2_obs_ext>(), 1004U);
    expect_golden(make_glonass<glonass_rtk_l1_obs>(), 1009U);
    expect_golden(make_glonass<glonass_rtk_l1_obs_ext>(), 1010U);
    expect_golden(make_glonass<glonass_rtk_l1_l2_obs>(), 1011U);
    expect_golden(make_glonass<glonass_rtk_l1_l2_obs_ext>(), 1012U);
}

TEST(RtcmMessagesUmbrella, MultipleSignalMessagesMatchGoldenFrames)
{
    expect_golden(make_msm<gps_msm4>(), 1074U);
    expect_golden(make_msm<gps_msm5>(), 1075U);
    expect_golden(make_msm<gps_msm7>(), 1077U);
    expect_golden(make_glonass_msm<glonass_msm4>(), 1084U);
    expect_golden(make_glonass_msm<glonass_msm5>(), 1085U);
    expect_golden(make_glonass_msm<glonass_msm7>(), 1087U);
    expect_golden(make_msm<galileo_msm4>(), 1094U);
    expect_golden(make_msm<galileo_msm5>(), 1095U);
    expect_golden(make_msm<galileo_msm7>(), 1097U);
    expect_golden(make_msm<beidou_msm4>(), 1124U);
    expect_golden(make_msm<beidou_msm5>(), 1125U);
    expect_golden(make_msm<beidou_msm7>(), 1127U);
}

/** @brief `Model` accepts its own number and no other supported number. */
template <typename Model>
void expect_accepts_only_own_number()
{
    for (const golden_entry& entry : GOLDEN)
    {
        EXPECT_EQ(Model::accepts(entry.type), entry.type == Model::MESSAGE_TYPE)
            << "model " << Model::MESSAGE_TYPE << " probed with " << entry.type;
    }
}

template <typename... Models>
struct model_list
{
    /** @brief Number of models in the list that accept message number `type`. */
    static unsigned acceptor_count(uint16_t type)
    {
        return (0U + ... + static_cast<unsigned>(Models::accepts(type)));
    }

    static void expect_each_accepts_only_own_number()
    {
        (expect_accepts_only_own_number<Models>(), ...);
    }
};

/** @brief Every message model of the umbrella, one per message number. */
using umbrella_models = model_list<gps_rtk_l1_obs, gps_rtk_l1_obs_ext, gps_rtk_l1_l2_obs, gps_rtk_l1_l2_obs_ext,
                                   glonass_rtk_l1_obs, glonass_rtk_l1_obs_ext, glonass_rtk_l1_l2_obs,
                                   glonass_rtk_l1_l2_obs_ext, station_arp, station_arp_height, antenna_descriptor,
                                   receiver_antenna_descriptor, glonass_code_phase_biases, gps_msm4, gps_msm5, gps_msm7,
                                   glonass_msm4, glonass_msm5, glonass_msm7, galileo_msm4, galileo_msm5, galileo_msm7,
                                   beidou_msm4, beidou_msm5, beidou_msm7>;

unsigned acceptor_count(uint16_t type)
{
    return umbrella_models::acceptor_count(type);
}

TEST(RtcmMessagesUmbrella, EverySupportedMessageNumberHasExactlyOneModel)
{
    for (const golden_entry& entry : GOLDEN)
    {
        EXPECT_EQ(acceptor_count(entry.type), 1U) << entry.type;
    }
}

TEST(RtcmMessagesUmbrella, UnsupportedMessageNumbersHaveNoModel)
{
    // Neighbours of the supported numbers, MSM levels that are not offered and the
    // u-blox proprietary 4072 (reported by UBX-RXM-RTCM, not an input message).
    const uint16_t unsupported[] = {0U,    1000U, 1008U, 1013U, 1032U, 1034U, 1229U, 1231U, 1071U, 1072U, 1073U,
                                    1076U, 1078U, 1079U, 1081U, 1086U, 1088U, 1091U, 1096U, 1098U, 1123U, 1126U,
                                    1128U, 4072U, 65535U};
    for (const uint16_t type : unsupported)
    {
        EXPECT_EQ(acceptor_count(type), 0U) << type;
    }
}

TEST(RtcmMessagesUmbrella, MessageNumbersDoNotCrossBetweenModels)
{
    // Exactly one acceptor per number is not enough: it must be the model of that number.
    umbrella_models::expect_each_accepts_only_own_number();
}

TEST(RtcmMessagesUmbrella, MixedEpochCanBeConcatenatedIntoOneBuffer)
{
    // A typical correction epoch: station position, GPS and GLONASS observables, MSM7.
    uint8_t stream[RTCM_MAX_FRAME_SIZE * 2U];
    memset(stream, FILL, sizeof(stream));
    castle::size_type used = 0U;
    castle::size_type written = 0U;

    ASSERT_EQ(encode_message(make_station<station_arp_height>(), &stream[used], sizeof(stream) - used, written),
              castle::status::ok);
    expect_well_formed(&stream[used], written, 1006U);
    used += written;

    ASSERT_EQ(encode_message(make_gps<gps_rtk_l1_l2_obs_ext>(), &stream[used], sizeof(stream) - used, written),
              castle::status::ok);
    expect_well_formed(&stream[used], written, 1004U);
    used += written;

    ASSERT_EQ(encode_message(make_glonass<glonass_rtk_l1_l2_obs_ext>(), &stream[used], sizeof(stream) - used, written),
              castle::status::ok);
    expect_well_formed(&stream[used], written, 1012U);
    used += written;

    ASSERT_EQ(encode_message(make_msm<gps_msm7>(), &stream[used], sizeof(stream) - used, written), castle::status::ok);
    expect_well_formed(&stream[used], written, 1077U);
    used += written;

    EXPECT_EQ(used, frame_size(21U) + frame_size(40U) + frame_size(41U) + frame_size(76U));
    EXPECT_EQ(stream[used], FILL);
}

TEST(RtcmMessagesUmbrella, EncodersLeaveNoStateBehind)
{
    // The same model object encodes identically twice.
    const gps_rtk_l1_l2_obs_ext gps = make_gps<gps_rtk_l1_l2_obs_ext>();
    uint8_t first[RTCM_MAX_FRAME_SIZE];
    uint8_t second[RTCM_MAX_FRAME_SIZE];
    castle::size_type first_size = 0U;
    castle::size_type second_size = 0U;
    ASSERT_EQ(encode_message(gps, first, sizeof(first), first_size), castle::status::ok);
    ASSERT_EQ(encode_message(gps, second, sizeof(second), second_size), castle::status::ok);
    ASSERT_EQ(first_size, second_size);
    EXPECT_EQ(memcmp(first, second, first_size), 0);
}

} // namespace
