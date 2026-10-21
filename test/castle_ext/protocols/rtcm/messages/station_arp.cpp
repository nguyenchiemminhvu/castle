#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

#include <string>

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

constexpr int64_t MAX_UNITS = (static_cast<int64_t>(1) << 37) - 1; // +2^37 - 1
constexpr int64_t MIN_UNITS = -(static_cast<int64_t>(1) << 37);    // -2^37

constexpr castle::size_type PAYLOAD_1005 = 19U; // 152 bits
constexpr castle::size_type PAYLOAD_1006 = 21U; // 168 bits

// Golden frames: station 1234, ITRF 14, GPS+GLONASS, single oscillator, quarter cycle 2,
// ARP (4027894.0060, 307045.6000, 4919474.9100) m. Decoded and CRC-checked with pyrtcm.
const char* const GOLDEN_1005 = "d300133ed4d23b0960cfd99c80b70370c08b743c4cac19d1f1";
const char* const GOLDEN_1006 = "d300153ee4d23b0960cfd99c80b70370c08b743c4cac3a9859bceb";

// Hand-assembled (independent Python bit packer + CRC-24Q, confirmed by pyrtcm).
const char* const ZERO_1005 = "d300133ed00000000000000000000000000000000000f24bf4";
const char* const ZERO_1006 = "d300153ee00000000000000000000000000000000000000002d21f";
const char* const MAX_1005 = "d300133edfffffdfffffffffa000000000ffffffffff6fcc02";
const char* const MAX_1006 = "d300153eefffffdfffffffffa000000000ffffffffffffffeaae97";
const char* const EXTREME_X_Y_Z = "d300133ed0000020000000001fffffffff1fffffffffdf7b4d";
const char* const NEGATIVE_1006 = "d300153ee00109369f3026643f48fc8f40748bc3b354000177deac";
const char* const ROUNDED_1005 = "d300133ed0000000000000013fffffffff0000003039c0e80e";

struct frame_buffer
{
    uint8_t data[64U] = {};
    castle::size_type size = 0U;
};

std::string to_hex(const frame_buffer& frame)
{
    static const char digits[] = "0123456789abcdef";
    std::string text;
    for (castle::size_type index = 0U; index < frame.size; ++index)
    {
        text.push_back(digits[frame.data[index] >> 4U]);
        text.push_back(digits[frame.data[index] & 0x0FU]);
    }
    return text;
}

template <typename Message>
castle::status encode(const Message& message, frame_buffer& frame)
{
    return encode_message(message, frame.data, sizeof(frame.data), frame.size);
}

template <typename Message>
void expect_frame(const Message& message, const char* hex)
{
    frame_buffer frame;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(to_hex(frame), std::string(hex));
}

/** @brief Reads `width` payload bits starting at bit `offset` (MSB first), independent of the encoder. */
uint64_t payload_bits(const frame_buffer& frame, castle::size_type offset, castle::size_type width)
{
    uint64_t value = 0U;
    for (castle::size_type bit = offset; bit < (offset + width); ++bit)
    {
        const uint8_t byte = frame.data[RTCM_FRAME_INDEX_PAYLOAD + (bit / 8U)];
        value = (value << 1U) | ((byte >> (7U - (bit % 8U))) & 1U);
    }
    return value;
}

template <typename Message>
Message golden_message()
{
    Message message{};
    message.station_id = 1234U;
    message.itrf_year = 14U;
    message.gps = true;
    message.glonass = true;
    message.single_receiver_oscillator = true;
    message.quarter_cycle_indicator = 2U;
    message.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    return message;
}

template <typename Message>
Message extreme_message()
{
    Message message{};
    message.station_id = 4095U;
    message.itrf_year = 63U;
    message.gps = true;
    message.glonass = true;
    message.galileo = true;
    message.reference_station_indicator = true;
    message.single_receiver_oscillator = true;
    message.quarter_cycle_indicator = 3U;
    message.ecef_x = MAX_UNITS;
    message.ecef_y = MIN_UNITS;
    message.ecef_z = -1;
    message.antenna_height = 65535U;
    return message;
}

TEST(RtcmMessageStationArp, DefaultsAndConstants)
{
    const station_arp message{};
    EXPECT_EQ(message.station_id, 0U);
    EXPECT_EQ(message.itrf_year, 0U);
    EXPECT_FALSE(message.gps);
    EXPECT_FALSE(message.glonass);
    EXPECT_FALSE(message.galileo);
    EXPECT_FALSE(message.reference_station_indicator);
    EXPECT_FALSE(message.single_receiver_oscillator);
    EXPECT_EQ(message.quarter_cycle_indicator, 0U);
    EXPECT_EQ(message.ecef_x, 0);
    EXPECT_EQ(message.ecef_y, 0);
    EXPECT_EQ(message.ecef_z, 0);
    EXPECT_EQ(message.antenna_height, 0U);
    EXPECT_DOUBLE_EQ(station_arp::RESOLUTION_M, 0.0001);
    EXPECT_DOUBLE_EQ(message.x_m(), 0.0);
    EXPECT_DOUBLE_EQ(message.y_m(), 0.0);
    EXPECT_DOUBLE_EQ(message.z_m(), 0.0);
    EXPECT_DOUBLE_EQ(message.antenna_height_m(), 0.0);
}

TEST(RtcmMessageStationArp, EachTypeAcceptsOnlyItsOwnNumber)
{
    static_assert(station_arp::MESSAGE_TYPE == RTCM_TYPE_STATION_ARP, "1005 model");
    static_assert(station_arp_height::MESSAGE_TYPE == RTCM_TYPE_STATION_ARP_HEIGHT, "1006 model");
    static_assert(!station_arp::HAS_ANTENNA_HEIGHT && station_arp_height::HAS_ANTENNA_HEIGHT, "height only in 1006");

    EXPECT_TRUE(station_arp::accepts(1005U));
    EXPECT_FALSE(station_arp::accepts(1006U));
    EXPECT_TRUE(station_arp_height::accepts(1006U));
    EXPECT_FALSE(station_arp_height::accepts(1005U));

    const uint16_t others[] = {0U, 1004U, 1007U, 1033U, 1230U, 4072U, 4095U, 4096U, 0xFFFFU};
    for (const uint16_t type : others)
    {
        EXPECT_FALSE(station_arp::accepts(type)) << type;
        EXPECT_FALSE(station_arp_height::accepts(type)) << type;
    }
}

TEST(RtcmMessageStationArp, EncodesGolden1005)
{
    station_arp message = golden_message<station_arp>();
    EXPECT_EQ(message.ecef_x, 40278940060);
    EXPECT_EQ(message.ecef_y, 3070456000);
    EXPECT_EQ(message.ecef_z, 49194749100);
    expect_frame(message, GOLDEN_1005);
}

TEST(RtcmMessageStationArp, EncodesGolden1006WithAntennaHeight)
{
    station_arp_height message = golden_message<station_arp_height>();
    message.set_antenna_height_m(1.5);
    EXPECT_EQ(message.antenna_height, 15000U);
    expect_frame(message, GOLDEN_1006);
}

TEST(RtcmMessageStationArp, AntennaHeightOnlyWrittenFor1006)
{
    station_arp message = golden_message<station_arp>();
    message.antenna_height = 15000U;

    frame_buffer short_frame;
    ASSERT_EQ(encode(message, short_frame), castle::status::ok);
    EXPECT_EQ(short_frame.size, frame_size(PAYLOAD_1005));
    EXPECT_EQ(to_hex(short_frame), std::string(GOLDEN_1005)); // height ignored for 1005

    station_arp_height with_height = golden_message<station_arp_height>();
    with_height.set_antenna_height_m(1.5);
    frame_buffer long_frame;
    ASSERT_EQ(encode(with_height, long_frame), castle::status::ok);
    EXPECT_EQ(long_frame.size, frame_size(PAYLOAD_1006));
    EXPECT_EQ(payload_bits(long_frame, 152U, 16U), 15000U);
}

TEST(RtcmMessageStationArp, EncodesAllZeroMessages)
{
    expect_frame(station_arp{}, ZERO_1005);
    expect_frame(station_arp_height{}, ZERO_1006);
}

TEST(RtcmMessageStationArp, EncodesMaximumFieldValues)
{
    expect_frame(extreme_message<station_arp>(), MAX_1005);
    expect_frame(extreme_message<station_arp_height>(), MAX_1006);
}

TEST(RtcmMessageStationArp, ReservedBitStaysZeroWithEveryFieldSet)
{
    frame_buffer frame;
    ASSERT_EQ(encode(extreme_message<station_arp_height>(), frame), castle::status::ok);
    EXPECT_EQ(payload_bits(frame, 73U, 1U), 0U); // DF001 reserved bit between DF142 and DF026
    EXPECT_EQ(payload_bits(frame, 72U, 1U), 1U); // DF142 right before it
    EXPECT_EQ(payload_bits(frame, 74U, 1U), 1U); // DF026 sign bit right after it
}

TEST(RtcmMessageStationArp, EveryFlagIsWrittenIndependently)
{
    struct flag_case
    {
        bool station_arp::*member;
        castle::size_type bit;
    };
    const flag_case cases[] = {
        {&station_arp::gps, 30U},
        {&station_arp::glonass, 31U},
        {&station_arp::galileo, 32U},
        {&station_arp::reference_station_indicator, 33U},
        {&station_arp::single_receiver_oscillator, 72U},
    };

    for (const flag_case& flag : cases)
    {
        station_arp message{};
        message.*(flag.member) = true;
        frame_buffer frame;
        ASSERT_EQ(encode(message, frame), castle::status::ok);

        for (const flag_case& probe : cases)
        {
            EXPECT_EQ(payload_bits(frame, probe.bit, 1U), (probe.bit == flag.bit) ? 1U : 0U)
                << "flag bit " << flag.bit << " probed at " << probe.bit;
        }
        // No data other than that single bit may change: 38-bit coordinates stay zero.
        EXPECT_EQ(payload_bits(frame, 34U, 38U), 0U);
        EXPECT_EQ(payload_bits(frame, 74U, 38U), 0U);
        EXPECT_EQ(payload_bits(frame, 114U, 38U), 0U);
    }
}

TEST(RtcmMessageStationArp, QuarterCycleIndicatorOccupiesTwoBitsForEveryValue)
{
    for (uint8_t value = 0U; value <= 3U; ++value)
    {
        station_arp message{};
        message.quarter_cycle_indicator = value;
        frame_buffer frame;
        ASSERT_EQ(encode(message, frame), castle::status::ok);
        EXPECT_EQ(payload_bits(frame, 112U, 2U), value);
        EXPECT_EQ(payload_bits(frame, 74U, 38U), 0U);
        EXPECT_EQ(payload_bits(frame, 114U, 38U), 0U);
    }

    station_arp message{};
    message.quarter_cycle_indicator = 4U;
    frame_buffer frame;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);
}

TEST(RtcmMessageStationArp, StationIdAndItrfYearBounds)
{
    station_arp message{};
    frame_buffer frame;

    message.station_id = 4095U;
    message.itrf_year = 63U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(payload_bits(frame, 12U, 12U), 4095U);
    EXPECT_EQ(payload_bits(frame, 24U, 6U), 63U);

    message.station_id = 4096U;
    message.itrf_year = 0U;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);

    message.station_id = 0U;
    message.itrf_year = 64U;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);
}

TEST(RtcmMessageStationArp, ThirtyEightBitBoundariesOfEachCoordinate)
{
    int64_t station_arp::*const members[] = {&station_arp::ecef_x, &station_arp::ecef_y, &station_arp::ecef_z};
    const castle::size_type offsets[] = {34U, 74U, 114U};

    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        frame_buffer frame;
        station_arp message{};

        message.*(members[index]) = MAX_UNITS;
        ASSERT_EQ(encode(message, frame), castle::status::ok) << "axis " << index;
        EXPECT_EQ(payload_bits(frame, offsets[index], 38U), 0x1FFFFFFFFFULL);

        message.*(members[index]) = MIN_UNITS;
        ASSERT_EQ(encode(message, frame), castle::status::ok) << "axis " << index;
        EXPECT_EQ(payload_bits(frame, offsets[index], 38U), 0x2000000000ULL);

        message.*(members[index]) = -1;
        ASSERT_EQ(encode(message, frame), castle::status::ok) << "axis " << index;
        EXPECT_EQ(payload_bits(frame, offsets[index], 38U), 0x3FFFFFFFFFULL);

        message.*(members[index]) = MAX_UNITS + 1;
        frame.size = 99U;
        EXPECT_EQ(encode(message, frame), castle::status::out_of_range) << "axis " << index;
        EXPECT_EQ(frame.size, 0U);

        message.*(members[index]) = MIN_UNITS - 1;
        frame.size = 99U;
        EXPECT_EQ(encode(message, frame), castle::status::out_of_range) << "axis " << index;
        EXPECT_EQ(frame.size, 0U);
    }

    expect_frame(station_arp{}, ZERO_1005);

    station_arp extreme{};
    extreme.ecef_x = MIN_UNITS;
    extreme.ecef_y = MAX_UNITS;
    extreme.ecef_z = MAX_UNITS;
    expect_frame(extreme, EXTREME_X_Y_Z);
}

TEST(RtcmMessageStationArp, SetPositionRoundsToNearestUnit)
{
    struct rounding_case
    {
        double meters;
        int64_t units;
    };
    const rounding_case cases[] = {
        {0.0, 0},
        {0.00004, 0},
        {0.00005, 1}, // exact tie: away from zero
        {0.00006, 1},
        {0.0001, 1},
        {-0.00004, 0},
        {-0.00005, -1}, // exact tie: away from zero
        {-0.00006, -1},
        {-0.0001, -1},
        {1.2345, 12345},
        {-1.2345, -12345},
        {4027894.0060, 40278940060},
        {-4027894.0060, -40278940060},
        {307045.6, 3070456000},
        {4919474.91, 49194749100},
        {13743895.3471, MAX_UNITS},
        {-13743895.3472, MIN_UNITS},
    };

    for (const rounding_case& item : cases)
    {
        station_arp message{};
        message.set_position_m(item.meters, item.meters, item.meters);
        EXPECT_EQ(message.ecef_x, item.units) << item.meters;
        EXPECT_EQ(message.ecef_y, item.units) << item.meters;
        EXPECT_EQ(message.ecef_z, item.units) << item.meters;
    }

    station_arp mixed{};
    mixed.set_position_m(0.0001, -0.0002, 12.3456);
    EXPECT_EQ(mixed.ecef_x, 1);
    EXPECT_EQ(mixed.ecef_y, -2);
    EXPECT_EQ(mixed.ecef_z, 123456);
}

TEST(RtcmMessageStationArp, RoundedPositionEncodesExpectedFrame)
{
    station_arp message{};
    message.set_position_m(0.00005, -0.00005, 1.2345);
    expect_frame(message, ROUNDED_1005);
}

TEST(RtcmMessageStationArp, CoordinatesRoundTripThroughMetres)
{
    const double x = 4027894.0060;
    const double y = -307045.6000;
    const double z = 4919474.9100;

    station_arp message{};
    message.set_position_m(x, y, z);
    EXPECT_NEAR(message.x_m(), x, 5.0e-5);
    EXPECT_NEAR(message.y_m(), y, 5.0e-5);
    EXPECT_NEAR(message.z_m(), z, 5.0e-5);

    // Converting back and forth is stable: the stored units survive a second round trip.
    station_arp again{};
    again.set_position_m(message.x_m(), message.y_m(), message.z_m());
    EXPECT_EQ(again.ecef_x, message.ecef_x);
    EXPECT_EQ(again.ecef_y, message.ecef_y);
    EXPECT_EQ(again.ecef_z, message.ecef_z);

    message.ecef_x = MAX_UNITS;
    EXPECT_NEAR(message.x_m(), 13743895.3471, 1.0e-6);
    message.ecef_y = MIN_UNITS;
    EXPECT_NEAR(message.y_m(), -13743895.3472, 1.0e-6);
    message.antenna_height = 65535U;
    EXPECT_NEAR(message.antenna_height_m(), 6.5535, 1.0e-9);
}

TEST(RtcmMessageStationArp, PositionFarOutsideRangeIsReportedAtEncode)
{
    const double hopeless[] = {13743895.3472, -13743895.3473, 1.0e12, -1.0e12, 1.0e30, -1.0e30,
                               1.0e300, -1.0e300, 1.0e308 * 10.0, -1.0e308 * 10.0};
    for (const double meters : hopeless)
    {
        for (castle::size_type axis = 0U; axis < 3U; ++axis)
        {
            station_arp message{};
            message.set_position_m(axis == 0U ? meters : 0.0, axis == 1U ? meters : 0.0, axis == 2U ? meters : 0.0);

            frame_buffer frame;
            frame.size = 99U;
            EXPECT_EQ(encode(message, frame), castle::status::out_of_range) << meters << " axis " << axis;
            EXPECT_EQ(frame.size, 0U);
        }
    }

    // Saturation keeps the sign and never wraps into the valid range.
    station_arp message{};
    message.set_position_m(1.0e30, -1.0e30, 1.0e300);
    EXPECT_GT(message.ecef_x, MAX_UNITS);
    EXPECT_LT(message.ecef_y, MIN_UNITS);
    EXPECT_GT(message.ecef_z, MAX_UNITS);
}

TEST(RtcmMessageStationArp, NegativePositionsEncodeTwosComplement)
{
    station_arp_height message{};
    message.station_id = 1U;
    message.itrf_year = 2U;
    message.glonass = true;
    message.quarter_cycle_indicator = 1U;
    message.set_position_m(-4027894.0060, -307045.6000, -4919474.9100);
    message.set_antenna_height_m(0.0001);
    EXPECT_EQ(message.antenna_height, 1U);
    expect_frame(message, NEGATIVE_1006);
}

TEST(RtcmMessageStationArp, AntennaHeightConversionAndClamp)
{
    struct height_case
    {
        double meters;
        uint16_t units;
    };
    const height_case cases[] = {
        {0.0, 0U},
        {0.00004, 0U},
        {0.00005, 1U},
        {0.0001, 1U},
        {1.5, 15000U},
        {6.5534, 65534U},
        {6.5535, 65535U},
        {6.5536, 65535U},   // one unit over the 16-bit range clamps
        {100.0, 65535U},
        {1.0e30, 65535U},
        {1.0e300 * 1.0e300, 65535U}, // +infinity
        {-0.00004, 0U},
        {-0.0001, 0U},     // negative heights clamp to zero
        {-1.5, 0U},
        {-1.0e30, 0U},
    };

    for (const height_case& item : cases)
    {
        station_arp_height message{};
        message.set_antenna_height_m(item.meters);
        EXPECT_EQ(message.antenna_height, item.units) << item.meters;
    }

    station_arp_height message{};
    message.set_antenna_height_m(1.5);
    EXPECT_NEAR(message.antenna_height_m(), 1.5, 1.0e-9);
    message.set_antenna_height_m(-3.0);
    EXPECT_DOUBLE_EQ(message.antenna_height_m(), 0.0);
}

TEST(RtcmMessageStationArp, NanInputBecomesZero)
{
    volatile double zero = 0.0;
    const double not_a_number = zero / zero;

    station_arp_height message{};
    message.set_position_m(1.0, 2.0, 3.0);
    message.set_antenna_height_m(1.5);

    message.set_position_m(not_a_number, 2.0, not_a_number);
    EXPECT_EQ(message.ecef_x, 0);
    EXPECT_EQ(message.ecef_y, 20000);
    EXPECT_EQ(message.ecef_z, 0);

    message.set_antenna_height_m(not_a_number);
    EXPECT_EQ(message.antenna_height, 0U);
}

TEST(RtcmMessageStationArp, FrameSizeAndBufferLimits)
{
    const station_arp message = golden_message<station_arp>();
    uint8_t buffer[64U] = {};
    castle::size_type written = 99U;

    // 1005: 19 byte payload -> 25 byte frame.
    ASSERT_EQ(encode_message(message, buffer, frame_size(PAYLOAD_1005), written), castle::status::ok);
    EXPECT_EQ(written, 25U);
    EXPECT_EQ(buffer[0], RTCM_PREAMBLE);
    EXPECT_EQ(buffer[2], PAYLOAD_1005);

    written = 99U;
    EXPECT_EQ(encode_message(message, buffer, frame_size(PAYLOAD_1005) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    // 1006: 21 byte payload -> 27 byte frame.
    const station_arp_height with_height = golden_message<station_arp_height>();
    ASSERT_EQ(encode_message(with_height, buffer, frame_size(PAYLOAD_1006), written), castle::status::ok);
    EXPECT_EQ(written, 27U);
    EXPECT_EQ(buffer[2], PAYLOAD_1006);

    written = 99U;
    EXPECT_EQ(encode_message(with_height, buffer, frame_size(PAYLOAD_1006) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    // A 1005 frame size is one byte short for 1006 only.
    written = 99U;
    EXPECT_EQ(encode_message(with_height, buffer, frame_size(PAYLOAD_1005), written), castle::status::full);
    EXPECT_EQ(written, 0U);

    EXPECT_EQ(encode_message(with_height, buffer, RTCM_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(encode_message(with_height, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(encode_message(with_height, nullptr, sizeof(buffer), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

} // namespace
