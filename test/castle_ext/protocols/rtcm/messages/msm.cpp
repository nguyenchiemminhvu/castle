#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/msm.hpp"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <string>

namespace
{
using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

using m4 = gps_msm4;
using m5 = gps_msm5;
using m7 = gps_msm7;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** @brief Result of encoding one message into a maximum-size frame buffer. */
struct encoded
{
    castle::status result = castle::status::ok;
    castle::size_type size = 0U;
    uint8_t bytes[RTCM_MAX_FRAME_SIZE] = {};
};

template <typename Message>
encoded encode(const Message& message)
{
    encoded out;
    out.result = encode_message(message, out.bytes, sizeof(out.bytes), out.size);
    return out;
}

std::string hex_of(const uint8_t* data, castle::size_type size)
{
    static const char digits[] = "0123456789abcdef";
    std::string text;
    for (castle::size_type index = 0U; index < size; ++index)
    {
        text.push_back(digits[data[index] >> 4U]);
        text.push_back(digits[data[index] & 0x0FU]);
    }
    return text;
}

/** @brief Reads `width` (<= 64) bits MSB-first at bit offset `position` of the payload. */
uint64_t bits_at(const encoded& frame, castle::size_type position, castle::size_type width)
{
    uint64_t value = 0U;
    for (castle::size_type index = 0U; index < width; ++index)
    {
        const castle::size_type bit = position + index;
        const uint8_t byte = frame.bytes[RTCM_FRAME_INDEX_PAYLOAD + (bit / 8U)];
        value = (value << 1U) | static_cast<uint64_t>((byte >> (7U - (bit % 8U))) & 1U);
    }
    return value;
}

/** @brief Payload length from the frame header. */
castle::size_type payload_length_of(const encoded& frame)
{
    return (static_cast<castle::size_type>(frame.bytes[1U] & 0x03U) << 8U) | frame.bytes[2U];
}

/** @brief One satellite, one signal, one cell: the smallest non-empty message. */
template <typename Message>
Message minimal()
{
    Message message{};
    message.satellite_mask = Message::satellite_bit(1U);
    message.signal_mask = Message::signal_bit(1U);
    message.cell_mask = Message::cell_bit(0U, 0U, 1U, 1U);
    static_cast<void>(message.satellites.push_back(msm_satellite{}));
    static_cast<void>(message.signals.push_back(msm_signal{}));
    return message;
}

/** @brief Bit offsets of every field inside the payload of `minimal<Message>()`. */
template <typename Message>
struct minimal_layout
{
    static CASTLE_CONSTEXPR castle::size_type STATION = 12U;
    static CASTLE_CONSTEXPR castle::size_type EPOCH = 24U;
    static CASTLE_CONSTEXPR castle::size_type MULTIPLE = 54U;
    static CASTLE_CONSTEXPR castle::size_type IODS = 55U;
    static CASTLE_CONSTEXPR castle::size_type RESERVED = 58U;
    static CASTLE_CONSTEXPR castle::size_type CLOCK_STEERING = 65U;
    static CASTLE_CONSTEXPR castle::size_type EXTERNAL_CLOCK = 67U;
    static CASTLE_CONSTEXPR castle::size_type DIVERGENCE_FREE = 69U;
    static CASTLE_CONSTEXPR castle::size_type SMOOTHING = 70U;
    static CASTLE_CONSTEXPR castle::size_type SATELLITE_MASK = 73U;
    static CASTLE_CONSTEXPR castle::size_type SIGNAL_MASK = 137U;
    static CASTLE_CONSTEXPR castle::size_type CELL_MASK = 169U;
    static CASTLE_CONSTEXPR castle::size_type ROUGH_RANGE_MS = 170U;
    static CASTLE_CONSTEXPR castle::size_type EXTENDED_INFO = 178U;
    static CASTLE_CONSTEXPR castle::size_type ROUGH_RANGE_MOD = Message::HAS_RATE ? 182U : 178U;
    static CASTLE_CONSTEXPR castle::size_type ROUGH_RATE = 192U;
    static CASTLE_CONSTEXPR castle::size_type PSEUDORANGE = Message::HAS_RATE ? 206U : 188U;
    static CASTLE_CONSTEXPR castle::size_type PHASE_RANGE = PSEUDORANGE + Message::PSEUDORANGE_BITS;
    static CASTLE_CONSTEXPR castle::size_type LOCK_TIME = PHASE_RANGE + Message::PHASE_RANGE_BITS;
    static CASTLE_CONSTEXPR castle::size_type HALF_CYCLE = LOCK_TIME + Message::LOCK_TIME_BITS;
    static CASTLE_CONSTEXPR castle::size_type CNR = HALF_CYCLE + 1U;
    static CASTLE_CONSTEXPR castle::size_type FINE_RATE = CNR + Message::CNR_BITS;
    static CASTLE_CONSTEXPR castle::size_type END = Message::HAS_RATE ? FINE_RATE + 15U : FINE_RATE;
};

uint64_t low_bits(int64_t value, castle::size_type width)
{
    return static_cast<uint64_t>(value) & ((static_cast<uint64_t>(1) << width) - 1U);
}

/**
 * @brief Checks the valid extremes of one field and the first invalid value beyond each.
 *
 * The valid values must encode and appear at `position`; the next value outside the
 * field's range must latch `out_of_range` and produce no frame.
 */
template <typename Message, typename Mutate>
void expect_field_limits(Mutate mutate,
                         castle::size_type position,
                         castle::size_type width,
                         int64_t lowest,
                         int64_t highest)
{
    Message message = minimal<Message>();
    for (int64_t value : {highest, lowest})
    {
        mutate(message, value);
        const encoded frame = encode(message);
        ASSERT_EQ(frame.result, castle::status::ok) << "value " << value << " at bit " << position;
        EXPECT_EQ(bits_at(frame, position, width), low_bits(value, width)) << "value " << value;
    }

    for (int64_t value : {highest + 1, lowest - 1})
    {
        if ((value < 0) && (lowest == 0))
        {
            continue; // unsigned field: nothing below zero to test
        }
        mutate(message, value);
        const encoded frame = encode(message);
        EXPECT_EQ(frame.result, castle::status::out_of_range) << "value " << value << " at bit " << position;
        EXPECT_EQ(frame.size, 0U);
    }
}

// ---------------------------------------------------------------------------
// Golden reference epoch (the same inputs were encoded by an independent run and
// the frames decoded and CRC-checked with the pyrtcm library)
// ---------------------------------------------------------------------------

template <typename Message>
Message reference_epoch(bool glonass_epoch)
{
    Message message{};
    message.station_id = 1234U;
    message.epoch_time = 123456789U;
    message.multiple_message = true;
    message.iods = 5U;
    message.clock_steering = 2U;
    message.external_clock = 1U;
    message.divergence_free = true;
    message.smoothing_interval = 6U;
    if (glonass_epoch)
    {
        message.set_glonass_epoch(3U, 12345678U);
    }
    message.satellite_mask = Message::satellite_bit(3U) | Message::satellite_bit(17U) | Message::satellite_bit(30U);
    message.signal_mask = Message::signal_bit(2U) | Message::signal_bit(16U);
    // 3 satellites x 2 signals: satellite 0 on both, satellite 1 on signal 0, satellite 2 on signal 1.
    message.cell_mask = Message::cell_bit(0U, 0U, 3U, 2U) | Message::cell_bit(0U, 1U, 3U, 2U)
                      | Message::cell_bit(1U, 0U, 3U, 2U) | Message::cell_bit(2U, 1U, 3U, 2U);

    const double range_ms[3] = {70.123456, 75.5, 80.25};
    for (unsigned index = 0U; index < 3U; ++index)
    {
        msm_satellite sat{};
        if constexpr (Message::HAS_RATE)
        {
            Message::set_rough_range(sat, range_ms[index], -123.4 + index * 50.0);
        }
        else
        {
            Message::set_rough_range(sat, range_ms[index]);
        }
        sat.extended_info = static_cast<uint8_t>(7U + index);
        static_cast<void>(message.satellites.push_back(sat));
    }

    const unsigned sat_of_cell[4] = {0U, 0U, 1U, 2U};
    for (unsigned cell_index = 0U; cell_index < 4U; ++cell_index)
    {
        const msm_satellite& sat = message.satellites[sat_of_cell[cell_index]];
        const double range = range_ms[sat_of_cell[cell_index]] + 0.0001 * (cell_index + 1U);
        msm_signal cell{};
        Message::set_pseudorange(sat, cell, range);
        Message::set_phase_range(sat, cell, range + 0.00002);
        cell.lock_time = static_cast<uint16_t>(3U + cell_index);
        cell.half_cycle = ((cell_index & 1U) != 0U);
        Message::set_cnr(cell, 40.5 + cell_index);
        if constexpr (Message::HAS_RATE)
        {
            Message::set_phase_range_rate(sat, cell, -123.4 + sat_of_cell[cell_index] * 50.0 + 0.0123);
        }
        static_cast<void>(message.signals.push_back(cell));
    }
    return message;
}

template <typename Message>
void expect_golden(uint16_t type, bool glonass_epoch, const char* expected_hex)
{
    const Message message = reference_epoch<Message>(glonass_epoch);
    EXPECT_EQ(Message::MESSAGE_TYPE, type);
    EXPECT_EQ(message.satellites.size(), 3U);
    EXPECT_EQ(message.signals.size(), 4U);
    EXPECT_EQ(message.cell_mask, 0x39U); // 0b111001: cells (0,0) (0,1) (1,0) (2,1)

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(hex_of(frame.bytes, frame.size), expected_hex);
    EXPECT_EQ(frame.size, strlen(expected_hex) / 2U);
    EXPECT_EQ(frame.bytes[0U], RTCM_PREAMBLE);
    EXPECT_EQ(payload_length_of(frame) + RTCM_FRAME_OVERHEAD, frame.size);
    EXPECT_EQ(bits_at(frame, 0U, 12U), type);
}

const char* const golden_1074 =
    "d300354324d21d6f3457404f100040020000000020008000728c96a03f400802"
    "15e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d355760d6bd74";
const char* const golden_1075 =
    "d300434334d21d6f3457404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36"
    "f86df0dbe1b615d7b2";
const char* const golden_1077 =
    "d3004c4354d21d6f3457404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e0180802"
    "80cb4453154571c36f86df0dbe1b60aba972";
const char* const golden_1084 =
    "d3003543c4d262f1853b404f100040020000000020008000728c96a03f400802"
    "15e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d3557604f10cf";
const char* const golden_1085 =
    "d3004343d4d262f1853b404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36"
    "f86df0dbe1b61b8091";
const char* const golden_1087 =
    "d3004c43f4d262f1853b404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e0180802"
    "80cb4453154571c36f86df0dbe1b6034af55";
const char* const golden_1094 =
    "d300354464d21d6f3457404f100040020000000020008000728c96a03f400802"
    "15e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d355760ed174a";
const char* const golden_1095 =
    "d300434474d21d6f3457404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36"
    "f86df0dbe1b6fb0524";
const char* const golden_1097 =
    "d3004c4494d21d6f3457404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e0180802"
    "80cb4453154571c36f86df0dbe1b609fa849";
const char* const golden_1124 =
    "d300354644d21d6f3457404f100040020000000020008000728c96a03f400802"
    "15e4fd64ea4d1b88ab50293af853e2e1b8671a2b2d35576069f607";
const char* const golden_1125 =
    "d300434654d21d6f3457404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd285793f593a9346e22ad40a4ebe14f8b86e19c68acb4d55d9c36"
    "f86df0dbe1b68406ba";
const char* const golden_1127 =
    "d3004c4674d21d6f3457404f100040020000000020008000728c96a0f123f400"
    "807f0bfdbffd28576c9fada4ea4a68db822ad3e293af814f8b61b866e0180802"
    "80cb4453154571c36f86df0dbe1b603776f3";

// ---------------------------------------------------------------------------
// Message numbers and level constants
// ---------------------------------------------------------------------------

TEST(RtcmMsm, SystemEnumValuesAreMsm1NumbersMinusOne)
{
    EXPECT_EQ(static_cast<uint16_t>(msm_system::gps), 1070U);
    EXPECT_EQ(static_cast<uint16_t>(msm_system::glonass), 1080U);
    EXPECT_EQ(static_cast<uint16_t>(msm_system::galileo), 1090U);
    EXPECT_EQ(static_cast<uint16_t>(msm_system::sbas), 1100U);
    EXPECT_EQ(static_cast<uint16_t>(msm_system::qzss), 1110U);
    EXPECT_EQ(static_cast<uint16_t>(msm_system::beidou), 1120U);
}

TEST(RtcmMsm, MessageTypeOfEveryAlias)
{
    EXPECT_EQ(gps_msm4::MESSAGE_TYPE, 1074U);
    EXPECT_EQ(gps_msm5::MESSAGE_TYPE, 1075U);
    EXPECT_EQ(gps_msm7::MESSAGE_TYPE, 1077U);
    EXPECT_EQ(glonass_msm4::MESSAGE_TYPE, 1084U);
    EXPECT_EQ(glonass_msm5::MESSAGE_TYPE, 1085U);
    EXPECT_EQ(glonass_msm7::MESSAGE_TYPE, 1087U);
    EXPECT_EQ(galileo_msm4::MESSAGE_TYPE, 1094U);
    EXPECT_EQ(galileo_msm5::MESSAGE_TYPE, 1095U);
    EXPECT_EQ(galileo_msm7::MESSAGE_TYPE, 1097U);
    EXPECT_EQ(beidou_msm4::MESSAGE_TYPE, 1124U);
    EXPECT_EQ(beidou_msm5::MESSAGE_TYPE, 1125U);
    EXPECT_EQ(beidou_msm7::MESSAGE_TYPE, 1127U);
}

TEST(RtcmMsm, MessageTypeOfSystemsWithoutAlias)
{
    EXPECT_EQ((msm_message<msm_system::sbas, 4U>::MESSAGE_TYPE), 1104U);
    EXPECT_EQ((msm_message<msm_system::qzss, 5U>::MESSAGE_TYPE), 1115U);
    EXPECT_EQ((msm_message<msm_system::qzss, 7U>::MESSAGE_TYPE), 1117U);

    msm_message<msm_system::qzss, 4U> qzss = minimal<msm_message<msm_system::qzss, 4U>>();
    const encoded frame = encode(qzss);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, 0U, 12U), 1114U);
}

TEST(RtcmMsm, AcceptsOnlyOwnMessageType)
{
    EXPECT_TRUE(gps_msm4::accepts(1074U));
    EXPECT_FALSE(gps_msm4::accepts(1075U));
    EXPECT_FALSE(gps_msm4::accepts(1077U));
    EXPECT_FALSE(gps_msm4::accepts(1084U));
    EXPECT_FALSE(gps_msm4::accepts(0U));
    EXPECT_TRUE(gps_msm7::accepts(1077U));
    EXPECT_FALSE(gps_msm7::accepts(1074U));
    EXPECT_TRUE(beidou_msm5::accepts(1125U));
    EXPECT_FALSE(beidou_msm5::accepts(1095U));
    EXPECT_TRUE(galileo_msm7::accepts(1097U));
    EXPECT_TRUE(glonass_msm4::accepts(1084U));
}

TEST(RtcmMsm, LevelDependentConstants)
{
    EXPECT_EQ(m4::MAX_CELLS, 64U);

    EXPECT_FALSE(m4::HAS_RATE);
    EXPECT_TRUE(m5::HAS_RATE);
    EXPECT_TRUE(m7::HAS_RATE);

    EXPECT_EQ(m4::PSEUDORANGE_BITS, 15U);
    EXPECT_EQ(m5::PSEUDORANGE_BITS, 15U);
    EXPECT_EQ(m7::PSEUDORANGE_BITS, 20U);
    EXPECT_EQ(m4::PHASE_RANGE_BITS, 22U);
    EXPECT_EQ(m5::PHASE_RANGE_BITS, 22U);
    EXPECT_EQ(m7::PHASE_RANGE_BITS, 24U);
    EXPECT_EQ(m4::LOCK_TIME_BITS, 4U);
    EXPECT_EQ(m5::LOCK_TIME_BITS, 4U);
    EXPECT_EQ(m7::LOCK_TIME_BITS, 10U);
    EXPECT_EQ(m4::CNR_BITS, 6U);
    EXPECT_EQ(m5::CNR_BITS, 6U);
    EXPECT_EQ(m7::CNR_BITS, 10U);

    EXPECT_EQ(m4::PSEUDORANGE_STEPS_PER_MS, 16777216.0);       // 2^24
    EXPECT_EQ(m5::PSEUDORANGE_STEPS_PER_MS, 16777216.0);
    EXPECT_EQ(m7::PSEUDORANGE_STEPS_PER_MS, 536870912.0);      // 2^29
    EXPECT_EQ(m4::PHASE_RANGE_STEPS_PER_MS, 536870912.0);      // 2^29
    EXPECT_EQ(m5::PHASE_RANGE_STEPS_PER_MS, 536870912.0);
    EXPECT_EQ(m7::PHASE_RANGE_STEPS_PER_MS, 2147483648.0);     // 2^31
    EXPECT_EQ(m4::CNR_STEPS_PER_DBHZ, 1.0);
    EXPECT_EQ(m7::CNR_STEPS_PER_DBHZ, 16.0);
    EXPECT_EQ(m4::ROUGH_RANGE_STEPS_PER_MS, 1024.0);
    EXPECT_EQ(m7::FINE_RATE_STEPS_PER_M_S, 10000.0);

    EXPECT_EQ(m4::INVALID_ROUGH_RANGE, 255U);
    EXPECT_EQ(m4::INVALID_ROUGH_RATE, -8192);
    EXPECT_EQ(m5::INVALID_FINE_RATE, -16384);
    EXPECT_EQ(m4::INVALID_FINE_PSEUDORANGE, -16384);
    EXPECT_EQ(m5::INVALID_FINE_PSEUDORANGE, -16384);
    EXPECT_EQ(m7::INVALID_FINE_PSEUDORANGE, -524288);
    EXPECT_EQ(m4::INVALID_FINE_PHASE_RANGE, -2097152);
    EXPECT_EQ(m5::INVALID_FINE_PHASE_RANGE, -2097152);
    EXPECT_EQ(m7::INVALID_FINE_PHASE_RANGE, -8388608);
}

TEST(RtcmMsm, DefaultMessageIsZeroed)
{
    const m7 message{};
    EXPECT_EQ(message.station_id, 0U);
    EXPECT_EQ(message.epoch_time, 0U);
    EXPECT_FALSE(message.multiple_message);
    EXPECT_EQ(message.iods, 0U);
    EXPECT_EQ(message.clock_steering, 0U);
    EXPECT_EQ(message.external_clock, 0U);
    EXPECT_FALSE(message.divergence_free);
    EXPECT_EQ(message.smoothing_interval, 0U);
    EXPECT_EQ(message.satellite_mask, 0U);
    EXPECT_EQ(message.signal_mask, 0U);
    EXPECT_EQ(message.cell_mask, 0U);
    EXPECT_EQ(message.satellites.size(), 0U);
    EXPECT_EQ(message.signals.size(), 0U);
    EXPECT_EQ(message.satellites.capacity(), 64U);
    EXPECT_EQ(message.signals.capacity(), 64U);

    const msm_satellite sat{};
    EXPECT_EQ(sat.rough_range_ms, 0U);
    EXPECT_EQ(sat.extended_info, 0U);
    EXPECT_EQ(sat.rough_range_mod, 0U);
    EXPECT_EQ(sat.rough_phase_range_rate, 0);

    const msm_signal cell{};
    EXPECT_EQ(cell.fine_pseudorange, 0);
    EXPECT_EQ(cell.fine_phase_range, 0);
    EXPECT_EQ(cell.lock_time, 0U);
    EXPECT_FALSE(cell.half_cycle);
    EXPECT_EQ(cell.cnr, 0U);
    EXPECT_EQ(cell.fine_phase_range_rate, 0);

    const msm_message<msm_system::gps, 4U, 5U, 7U> small{};
    EXPECT_EQ(small.satellites.capacity(), 5U);
    EXPECT_EQ(small.signals.capacity(), 7U);
}

// ---------------------------------------------------------------------------
// Mask helpers
// ---------------------------------------------------------------------------

TEST(RtcmMsm, SatelliteBitIsMsbFirst)
{
    EXPECT_EQ(m7::satellite_bit(1U), 0x8000000000000000ULL);
    EXPECT_EQ(m7::satellite_bit(2U), 0x4000000000000000ULL);
    EXPECT_EQ(m7::satellite_bit(3U), 0x2000000000000000ULL);
    EXPECT_EQ(m7::satellite_bit(32U), 0x0000000100000000ULL);
    EXPECT_EQ(m7::satellite_bit(33U), 0x0000000080000000ULL);
    EXPECT_EQ(m7::satellite_bit(63U), 0x2ULL);
    EXPECT_EQ(m7::satellite_bit(64U), 0x1ULL);
}

TEST(RtcmMsm, SatelliteBitOutOfRangeIsZero)
{
    EXPECT_EQ(m7::satellite_bit(0U), 0U);
    EXPECT_EQ(m7::satellite_bit(65U), 0U);
    EXPECT_EQ(m7::satellite_bit(128U), 0U);
    EXPECT_EQ(m7::satellite_bit(~static_cast<castle::size_type>(0U)), 0U);
}

TEST(RtcmMsm, SatelliteBitsAreDistinct)
{
    uint64_t seen = 0U;
    for (castle::size_type id = 1U; id <= 64U; ++id)
    {
        const uint64_t bit = m4::satellite_bit(id);
        EXPECT_NE(bit, 0U);
        EXPECT_EQ(seen & bit, 0U);
        seen |= bit;
    }
    EXPECT_EQ(seen, ~static_cast<uint64_t>(0U));
}

TEST(RtcmMsm, SignalBitIsMsbFirst)
{
    EXPECT_EQ(m7::signal_bit(1U), 0x80000000U);
    EXPECT_EQ(m7::signal_bit(2U), 0x40000000U);
    EXPECT_EQ(m7::signal_bit(16U), 0x00010000U);
    EXPECT_EQ(m7::signal_bit(17U), 0x00008000U);
    EXPECT_EQ(m7::signal_bit(31U), 0x2U);
    EXPECT_EQ(m7::signal_bit(32U), 0x1U);
}

TEST(RtcmMsm, SignalBitOutOfRangeIsZero)
{
    EXPECT_EQ(m7::signal_bit(0U), 0U);
    EXPECT_EQ(m7::signal_bit(33U), 0U);
    EXPECT_EQ(m7::signal_bit(64U), 0U);
    EXPECT_EQ(m7::signal_bit(~static_cast<castle::size_type>(0U)), 0U);
}

TEST(RtcmMsm, CellBitIsSatelliteMajorMsbFirst)
{
    // 3 satellites x 2 signals = 6 bits; cell (s, g) is bit index s*2+g counted from the MSB.
    EXPECT_EQ(m7::cell_bit(0U, 0U, 3U, 2U), 1ULL << 5U);
    EXPECT_EQ(m7::cell_bit(0U, 1U, 3U, 2U), 1ULL << 4U);
    EXPECT_EQ(m7::cell_bit(1U, 0U, 3U, 2U), 1ULL << 3U);
    EXPECT_EQ(m7::cell_bit(1U, 1U, 3U, 2U), 1ULL << 2U);
    EXPECT_EQ(m7::cell_bit(2U, 0U, 3U, 2U), 1ULL << 1U);
    EXPECT_EQ(m7::cell_bit(2U, 1U, 3U, 2U), 1ULL << 0U);

    EXPECT_EQ(m7::cell_bit(0U, 0U, 1U, 1U), 1U);
    // Same cell, different shape: the bit depends on the total cell count.
    EXPECT_EQ(m7::cell_bit(0U, 0U, 2U, 1U), 1ULL << 1U);
    EXPECT_EQ(m7::cell_bit(1U, 0U, 2U, 1U), 1ULL << 0U);
    EXPECT_EQ(m7::cell_bit(0U, 1U, 1U, 2U), 1ULL << 0U);
}

TEST(RtcmMsm, CellBitAtTheSixtyFourCellLimit)
{
    EXPECT_EQ(m7::cell_bit(0U, 0U, 8U, 8U), 1ULL << 63U);
    EXPECT_EQ(m7::cell_bit(7U, 7U, 8U, 8U), 1ULL);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 64U, 1U), 1ULL << 63U);
    EXPECT_EQ(m7::cell_bit(63U, 0U, 64U, 1U), 1ULL);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 2U, 32U), 1ULL << 63U);
    EXPECT_EQ(m7::cell_bit(1U, 31U, 2U, 32U), 1ULL);
    EXPECT_EQ(m7::cell_bit(0U, 63U, 1U, 64U), 1ULL);
}

TEST(RtcmMsm, CellBitOutOfRangeIndexIsZero)
{
    EXPECT_EQ(m7::cell_bit(3U, 0U, 3U, 2U), 0U);    // satellite index == count
    EXPECT_EQ(m7::cell_bit(0U, 2U, 3U, 2U), 0U);    // signal index == count
    EXPECT_EQ(m7::cell_bit(7U, 9U, 3U, 2U), 0U);    // both beyond
    EXPECT_EQ(m7::cell_bit(0U, 0U, 0U, 0U), 0U);    // empty shape
    EXPECT_EQ(m7::cell_bit(0U, 0U, 0U, 5U), 0U);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 5U, 0U), 0U);
    EXPECT_EQ(m7::cell_bit(~static_cast<castle::size_type>(0U), 0U, 3U, 2U), 0U);
}

TEST(RtcmMsm, CellBitProductAboveSixtyFourIsZero)
{
    EXPECT_EQ(m7::cell_bit(0U, 0U, 13U, 5U), 0U);   // 65 cells
    EXPECT_EQ(m7::cell_bit(12U, 4U, 13U, 5U), 0U);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 1U, 65U), 0U);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 65U, 1U), 0U);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 33U, 2U), 0U);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 64U, 2U), 0U);
    EXPECT_EQ(m7::cell_bit(0U, 0U, 64U, 64U), 0U);
}

TEST(RtcmMsm, CellBitsOfEveryCellOrToTheFullMask)
{
    uint64_t mask = 0U;
    for (castle::size_type sat = 0U; sat < 5U; ++sat)
    {
        for (castle::size_type sig = 0U; sig < 4U; ++sig)
        {
            const uint64_t bit = m5::cell_bit(sat, sig, 5U, 4U);
            EXPECT_NE(bit, 0U);
            EXPECT_EQ(mask & bit, 0U);
            mask |= bit;
        }
    }
    EXPECT_EQ(mask, 0xFFFFFULL);
}

// ---------------------------------------------------------------------------
// set_glonass_epoch
// ---------------------------------------------------------------------------

TEST(RtcmMsm, GlonassEpochPacksDayAndTimeOfDay)
{
    glonass_msm4 message{};
    message.set_glonass_epoch(0U, 0U);
    EXPECT_EQ(message.epoch_time, 0U);

    message.set_glonass_epoch(3U, 12345678U);
    EXPECT_EQ(message.epoch_time, (3U << 27U) | 12345678U);
    EXPECT_EQ(message.epoch_time, 0x18BC614EU);

    message.set_glonass_epoch(1U, 0U);
    EXPECT_EQ(message.epoch_time, 0x08000000U);

    message.set_glonass_epoch(6U, 86399999U);
    EXPECT_EQ(message.epoch_time, (6U << 27U) | 86399999U);

    message.set_glonass_epoch(7U, 0U); // 7 = unknown day
    EXPECT_EQ(message.epoch_time, 0x38000000U);
}

TEST(RtcmMsm, GlonassEpochMasksOverflowBits)
{
    glonass_msm5 message{};

    message.set_glonass_epoch(7U, 0x07FFFFFFU);
    EXPECT_EQ(message.epoch_time, 0x3FFFFFFFU);

    // Day 8 wraps to 0; bit 3 of the day must not reach the time of day.
    message.set_glonass_epoch(8U, 0U);
    EXPECT_EQ(message.epoch_time, 0U);
    message.set_glonass_epoch(9U, 0U);
    EXPECT_EQ(message.epoch_time, 0x08000000U);
    message.set_glonass_epoch(0xFFU, 0U);
    EXPECT_EQ(message.epoch_time, 7U << 27U);

    // Time of day above 27 bits is truncated and cannot reach the day bits.
    message.set_glonass_epoch(0U, 0x08000000U);
    EXPECT_EQ(message.epoch_time, 0U);
    message.set_glonass_epoch(4U, 0x08000001U);
    EXPECT_EQ(message.epoch_time, (4U << 27U) | 1U);
    message.set_glonass_epoch(0U, 0xFFFFFFFFU);
    EXPECT_EQ(message.epoch_time, 0x07FFFFFFU);
    message.set_glonass_epoch(0xFFU, 0xFFFFFFFFU);
    EXPECT_EQ(message.epoch_time, 0x3FFFFFFFU);
}

TEST(RtcmMsm, GlonassEpochAlwaysEncodes)
{
    glonass_msm7 message = minimal<glonass_msm7>();
    message.set_glonass_epoch(0xFFU, 0xFFFFFFFFU);
    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, 24U, 30U), 0x3FFFFFFFU);
}

// ---------------------------------------------------------------------------
// set_rough_range
// ---------------------------------------------------------------------------

msm_satellite dirty_satellite()
{
    msm_satellite sat{};
    sat.rough_range_ms = 99U;
    sat.rough_range_mod = 77U;
    sat.rough_phase_range_rate = 55;
    return sat;
}

TEST(RtcmMsm, RoughRangeSplitsMillisecondsAndFraction)
{
    msm_satellite sat = dirty_satellite();

    m4::set_rough_range(sat, 0.0);
    EXPECT_EQ(sat.rough_range_ms, 0U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    m4::set_rough_range(sat, -0.0);
    EXPECT_EQ(sat.rough_range_ms, 0U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    m4::set_rough_range(sat, 70.123456);
    EXPECT_EQ(sat.rough_range_ms, 70U);
    EXPECT_EQ(sat.rough_range_mod, 126U); // round(0.123456 * 1024) = 126
    EXPECT_EQ(sat.rough_phase_range_rate, 55) << "the one-argument overload leaves the rate alone";

    m4::set_rough_range(sat, 1.5);
    EXPECT_EQ(sat.rough_range_ms, 1U);
    EXPECT_EQ(sat.rough_range_mod, 512U);

    m4::set_rough_range(sat, 254.0);
    EXPECT_EQ(sat.rough_range_ms, 254U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    m4::set_rough_range(sat, 254.9990234375); // 254 + 1023/1024
    EXPECT_EQ(sat.rough_range_ms, 254U);
    EXPECT_EQ(sat.rough_range_mod, 1023U);
}

TEST(RtcmMsm, RoughRangeRoundsToNearestStep)
{
    msm_satellite sat{};
    m7::set_rough_range(sat, 0.0004); // 0.41 steps -> 0
    EXPECT_EQ(sat.rough_range_ms, 0U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    m7::set_rough_range(sat, 0.0005); // 0.512 steps -> 1
    EXPECT_EQ(sat.rough_range_ms, 0U);
    EXPECT_EQ(sat.rough_range_mod, 1U);

    m7::set_rough_range(sat, 3.0 + 0.5 / 1024.0); // exactly half a step rounds up
    EXPECT_EQ(sat.rough_range_ms, 3U);
    EXPECT_EQ(sat.rough_range_mod, 1U);
}

TEST(RtcmMsm, RoughRangeFractionCarriesIntoMilliseconds)
{
    msm_satellite sat{};
    m4::set_rough_range(sat, 0.9990234375); // 1023/1024: stays in the fraction
    EXPECT_EQ(sat.rough_range_ms, 0U);
    EXPECT_EQ(sat.rough_range_mod, 1023U);

    m4::set_rough_range(sat, 0.99951171875); // 1023.5/1024 rounds up to 1 ms + 0
    EXPECT_EQ(sat.rough_range_ms, 1U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    m4::set_rough_range(sat, 0.9999);
    EXPECT_EQ(sat.rough_range_ms, 1U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    m4::set_rough_range(sat, 69.9999);
    EXPECT_EQ(sat.rough_range_ms, 70U);
    EXPECT_EQ(sat.rough_range_mod, 0U);
}

TEST(RtcmMsm, RoughRangeInvalidMarkerWhenOutOfRange)
{
    const double bad[] = {-1.0, -1e-12, -1000.0, 255.0, 255.5, 1e9, INFINITY, -INFINITY, NAN};
    for (double range : bad)
    {
        msm_satellite sat = dirty_satellite();
        m4::set_rough_range(sat, range);
        EXPECT_EQ(sat.rough_range_ms, m4::INVALID_ROUGH_RANGE) << range;
        EXPECT_EQ(sat.rough_range_mod, 0U) << range;
        EXPECT_EQ(sat.rough_phase_range_rate, 55) << range;
    }
}

TEST(RtcmMsm, RoughRangeRoundingUpToTwoFiftyFiveIsInvalid)
{
    // 254.9999 ms is 261119.9 steps: it rounds up to 255 ms + 0, which is the invalid marker.
    msm_satellite sat = dirty_satellite();
    m4::set_rough_range(sat, 254.9999);
    EXPECT_EQ(sat.rough_range_ms, 255U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    sat = dirty_satellite();
    m7::set_rough_range(sat, 254.99951171875); // 261119.5 steps rounds up
    EXPECT_EQ(sat.rough_range_ms, 255U);
    EXPECT_EQ(sat.rough_range_mod, 0U);

    // One step below stays valid.
    m7::set_rough_range(sat, 254.9990234375);
    EXPECT_EQ(sat.rough_range_ms, 254U);
    EXPECT_EQ(sat.rough_range_mod, 1023U);
}

TEST(RtcmMsm, RoughRangeWithRateQuantisesBoth)
{
    msm_satellite sat = dirty_satellite();

    m5::set_rough_range(sat, 70.5, -123.4);
    EXPECT_EQ(sat.rough_range_ms, 70U);
    EXPECT_EQ(sat.rough_range_mod, 512U);
    EXPECT_EQ(sat.rough_phase_range_rate, -123);

    m7::set_rough_range(sat, 70.5, 123.5);
    EXPECT_EQ(sat.rough_phase_range_rate, 124); // half away from zero
    m7::set_rough_range(sat, 70.5, -123.5);
    EXPECT_EQ(sat.rough_phase_range_rate, -124);
    m7::set_rough_range(sat, 70.5, 123.49);
    EXPECT_EQ(sat.rough_phase_range_rate, 123);
    m7::set_rough_range(sat, 70.5, -123.49);
    EXPECT_EQ(sat.rough_phase_range_rate, -123);
    m7::set_rough_range(sat, 70.5, 0.0);
    EXPECT_EQ(sat.rough_phase_range_rate, 0);
    m7::set_rough_range(sat, 70.5, 0.4);
    EXPECT_EQ(sat.rough_phase_range_rate, 0);
    m7::set_rough_range(sat, 70.5, -0.4);
    EXPECT_EQ(sat.rough_phase_range_rate, 0);
}

TEST(RtcmMsm, RoughRateSaturatesAtFieldExtremes)
{
    msm_satellite sat{};

    m7::set_rough_range(sat, 1.0, 8191.0);
    EXPECT_EQ(sat.rough_phase_range_rate, 8191);
    m7::set_rough_range(sat, 1.0, -8191.0);
    EXPECT_EQ(sat.rough_phase_range_rate, -8191);
    m7::set_rough_range(sat, 1.0, 8191.49);
    EXPECT_EQ(sat.rough_phase_range_rate, 8191);
    m7::set_rough_range(sat, 1.0, -8191.49);
    EXPECT_EQ(sat.rough_phase_range_rate, -8191);

    // +-8191.5 would round onto 8192 / -8192, so both become the invalid marker.
    m7::set_rough_range(sat, 1.0, 8191.5);
    EXPECT_EQ(sat.rough_phase_range_rate, m7::INVALID_ROUGH_RATE);
    m7::set_rough_range(sat, 1.0, -8191.5);
    EXPECT_EQ(sat.rough_phase_range_rate, m7::INVALID_ROUGH_RATE);
    m7::set_rough_range(sat, 1.0, 9000.0);
    EXPECT_EQ(sat.rough_phase_range_rate, m7::INVALID_ROUGH_RATE);
    m7::set_rough_range(sat, 1.0, -1e9);
    EXPECT_EQ(sat.rough_phase_range_rate, m7::INVALID_ROUGH_RATE);
    m7::set_rough_range(sat, 1.0, INFINITY);
    EXPECT_EQ(sat.rough_phase_range_rate, m7::INVALID_ROUGH_RATE);
    m7::set_rough_range(sat, 1.0, NAN);
    EXPECT_EQ(sat.rough_phase_range_rate, m7::INVALID_ROUGH_RATE);
}

TEST(RtcmMsm, RoughRangeAndRateAreIndependentlyInvalid)
{
    msm_satellite sat{};
    m5::set_rough_range(sat, -5.0, 100.0);
    EXPECT_EQ(sat.rough_range_ms, m5::INVALID_ROUGH_RANGE);
    EXPECT_EQ(sat.rough_range_mod, 0U);
    EXPECT_EQ(sat.rough_phase_range_rate, 100);

    m5::set_rough_range(sat, 5.0, NAN);
    EXPECT_EQ(sat.rough_range_ms, 5U);
    EXPECT_EQ(sat.rough_phase_range_rate, m5::INVALID_ROUGH_RATE);
}

// ---------------------------------------------------------------------------
// Fine pseudorange / phase-range / phase-range-rate quantisation
// ---------------------------------------------------------------------------

/** @brief Satellite whose quantised rough range is exactly 70.5 ms, rough rate -123 m/s. */
msm_satellite rough_70_5()
{
    msm_satellite sat{};
    sat.rough_range_ms = 70U;
    sat.rough_range_mod = 512U;
    sat.rough_phase_range_rate = -123;
    return sat;
}

/** @brief Fine pseudorange for a full range that sits `steps` LSBs above the rough range. */
template <typename Message>
int32_t pseudorange_for_steps(double steps)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};
    cell.fine_pseudorange = 12345; // proves the field is always overwritten
    Message::set_pseudorange(sat, cell, 70.5 + steps / Message::PSEUDORANGE_STEPS_PER_MS);
    return cell.fine_pseudorange;
}

template <typename Message>
int32_t phase_range_for_steps(double steps)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};
    cell.fine_phase_range = 12345;
    Message::set_phase_range(sat, cell, 70.5 + steps / Message::PHASE_RANGE_STEPS_PER_MS);
    return cell.fine_phase_range;
}

TEST(RtcmMsm, PseudorangeLsbIsTwoPowMinus24MsForMsm4AndMsm5)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};

    m4::set_pseudorange(sat, cell, 70.5);
    EXPECT_EQ(cell.fine_pseudorange, 0);
    m4::set_pseudorange(sat, cell, 70.5 + 1.0 / 16777216.0); // exactly 2^-24 ms
    EXPECT_EQ(cell.fine_pseudorange, 1);
    m4::set_pseudorange(sat, cell, 70.5 - 1.0 / 16777216.0);
    EXPECT_EQ(cell.fine_pseudorange, -1);
    m5::set_pseudorange(sat, cell, 70.5 + 5.0 / 16777216.0);
    EXPECT_EQ(cell.fine_pseudorange, 5);
}

TEST(RtcmMsm, PseudorangeLsbIsTwoPowMinus29MsForMsm7)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};

    m7::set_pseudorange(sat, cell, 70.5 + 1.0 / 536870912.0); // exactly 2^-29 ms
    EXPECT_EQ(cell.fine_pseudorange, 1);
    m7::set_pseudorange(sat, cell, 70.5 - 1.0 / 536870912.0);
    EXPECT_EQ(cell.fine_pseudorange, -1);

    // The same physical value is 32 times coarser in MSM4: 2^-29 ms is 1/32 of an MSM4 LSB.
    m4::set_pseudorange(sat, cell, 70.5 + 1.0 / 536870912.0);
    EXPECT_EQ(cell.fine_pseudorange, 0);
    m7::set_pseudorange(sat, cell, 70.5 + 32.0 / 536870912.0);
    EXPECT_EQ(cell.fine_pseudorange, 32);
    m4::set_pseudorange(sat, cell, 70.5 + 32.0 / 536870912.0);
    EXPECT_EQ(cell.fine_pseudorange, 1);
}

TEST(RtcmMsm, PhaseRangeLsbIsTwoPowMinus29MsForMsm4AndMsm5)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};

    m4::set_phase_range(sat, cell, 70.5);
    EXPECT_EQ(cell.fine_phase_range, 0);
    m4::set_phase_range(sat, cell, 70.5 + 1.0 / 536870912.0); // 2^-29 ms
    EXPECT_EQ(cell.fine_phase_range, 1);
    m4::set_phase_range(sat, cell, 70.5 - 1.0 / 536870912.0);
    EXPECT_EQ(cell.fine_phase_range, -1);
    m5::set_phase_range(sat, cell, 70.5 + 7.0 / 536870912.0);
    EXPECT_EQ(cell.fine_phase_range, 7);
}

TEST(RtcmMsm, PhaseRangeLsbIsTwoPowMinus31MsForMsm7)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};

    m7::set_phase_range(sat, cell, 70.5 + 1.0 / 2147483648.0); // 2^-31 ms
    EXPECT_EQ(cell.fine_phase_range, 1);
    m7::set_phase_range(sat, cell, 70.5 - 1.0 / 2147483648.0);
    EXPECT_EQ(cell.fine_phase_range, -1);

    m4::set_phase_range(sat, cell, 70.5 + 1.0 / 2147483648.0); // a quarter of an MSM4 LSB
    EXPECT_EQ(cell.fine_phase_range, 0);
    m4::set_phase_range(sat, cell, 70.5 + 4.0 / 2147483648.0);
    EXPECT_EQ(cell.fine_phase_range, 1);
    m7::set_phase_range(sat, cell, 70.5 + 4.0 / 2147483648.0);
    EXPECT_EQ(cell.fine_phase_range, 4);
}

TEST(RtcmMsm, FineValuesAreRelativeToTheQuantisedRoughRange)
{
    // The rough range 70.123456 quantises to 70 + 126/1024; the fine value carries the rest.
    msm_satellite sat{};
    m7::set_rough_range(sat, 70.123456);
    msm_signal cell{};

    m4::set_pseudorange(sat, cell, 70.123456);
    EXPECT_EQ(cell.fine_pseudorange, 6864);   // 0.000409125 ms * 2^24 = 6863.98
    m7::set_pseudorange(sat, cell, 70.123456);
    EXPECT_EQ(cell.fine_pseudorange, 219647); // 0.000409125 ms * 2^29 = 219647.31
    m4::set_phase_range(sat, cell, 70.123456);
    EXPECT_EQ(cell.fine_phase_range, 219647);
    m7::set_phase_range(sat, cell, 70.123456);
    EXPECT_EQ(cell.fine_phase_range, 878589); // 0.000409125 ms * 2^31 = 878589.25
}

TEST(RtcmMsm, FineValuesRoundHalfAwayFromZero)
{
    EXPECT_EQ(pseudorange_for_steps<m4>(0.49), 0);
    EXPECT_EQ(pseudorange_for_steps<m4>(0.5), 1);
    EXPECT_EQ(pseudorange_for_steps<m4>(-0.49), 0);
    EXPECT_EQ(pseudorange_for_steps<m4>(-0.5), -1);
    EXPECT_EQ(pseudorange_for_steps<m4>(1.5), 2);
    EXPECT_EQ(pseudorange_for_steps<m4>(-1.5), -2);
    EXPECT_EQ(pseudorange_for_steps<m7>(2.5), 3);
    EXPECT_EQ(pseudorange_for_steps<m7>(-2.5), -3);
    EXPECT_EQ(phase_range_for_steps<m4>(0.5), 1);
    EXPECT_EQ(phase_range_for_steps<m4>(-0.5), -1);
    EXPECT_EQ(phase_range_for_steps<m7>(1.5), 2);
    EXPECT_EQ(phase_range_for_steps<m7>(-1.5), -2);
}

TEST(RtcmMsm, PseudorangeBoundariesMsm4)
{
    const int32_t invalid = m4::INVALID_FINE_PSEUDORANGE;
    EXPECT_EQ(pseudorange_for_steps<m4>(16383.0), 16383);
    EXPECT_EQ(pseudorange_for_steps<m4>(16383.4), 16383);
    EXPECT_EQ(pseudorange_for_steps<m4>(16383.5), invalid); // rounds onto +2^14
    EXPECT_EQ(pseudorange_for_steps<m4>(16384.0), invalid);
    EXPECT_EQ(pseudorange_for_steps<m4>(40000.0), invalid);
    EXPECT_EQ(pseudorange_for_steps<m4>(-16383.0), -16383);
    EXPECT_EQ(pseudorange_for_steps<m4>(-16383.4), -16383);
    EXPECT_EQ(pseudorange_for_steps<m4>(-16383.5), invalid); // rounds onto -2^14 == the marker
    EXPECT_EQ(pseudorange_for_steps<m4>(-16384.0), invalid);
    EXPECT_EQ(pseudorange_for_steps<m4>(-40000.0), invalid);
    EXPECT_EQ(invalid, -16384);
}

TEST(RtcmMsm, PseudorangeBoundariesMsm7)
{
    const int32_t invalid = m7::INVALID_FINE_PSEUDORANGE;
    EXPECT_EQ(pseudorange_for_steps<m7>(524287.0), 524287);
    EXPECT_EQ(pseudorange_for_steps<m7>(524287.4), 524287);
    EXPECT_EQ(pseudorange_for_steps<m7>(524287.5), invalid);
    EXPECT_EQ(pseudorange_for_steps<m7>(524288.0), invalid);
    EXPECT_EQ(pseudorange_for_steps<m7>(-524287.0), -524287);
    EXPECT_EQ(pseudorange_for_steps<m7>(-524287.5), invalid);
    EXPECT_EQ(pseudorange_for_steps<m7>(-524288.0), invalid);
    EXPECT_EQ(pseudorange_for_steps<m7>(-1e7), invalid);
    EXPECT_EQ(invalid, -524288);
}

TEST(RtcmMsm, PhaseRangeBoundariesMsm4)
{
    const int32_t invalid = m4::INVALID_FINE_PHASE_RANGE;
    EXPECT_EQ(phase_range_for_steps<m4>(2097151.0), 2097151);
    EXPECT_EQ(phase_range_for_steps<m4>(2097151.5), invalid);
    EXPECT_EQ(phase_range_for_steps<m4>(2097152.0), invalid);
    EXPECT_EQ(phase_range_for_steps<m4>(-2097151.0), -2097151);
    EXPECT_EQ(phase_range_for_steps<m4>(-2097151.5), invalid);
    EXPECT_EQ(phase_range_for_steps<m4>(-2097152.0), invalid);
    EXPECT_EQ(phase_range_for_steps<m4>(1e8), invalid);
    EXPECT_EQ(invalid, -2097152);
}

TEST(RtcmMsm, PhaseRangeBoundariesMsm7)
{
    const int32_t invalid = m7::INVALID_FINE_PHASE_RANGE;
    EXPECT_EQ(phase_range_for_steps<m7>(8388607.0), 8388607);
    EXPECT_EQ(phase_range_for_steps<m7>(8388607.5), invalid);
    EXPECT_EQ(phase_range_for_steps<m7>(8388608.0), invalid);
    EXPECT_EQ(phase_range_for_steps<m7>(-8388607.0), -8388607);
    EXPECT_EQ(phase_range_for_steps<m7>(-8388607.5), invalid);
    EXPECT_EQ(phase_range_for_steps<m7>(-8388608.0), invalid);
    EXPECT_EQ(phase_range_for_steps<m7>(-1e9), invalid);
    EXPECT_EQ(invalid, -8388608);
}

TEST(RtcmMsm, FineValuesNonFiniteBecomeInvalid)
{
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};
    const double bad[] = {NAN, INFINITY, -INFINITY};
    for (double range : bad)
    {
        m4::set_pseudorange(sat, cell, range);
        EXPECT_EQ(cell.fine_pseudorange, m4::INVALID_FINE_PSEUDORANGE);
        m7::set_pseudorange(sat, cell, range);
        EXPECT_EQ(cell.fine_pseudorange, m7::INVALID_FINE_PSEUDORANGE);
        m4::set_phase_range(sat, cell, range);
        EXPECT_EQ(cell.fine_phase_range, m4::INVALID_FINE_PHASE_RANGE);
        m7::set_phase_range(sat, cell, range);
        EXPECT_EQ(cell.fine_phase_range, m7::INVALID_FINE_PHASE_RANGE);
        m5::set_phase_range_rate(sat, cell, range);
        EXPECT_EQ(cell.fine_phase_range_rate, m5::INVALID_FINE_RATE);
    }
}

TEST(RtcmMsm, FullRangeFarFromRoughRangeIsInvalid)
{
    // A pseudorange 1 ms away from its (quantised) rough range cannot be represented.
    const msm_satellite sat = rough_70_5();
    msm_signal cell{};
    m4::set_pseudorange(sat, cell, 71.5);
    EXPECT_EQ(cell.fine_pseudorange, m4::INVALID_FINE_PSEUDORANGE);
    m7::set_pseudorange(sat, cell, 69.5);
    EXPECT_EQ(cell.fine_pseudorange, m7::INVALID_FINE_PSEUDORANGE);
    m4::set_phase_range(sat, cell, 71.5);
    EXPECT_EQ(cell.fine_phase_range, m4::INVALID_FINE_PHASE_RANGE);
    m7::set_phase_range(sat, cell, 69.5);
    EXPECT_EQ(cell.fine_phase_range, m7::INVALID_FINE_PHASE_RANGE);
}

TEST(RtcmMsm, PhaseRangeRateIsRelativeToRoughRateInTenthsOfMillimetrePerSecond)
{
    msm_satellite sat = rough_70_5(); // rough rate -123 m/s
    msm_signal cell{};

    m5::set_phase_range_rate(sat, cell, -123.0);
    EXPECT_EQ(cell.fine_phase_range_rate, 0);
    m5::set_phase_range_rate(sat, cell, -122.5);
    EXPECT_EQ(cell.fine_phase_range_rate, 5000);
    m7::set_phase_range_rate(sat, cell, -123.5);
    EXPECT_EQ(cell.fine_phase_range_rate, -5000);
    m7::set_phase_range_rate(sat, cell, -123.0 + 0.0001);
    EXPECT_EQ(cell.fine_phase_range_rate, 1);
    m7::set_phase_range_rate(sat, cell, -123.0 - 0.0001);
    EXPECT_EQ(cell.fine_phase_range_rate, -1);
    m7::set_phase_range_rate(sat, cell, -123.0 + 0.00004);
    EXPECT_EQ(cell.fine_phase_range_rate, 0);
    m7::set_phase_range_rate(sat, cell, -123.0 + 0.00006);
    EXPECT_EQ(cell.fine_phase_range_rate, 1);

    // The rough rate is the reference, not zero.
    sat.rough_phase_range_rate = 0;
    m7::set_phase_range_rate(sat, cell, 0.5);
    EXPECT_EQ(cell.fine_phase_range_rate, 5000);
}

TEST(RtcmMsm, PhaseRangeRateBoundaries)
{
    msm_satellite sat{};
    sat.rough_phase_range_rate = 0;
    msm_signal cell{};
    const int16_t invalid = m5::INVALID_FINE_RATE;

    m5::set_phase_range_rate(sat, cell, 1.6383);
    EXPECT_EQ(cell.fine_phase_range_rate, 16383);
    m5::set_phase_range_rate(sat, cell, 1.63834);
    EXPECT_EQ(cell.fine_phase_range_rate, 16383);
    m5::set_phase_range_rate(sat, cell, 1.63836); // rounds onto +2^14
    EXPECT_EQ(cell.fine_phase_range_rate, invalid);
    m5::set_phase_range_rate(sat, cell, 1.6384);
    EXPECT_EQ(cell.fine_phase_range_rate, invalid);
    m5::set_phase_range_rate(sat, cell, 100.0);
    EXPECT_EQ(cell.fine_phase_range_rate, invalid);

    m7::set_phase_range_rate(sat, cell, -1.6383);
    EXPECT_EQ(cell.fine_phase_range_rate, -16383);
    m7::set_phase_range_rate(sat, cell, -1.63834);
    EXPECT_EQ(cell.fine_phase_range_rate, -16383);
    m7::set_phase_range_rate(sat, cell, -1.63836); // rounds onto -2^14 == the marker
    EXPECT_EQ(cell.fine_phase_range_rate, invalid);
    m7::set_phase_range_rate(sat, cell, -1.6384);
    EXPECT_EQ(cell.fine_phase_range_rate, invalid);
    m7::set_phase_range_rate(sat, cell, -100.0);
    EXPECT_EQ(cell.fine_phase_range_rate, invalid);
    EXPECT_EQ(invalid, -16384);
}

// ---------------------------------------------------------------------------
// set_cnr
// ---------------------------------------------------------------------------

TEST(RtcmMsm, CnrMsm4IsOneDbHzPerStep)
{
    msm_signal cell{};
    cell.cnr = 99U;

    m4::set_cnr(cell, 0.0);
    EXPECT_EQ(cell.cnr, 0U);
    m4::set_cnr(cell, 40.0);
    EXPECT_EQ(cell.cnr, 40U);
    m4::set_cnr(cell, 40.49);
    EXPECT_EQ(cell.cnr, 40U);
    m4::set_cnr(cell, 40.5);
    EXPECT_EQ(cell.cnr, 41U);
    m5::set_cnr(cell, 45.2);
    EXPECT_EQ(cell.cnr, 45U);
    m4::set_cnr(cell, 62.4);
    EXPECT_EQ(cell.cnr, 62U);
    m4::set_cnr(cell, 63.0);
    EXPECT_EQ(cell.cnr, 63U);
}

TEST(RtcmMsm, CnrMsm4SaturatesAndClampsNegative)
{
    msm_signal cell{};
    m4::set_cnr(cell, 63.4);
    EXPECT_EQ(cell.cnr, 63U);
    m4::set_cnr(cell, 63.5); // would round to 64: saturate at the field maximum
    EXPECT_EQ(cell.cnr, 63U);
    m4::set_cnr(cell, 64.0);
    EXPECT_EQ(cell.cnr, 63U);
    m4::set_cnr(cell, 1e6);
    EXPECT_EQ(cell.cnr, 63U);
    m4::set_cnr(cell, INFINITY);
    EXPECT_EQ(cell.cnr, 63U);

    cell.cnr = 20U;
    m4::set_cnr(cell, -0.1);
    EXPECT_EQ(cell.cnr, 0U);
    cell.cnr = 20U;
    m4::set_cnr(cell, -45.0);
    EXPECT_EQ(cell.cnr, 0U);
    cell.cnr = 20U;
    m4::set_cnr(cell, -INFINITY);
    EXPECT_EQ(cell.cnr, 0U);
    cell.cnr = 20U;
    m4::set_cnr(cell, NAN);
    EXPECT_EQ(cell.cnr, 0U);
}

TEST(RtcmMsm, CnrMsm7IsOneSixteenthDbHzPerStep)
{
    msm_signal cell{};
    m7::set_cnr(cell, 0.0);
    EXPECT_EQ(cell.cnr, 0U);
    m7::set_cnr(cell, 0.0625);
    EXPECT_EQ(cell.cnr, 1U);
    m7::set_cnr(cell, 0.03); // 0.48 steps
    EXPECT_EQ(cell.cnr, 0U);
    m7::set_cnr(cell, 0.0313); // 0.5008 steps
    EXPECT_EQ(cell.cnr, 1U);
    m7::set_cnr(cell, 40.5);
    EXPECT_EQ(cell.cnr, 648U);
    m7::set_cnr(cell, 40.53125); // 648.5 steps rounds up
    EXPECT_EQ(cell.cnr, 649U);
    m7::set_cnr(cell, 63.9375);
    EXPECT_EQ(cell.cnr, 1023U);
}

TEST(RtcmMsm, CnrMsm7SaturatesAndClampsNegative)
{
    msm_signal cell{};
    m7::set_cnr(cell, 63.96875); // 1023.5 steps
    EXPECT_EQ(cell.cnr, 1023U);
    m7::set_cnr(cell, 64.0);
    EXPECT_EQ(cell.cnr, 1023U);
    m7::set_cnr(cell, 1e9);
    EXPECT_EQ(cell.cnr, 1023U);
    m7::set_cnr(cell, -0.01);
    EXPECT_EQ(cell.cnr, 0U);
    m7::set_cnr(cell, -100.0);
    EXPECT_EQ(cell.cnr, 0U);
}

// ---------------------------------------------------------------------------
// Golden frames
// ---------------------------------------------------------------------------

TEST(RtcmMsm, GoldenGpsMsm4) { expect_golden<gps_msm4>(1074U, false, golden_1074); }
TEST(RtcmMsm, GoldenGpsMsm5) { expect_golden<gps_msm5>(1075U, false, golden_1075); }
TEST(RtcmMsm, GoldenGpsMsm7) { expect_golden<gps_msm7>(1077U, false, golden_1077); }
TEST(RtcmMsm, GoldenGlonassMsm4) { expect_golden<glonass_msm4>(1084U, true, golden_1084); }
TEST(RtcmMsm, GoldenGlonassMsm5) { expect_golden<glonass_msm5>(1085U, true, golden_1085); }
TEST(RtcmMsm, GoldenGlonassMsm7) { expect_golden<glonass_msm7>(1087U, true, golden_1087); }
TEST(RtcmMsm, GoldenGalileoMsm4) { expect_golden<galileo_msm4>(1094U, false, golden_1094); }
TEST(RtcmMsm, GoldenGalileoMsm5) { expect_golden<galileo_msm5>(1095U, false, golden_1095); }
TEST(RtcmMsm, GoldenGalileoMsm7) { expect_golden<galileo_msm7>(1097U, false, golden_1097); }
TEST(RtcmMsm, GoldenBeidouMsm4) { expect_golden<beidou_msm4>(1124U, false, golden_1124); }
TEST(RtcmMsm, GoldenBeidouMsm5) { expect_golden<beidou_msm5>(1125U, false, golden_1125); }
TEST(RtcmMsm, GoldenBeidouMsm7) { expect_golden<beidou_msm7>(1127U, false, golden_1127); }

TEST(RtcmMsm, ReferenceEpochQuantisedValues)
{
    const gps_msm7 message = reference_epoch<gps_msm7>(false);
    EXPECT_EQ(message.satellite_mask, m7::satellite_bit(3U) | m7::satellite_bit(17U) | m7::satellite_bit(30U));
    EXPECT_EQ(message.signal_mask, 0x40010000U);
    EXPECT_EQ(message.satellites[0U].rough_range_ms, 70U);
    EXPECT_EQ(message.satellites[0U].rough_range_mod, 126U);
    EXPECT_EQ(message.satellites[0U].rough_phase_range_rate, -123);
    EXPECT_EQ(message.satellites[0U].extended_info, 7U);
    EXPECT_EQ(message.satellites[1U].rough_range_ms, 75U);
    EXPECT_EQ(message.satellites[1U].rough_range_mod, 512U);
    EXPECT_EQ(message.satellites[1U].rough_phase_range_rate, -73);
    EXPECT_EQ(message.satellites[2U].rough_range_ms, 80U);
    EXPECT_EQ(message.satellites[2U].rough_range_mod, 256U);
    EXPECT_EQ(message.satellites[2U].rough_phase_range_rate, -23);
    EXPECT_EQ(message.satellites[2U].extended_info, 9U);
    EXPECT_EQ(message.signals[0U].cnr, 648U);
    EXPECT_EQ(message.signals[1U].cnr, 664U);
    EXPECT_TRUE(message.signals[1U].half_cycle);
    EXPECT_FALSE(message.signals[2U].half_cycle);
    EXPECT_EQ(message.signals[3U].lock_time, 6U);
}

// ---------------------------------------------------------------------------
// Hand-computed payload layout
// ---------------------------------------------------------------------------

// One GPS MSM4 message: satellite ID 1, signal ID 2, one cell. Field by field, MSB first:
//
//   DF002 type            12   010000110010      1074
//   DF003 station          12   000000000001      1
//   epoch (TOW)            30   000000000000000000000000000001   1 ms
//   DF393 multiple          1   1
//   DF409 IODS              3   101               5
//   reserved                7   0000000
//   DF411 clock steering    2   10                2
//   DF412 external clock    2   01                1
//   DF417 divergence-free   1   1
//   DF418 smoothing         3   110               6
//   DF394 satellite mask   64   1 followed by 63 zeros      (satellite ID 1)
//   DF395 signal mask      32   01 followed by 30 zeros     (signal ID 2)
//   DF396 cell mask         1   1                 (1 satellite x 1 signal)
//   DF397 rough range ms    8   01000110          70
//   DF398 rough range mod  10   1000000000        512 (= 0.5 ms)
//   DF400 fine pseudorange 15   111111111111110   -2
//   DF401 fine phase range 22   0000000000000000000011   3
//   DF402 lock time         4   1001              9
//   DF420 half cycle        1   1
//   DF403 CNR               6   101101            45
//   padding                 4   0000              (236 bits -> 30 bytes)
//
// Grouped into bytes:
//   01000011 00100000 00000001 00000000 00000000 00000000 00000111 01000000 01001111 01000000
//   00000000 00000000 00000000 00000000 00000000 00000000 00000000 00100000 00000000 00000000
//   00000000 01010001 10100000 11111111 11000000 00000000 00000001 11001110 11010000
//   =  43 20 01 00 00 00 07 40 4f 40 | 00 00 00 00 00 00 00 20 00 00 | 00 51 a0 0f ff c0 00 01 ce d0
//
// The frame is d3 00 1e <payload> <CRC-24Q 58 ea 91>; the payload was also decoded with pyrtcm.
msm_message<msm_system::gps, 4U> hand_computed_message()
{
    msm_message<msm_system::gps, 4U> message{};
    message.station_id = 1U;
    message.epoch_time = 1U;
    message.multiple_message = true;
    message.iods = 5U;
    message.clock_steering = 2U;
    message.external_clock = 1U;
    message.divergence_free = true;
    message.smoothing_interval = 6U;
    message.satellite_mask = m4::satellite_bit(1U);
    message.signal_mask = m4::signal_bit(2U);
    message.cell_mask = m4::cell_bit(0U, 0U, 1U, 1U);

    msm_satellite sat{};
    sat.rough_range_ms = 70U;
    sat.rough_range_mod = 512U;
    static_cast<void>(message.satellites.push_back(sat));

    msm_signal cell{};
    cell.fine_pseudorange = -2;
    cell.fine_phase_range = 3;
    cell.lock_time = 9U;
    cell.half_cycle = true;
    cell.cnr = 45U;
    static_cast<void>(message.signals.push_back(cell));
    return message;
}

TEST(RtcmMsm, HandComputedMsm4PayloadBits)
{
    const encoded frame = encode(hand_computed_message());
    ASSERT_EQ(frame.result, castle::status::ok);
    ASSERT_EQ(frame.size, 36U);

    static const uint8_t expected_payload[30U] = {
        0x43U, 0x20U, 0x01U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U, 0x4FU, 0x40U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x20U, 0x00U, 0x00U,
        0x00U, 0x51U, 0xA0U, 0x0FU, 0xFFU, 0xC0U, 0x00U, 0x01U, 0xCEU, 0xD0U};

    EXPECT_EQ(frame.bytes[0U], 0xD3U);
    EXPECT_EQ(frame.bytes[1U], 0x00U);
    EXPECT_EQ(frame.bytes[2U], 0x1EU);
    EXPECT_EQ(memcmp(&frame.bytes[3U], expected_payload, sizeof(expected_payload)), 0)
        << hex_of(&frame.bytes[3U], 30U);
    EXPECT_EQ(hex_of(frame.bytes, frame.size),
              "d3001e43200100000007404f40000000000000002000000051a00fffc00001ced058ea91");
    EXPECT_EQ(frame.bytes[33U], 0x58U);
    EXPECT_EQ(frame.bytes[34U], 0xEAU);
    EXPECT_EQ(frame.bytes[35U], 0x91U);
    EXPECT_EQ(bits_at(frame, 236U, 4U), 0U) << "the four padding bits are zero";
}

TEST(RtcmMsm, FieldPositionsOfTheMinimalMessage)
{
    // One satellite/signal per level; every field written at its documented offset, value-distinct.
    using layout = minimal_layout<m7>;
    m7 message = minimal<m7>();
    message.station_id = 0xABCU;
    message.epoch_time = 0x2AAAAAAAU;
    message.multiple_message = true;
    message.iods = 6U;
    message.clock_steering = 1U;
    message.external_clock = 2U;
    message.divergence_free = true;
    message.smoothing_interval = 5U;
    message.satellites[0U].rough_range_ms = 200U;
    message.satellites[0U].extended_info = 9U;
    message.satellites[0U].rough_range_mod = 777U;
    message.satellites[0U].rough_phase_range_rate = -1000;
    message.signals[0U].fine_pseudorange = -123456;
    message.signals[0U].fine_phase_range = 7654321;
    message.signals[0U].lock_time = 1000U;
    message.signals[0U].half_cycle = true;
    message.signals[0U].cnr = 999U;
    message.signals[0U].fine_phase_range_rate = -12345;

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, 0U, 12U), 1077U);
    EXPECT_EQ(bits_at(frame, layout::STATION, 12U), 0xABCU);
    EXPECT_EQ(bits_at(frame, layout::EPOCH, 30U), 0x2AAAAAAAU);
    EXPECT_EQ(bits_at(frame, layout::MULTIPLE, 1U), 1U);
    EXPECT_EQ(bits_at(frame, layout::IODS, 3U), 6U);
    EXPECT_EQ(bits_at(frame, layout::RESERVED, 7U), 0U);
    EXPECT_EQ(bits_at(frame, layout::CLOCK_STEERING, 2U), 1U);
    EXPECT_EQ(bits_at(frame, layout::EXTERNAL_CLOCK, 2U), 2U);
    EXPECT_EQ(bits_at(frame, layout::DIVERGENCE_FREE, 1U), 1U);
    EXPECT_EQ(bits_at(frame, layout::SMOOTHING, 3U), 5U);
    EXPECT_EQ(bits_at(frame, layout::SATELLITE_MASK, 64U), 0x8000000000000000ULL);
    EXPECT_EQ(bits_at(frame, layout::SIGNAL_MASK, 32U), 0x80000000U);
    EXPECT_EQ(bits_at(frame, layout::CELL_MASK, 1U), 1U);
    EXPECT_EQ(bits_at(frame, layout::ROUGH_RANGE_MS, 8U), 200U);
    EXPECT_EQ(bits_at(frame, layout::EXTENDED_INFO, 4U), 9U);
    EXPECT_EQ(bits_at(frame, layout::ROUGH_RANGE_MOD, 10U), 777U);
    EXPECT_EQ(bits_at(frame, layout::ROUGH_RATE, 14U), low_bits(-1000, 14U));
    EXPECT_EQ(bits_at(frame, layout::PSEUDORANGE, 20U), low_bits(-123456, 20U));
    EXPECT_EQ(bits_at(frame, layout::PHASE_RANGE, 24U), low_bits(7654321, 24U));
    EXPECT_EQ(bits_at(frame, layout::LOCK_TIME, 10U), 1000U);
    EXPECT_EQ(bits_at(frame, layout::HALF_CYCLE, 1U), 1U);
    EXPECT_EQ(bits_at(frame, layout::CNR, 10U), 999U);
    EXPECT_EQ(bits_at(frame, layout::FINE_RATE, 15U), low_bits(-12345, 15U));
    EXPECT_EQ(layout::END, 206U + 80U);
    EXPECT_EQ(payload_length_of(frame), (layout::END + 7U) / 8U);
}

TEST(RtcmMsm, DataIsFieldMajor)
{
    // 2 satellites x 1 signal, both present: every field is written for all entries before the next.
    m5 message{};
    message.satellite_mask = m5::satellite_bit(4U) | m5::satellite_bit(9U);
    message.signal_mask = m5::signal_bit(7U);
    message.cell_mask = m5::cell_bit(0U, 0U, 2U, 1U) | m5::cell_bit(1U, 0U, 2U, 1U);

    msm_satellite sat{};
    sat.rough_range_ms = 11U;
    sat.extended_info = 1U;
    sat.rough_range_mod = 101U;
    sat.rough_phase_range_rate = -5;
    static_cast<void>(message.satellites.push_back(sat));
    sat.rough_range_ms = 22U;
    sat.extended_info = 2U;
    sat.rough_range_mod = 202U;
    sat.rough_phase_range_rate = 6;
    static_cast<void>(message.satellites.push_back(sat));

    msm_signal cell{};
    cell.fine_pseudorange = 100;
    cell.fine_phase_range = -200;
    cell.lock_time = 3U;
    cell.half_cycle = false;
    cell.cnr = 30U;
    cell.fine_phase_range_rate = -7;
    static_cast<void>(message.signals.push_back(cell));
    cell.fine_pseudorange = -100;
    cell.fine_phase_range = 200;
    cell.lock_time = 4U;
    cell.half_cycle = true;
    cell.cnr = 31U;
    cell.fine_phase_range_rate = 8;
    static_cast<void>(message.signals.push_back(cell));

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);

    castle::size_type position = 169U;
    EXPECT_EQ(bits_at(frame, position, 2U), 3U); // cell mask
    position += 2U;
    EXPECT_EQ(bits_at(frame, position, 8U), 11U);
    EXPECT_EQ(bits_at(frame, position + 8U, 8U), 22U);
    position += 16U;
    EXPECT_EQ(bits_at(frame, position, 4U), 1U);
    EXPECT_EQ(bits_at(frame, position + 4U, 4U), 2U);
    position += 8U;
    EXPECT_EQ(bits_at(frame, position, 10U), 101U);
    EXPECT_EQ(bits_at(frame, position + 10U, 10U), 202U);
    position += 20U;
    EXPECT_EQ(bits_at(frame, position, 14U), low_bits(-5, 14U));
    EXPECT_EQ(bits_at(frame, position + 14U, 14U), 6U);
    position += 28U;
    EXPECT_EQ(bits_at(frame, position, 15U), 100U);
    EXPECT_EQ(bits_at(frame, position + 15U, 15U), low_bits(-100, 15U));
    position += 30U;
    EXPECT_EQ(bits_at(frame, position, 22U), low_bits(-200, 22U));
    EXPECT_EQ(bits_at(frame, position + 22U, 22U), 200U);
    position += 44U;
    EXPECT_EQ(bits_at(frame, position, 4U), 3U);
    EXPECT_EQ(bits_at(frame, position + 4U, 4U), 4U);
    position += 8U;
    EXPECT_EQ(bits_at(frame, position, 1U), 0U);
    EXPECT_EQ(bits_at(frame, position + 1U, 1U), 1U);
    position += 2U;
    EXPECT_EQ(bits_at(frame, position, 6U), 30U);
    EXPECT_EQ(bits_at(frame, position + 6U, 6U), 31U);
    position += 12U;
    EXPECT_EQ(bits_at(frame, position, 15U), low_bits(-7, 15U));
    EXPECT_EQ(bits_at(frame, position + 15U, 15U), 8U);
    position += 30U;
    EXPECT_EQ(payload_length_of(frame), (position + 7U) / 8U);
}

TEST(RtcmMsm, Msm4OmitsExtendedInfoAndRateFields)
{
    const m4 plain = minimal<m4>();
    m4 noisy = plain;
    noisy.satellites[0U].extended_info = 0xFFU;          // would be out of range if it were written
    noisy.satellites[0U].rough_phase_range_rate = 30000; // ditto
    noisy.signals[0U].fine_phase_range_rate = 30000;

    const encoded a = encode(plain);
    const encoded b = encode(noisy);
    ASSERT_EQ(a.result, castle::status::ok);
    ASSERT_EQ(b.result, castle::status::ok);
    EXPECT_EQ(a.size, b.size);
    EXPECT_EQ(hex_of(a.bytes, a.size), hex_of(b.bytes, b.size));
    EXPECT_EQ(payload_length_of(a), (minimal_layout<m4>::END + 7U) / 8U);
    EXPECT_EQ(minimal_layout<m4>::END, 236U);
}

TEST(RtcmMsm, Msm5AndMsm7WriteExtendedInfoAndRateFields)
{
    m5 low = minimal<m5>();
    m5 high = low;
    high.satellites[0U].extended_info = 15U;
    high.satellites[0U].rough_phase_range_rate = 100;
    high.signals[0U].fine_phase_range_rate = 100;
    const encoded a = encode(low);
    const encoded b = encode(high);
    ASSERT_EQ(a.result, castle::status::ok);
    ASSERT_EQ(b.result, castle::status::ok);
    EXPECT_NE(hex_of(a.bytes, a.size), hex_of(b.bytes, b.size));
    EXPECT_EQ(bits_at(b, minimal_layout<m5>::EXTENDED_INFO, 4U), 15U);
    EXPECT_EQ(bits_at(b, minimal_layout<m5>::ROUGH_RATE, 14U), 100U);
    EXPECT_EQ(bits_at(b, minimal_layout<m5>::FINE_RATE, 15U), 100U);
    EXPECT_EQ(minimal_layout<m5>::END, 269U);
    EXPECT_EQ(payload_length_of(a), (minimal_layout<m5>::END + 7U) / 8U);
}

// ---------------------------------------------------------------------------
// Header field limits (all four levels where the field exists)
// ---------------------------------------------------------------------------

template <typename Message>
void expect_header_limits()
{
    using layout = minimal_layout<Message>;
    expect_field_limits<Message>([](Message& m, int64_t v) { m.station_id = static_cast<uint16_t>(v); },
                                 layout::STATION, 12U, 0, 4095);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.epoch_time = static_cast<uint32_t>(v); },
                                 layout::EPOCH, 30U, 0, 0x3FFFFFFF);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.iods = static_cast<uint8_t>(v); },
                                 layout::IODS, 3U, 0, 7);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.clock_steering = static_cast<uint8_t>(v); },
                                 layout::CLOCK_STEERING, 2U, 0, 3);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.external_clock = static_cast<uint8_t>(v); },
                                 layout::EXTERNAL_CLOCK, 2U, 0, 3);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.smoothing_interval = static_cast<uint8_t>(v); },
                                 layout::SMOOTHING, 3U, 0, 7);
}

template <typename Message>
void expect_satellite_limits()
{
    using layout = minimal_layout<Message>;
    expect_field_limits<Message>(
        [](Message& m, int64_t v) { m.satellites[0U].rough_range_mod = static_cast<uint16_t>(v); },
        layout::ROUGH_RANGE_MOD, 10U, 0, 1023);
    if constexpr (Message::HAS_RATE)
    {
        expect_field_limits<Message>(
            [](Message& m, int64_t v) { m.satellites[0U].extended_info = static_cast<uint8_t>(v); },
            layout::EXTENDED_INFO, 4U, 0, 15);
        expect_field_limits<Message>(
            [](Message& m, int64_t v) { m.satellites[0U].rough_phase_range_rate = static_cast<int16_t>(v); },
            layout::ROUGH_RATE, 14U, -8192, 8191);
    }
}

template <typename Message>
void expect_signal_limits()
{
    using layout = minimal_layout<Message>;
    const int64_t pseudorange_max = (int64_t{1} << (Message::PSEUDORANGE_BITS - 1U)) - 1;
    const int64_t phase_max = (int64_t{1} << (Message::PHASE_RANGE_BITS - 1U)) - 1;
    const int64_t lock_max = (int64_t{1} << Message::LOCK_TIME_BITS) - 1;
    const int64_t cnr_max = (int64_t{1} << Message::CNR_BITS) - 1;

    expect_field_limits<Message>([](Message& m, int64_t v) { m.signals[0U].fine_pseudorange = static_cast<int32_t>(v); },
                                 layout::PSEUDORANGE, Message::PSEUDORANGE_BITS, -pseudorange_max - 1, pseudorange_max);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.signals[0U].fine_phase_range = static_cast<int32_t>(v); },
                                 layout::PHASE_RANGE, Message::PHASE_RANGE_BITS, -phase_max - 1, phase_max);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.signals[0U].lock_time = static_cast<uint16_t>(v); },
                                 layout::LOCK_TIME, Message::LOCK_TIME_BITS, 0, lock_max);
    expect_field_limits<Message>([](Message& m, int64_t v) { m.signals[0U].cnr = static_cast<uint16_t>(v); },
                                 layout::CNR, Message::CNR_BITS, 0, cnr_max);
    if constexpr (Message::HAS_RATE)
    {
        expect_field_limits<Message>(
            [](Message& m, int64_t v) { m.signals[0U].fine_phase_range_rate = static_cast<int16_t>(v); },
            layout::FINE_RATE, 15U, -16384, 16383);
    }
}

TEST(RtcmMsm, HeaderFieldLimitsMsm4) { expect_header_limits<m4>(); }
TEST(RtcmMsm, HeaderFieldLimitsMsm5) { expect_header_limits<m5>(); }
TEST(RtcmMsm, HeaderFieldLimitsMsm7) { expect_header_limits<m7>(); }
TEST(RtcmMsm, SatelliteFieldLimitsMsm4) { expect_satellite_limits<m4>(); }
TEST(RtcmMsm, SatelliteFieldLimitsMsm5) { expect_satellite_limits<m5>(); }
TEST(RtcmMsm, SatelliteFieldLimitsMsm7) { expect_satellite_limits<m7>(); }
TEST(RtcmMsm, SignalFieldLimitsMsm4) { expect_signal_limits<m4>(); }
TEST(RtcmMsm, SignalFieldLimitsMsm5) { expect_signal_limits<m5>(); }
TEST(RtcmMsm, SignalFieldLimitsMsm7) { expect_signal_limits<m7>(); }

TEST(RtcmMsm, FlagsAreSingleBits)
{
    m4 message = minimal<m4>();
    encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::MULTIPLE, 1U), 0U);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::DIVERGENCE_FREE, 1U), 0U);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::HALF_CYCLE, 1U), 0U);

    message.multiple_message = true;
    frame = encode(message);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::MULTIPLE, 1U), 1U);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::DIVERGENCE_FREE, 1U), 0U);

    message.multiple_message = false;
    message.divergence_free = true;
    message.signals[0U].half_cycle = true;
    frame = encode(message);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::MULTIPLE, 1U), 0U);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::DIVERGENCE_FREE, 1U), 1U);
    EXPECT_EQ(bits_at(frame, minimal_layout<m4>::HALF_CYCLE, 1U), 1U);
}

TEST(RtcmMsm, InvalidMarkersEncodeAtTheirFieldWidth)
{
    m7 message = minimal<m7>();
    m7::set_rough_range(message.satellites[0U], -1.0, NAN);
    m7::set_pseudorange(message.satellites[0U], message.signals[0U], 1000.0);
    m7::set_phase_range(message.satellites[0U], message.signals[0U], 1000.0);
    m7::set_phase_range_rate(message.satellites[0U], message.signals[0U], 1e6);

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    using layout = minimal_layout<m7>;
    EXPECT_EQ(bits_at(frame, layout::ROUGH_RANGE_MS, 8U), 0xFFU);
    EXPECT_EQ(bits_at(frame, layout::ROUGH_RANGE_MOD, 10U), 0U);
    EXPECT_EQ(bits_at(frame, layout::ROUGH_RATE, 14U), 0x2000U);       // -8192: 1 followed by zeros
    EXPECT_EQ(bits_at(frame, layout::PSEUDORANGE, 20U), 0x80000U);     // -2^19
    EXPECT_EQ(bits_at(frame, layout::PHASE_RANGE, 24U), 0x800000U);    // -2^23
    EXPECT_EQ(bits_at(frame, layout::FINE_RATE, 15U), 0x4000U);        // -2^14
}

// ---------------------------------------------------------------------------
// Encode validation
// ---------------------------------------------------------------------------

TEST(RtcmMsm, SatelliteCountMustMatchSatelliteMask)
{
    m4 message = minimal<m4>();
    message.satellite_mask |= m4::satellite_bit(9U); // two bits, one entry
    encoded frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);
    EXPECT_EQ(frame.size, 0U);

    message = minimal<m4>();
    static_cast<void>(message.satellites.push_back(msm_satellite{})); // one bit, two entries
    frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);
    EXPECT_EQ(frame.size, 0U);

    message = minimal<m4>();
    message.satellite_mask = 0U; // no bit, one entry
    frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);

    message = minimal<m4>();
    message.satellites.clear(); // one bit, no entry
    frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);
}

TEST(RtcmMsm, SignalCountMustMatchCellMask)
{
    m4 message = minimal<m4>();
    static_cast<void>(message.signals.push_back(msm_signal{})); // one cell bit, two entries
    encoded frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);
    EXPECT_EQ(frame.size, 0U);

    message = minimal<m4>();
    message.signals.clear(); // one cell bit, no entry
    frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);

    // Two satellites, one signal: both cell bits set but only one entry.
    m4 two = minimal<m4>();
    two.satellite_mask |= m4::satellite_bit(2U);
    static_cast<void>(two.satellites.push_back(msm_satellite{}));
    two.cell_mask = m4::cell_bit(0U, 0U, 2U, 1U) | m4::cell_bit(1U, 0U, 2U, 1U);
    frame = encode(two);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);

    static_cast<void>(two.signals.push_back(msm_signal{}));
    frame = encode(two);
    EXPECT_EQ(frame.result, castle::status::ok);
}

TEST(RtcmMsm, CellMaskWiderThanTheCellGridIsRejected)
{
    // 1 satellite x 1 signal gives a 1-bit mask: bit 1 lies outside it although popcount matches.
    m4 message = minimal<m4>();
    message.cell_mask = 0x2U;
    encoded frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);

    message.cell_mask = 0x80000000ULL << 32U; // bit 63
    frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::out_of_range);

    // 2 satellites x 1 signal: bit 2 is outside the 2-bit grid.
    m4 two = minimal<m4>();
    two.satellite_mask |= m4::satellite_bit(2U);
    static_cast<void>(two.satellites.push_back(msm_satellite{}));
    two.cell_mask = 0x5U; // popcount 2 matches the two signal entries, but bit 2 is outside the grid
    static_cast<void>(two.signals.push_back(msm_signal{}));
    frame = encode(two);
    EXPECT_EQ(frame.result, castle::status::out_of_range);

    two.cell_mask = 0x3U;
    frame = encode(two);
    EXPECT_EQ(frame.result, castle::status::ok);
}

TEST(RtcmMsm, NoCellsRejectsAnyCellMaskBit)
{
    m4 message{};
    message.satellite_mask = m4::satellite_bit(5U); // one satellite, no signals: zero cell bits
    static_cast<void>(message.satellites.push_back(msm_satellite{}));
    message.cell_mask = 1U;
    static_cast<void>(message.signals.push_back(msm_signal{}));
    EXPECT_EQ(encode(message).result, castle::status::out_of_range);

    message.cell_mask = 0U;
    message.signals.clear();
    EXPECT_EQ(encode(message).result, castle::status::ok);
}

TEST(RtcmMsm, MoreThanSixtyFourCellsIsRejected)
{
    // 13 satellites x 5 signals = 65 cells.
    m4 message{};
    for (castle::size_type id = 1U; id <= 13U; ++id)
    {
        message.satellite_mask |= m4::satellite_bit(id);
        static_cast<void>(message.satellites.push_back(msm_satellite{}));
    }
    for (castle::size_type id = 1U; id <= 5U; ++id)
    {
        message.signal_mask |= m4::signal_bit(id);
    }
    message.cell_mask = ~static_cast<uint64_t>(0U);
    for (castle::size_type index = 0U; index < 64U; ++index)
    {
        static_cast<void>(message.signals.push_back(msm_signal{}));
    }
    encoded frame = encode(message);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);
    EXPECT_EQ(frame.size, 0U);

    // 33 satellites x 2 signals = 66; every other count matches, only the product is wrong.
    m4 wide{};
    for (castle::size_type id = 1U; id <= 33U; ++id)
    {
        wide.satellite_mask |= m4::satellite_bit(id);
        static_cast<void>(wide.satellites.push_back(msm_satellite{}));
    }
    wide.signal_mask = m4::signal_bit(1U) | m4::signal_bit(2U);
    wide.cell_mask = 1U;
    static_cast<void>(wide.signals.push_back(msm_signal{}));
    frame = encode(wide);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);

    // 64 satellites x 32 signals.
    m4 full{};
    full.satellite_mask = ~static_cast<uint64_t>(0U);
    full.signal_mask = ~static_cast<uint32_t>(0U);
    for (castle::size_type index = 0U; index < 64U; ++index)
    {
        static_cast<void>(full.satellites.push_back(msm_satellite{}));
    }
    frame = encode(full);
    EXPECT_EQ(frame.result, castle::status::invalid_argument);

    // 13 x 5 is the first oversize product; 8 x 8 = 64 is fine.
    m4 square{};
    for (castle::size_type id = 1U; id <= 8U; ++id)
    {
        square.satellite_mask |= m4::satellite_bit(id);
        square.signal_mask |= m4::signal_bit(id);
        static_cast<void>(square.satellites.push_back(msm_satellite{}));
    }
    square.cell_mask = ~static_cast<uint64_t>(0U);
    for (castle::size_type index = 0U; index < 64U; ++index)
    {
        static_cast<void>(square.signals.push_back(msm_signal{}));
    }
    frame = encode(square);
    EXPECT_EQ(frame.result, castle::status::ok);
}

TEST(RtcmMsm, EmptyMessageEncodesHeaderOnly)
{
    m7 message{};
    message.station_id = 7U;
    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    // 169 header bits -> 22 payload bytes.
    EXPECT_EQ(payload_length_of(frame), 22U);
    EXPECT_EQ(frame.size, 28U);
    EXPECT_EQ(bits_at(frame, 0U, 12U), 1077U);
    EXPECT_EQ(bits_at(frame, 12U, 12U), 7U);
    EXPECT_EQ(bits_at(frame, 73U, 64U), 0U);
    EXPECT_EQ(bits_at(frame, 137U, 32U), 0U);
    EXPECT_EQ(bits_at(frame, 169U, 7U), 0U) << "padding bits";
}

TEST(RtcmMsm, SatellitesWithoutSignalsEncodeRangeFieldsOnly)
{
    m5 message{};
    message.satellite_mask = m5::satellite_bit(2U) | m5::satellite_bit(4U);
    message.signal_mask = 0U;
    msm_satellite sat{};
    sat.rough_range_ms = 10U;
    static_cast<void>(message.satellites.push_back(sat));
    sat.rough_range_ms = 20U;
    static_cast<void>(message.satellites.push_back(sat));

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    // 169 header bits, no cell mask, 2 x (8 + 4 + 10 + 14) = 72 satellite bits.
    EXPECT_EQ(payload_length_of(frame), (169U + 72U + 7U) / 8U);
    EXPECT_EQ(bits_at(frame, 169U, 8U), 10U);
    EXPECT_EQ(bits_at(frame, 177U, 8U), 20U);
}

TEST(RtcmMsm, SignalsWithoutSatellitesEncodeMasksOnly)
{
    m4 message{};
    message.signal_mask = m4::signal_bit(1U) | m4::signal_bit(2U);
    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_length_of(frame), 22U);
    EXPECT_EQ(bits_at(frame, 137U, 32U), 0xC0000000U);
}

TEST(RtcmMsm, SixtyFourSatellitesOneSignalMsm7)
{
    m7 message{};
    message.station_id = 4095U;
    message.satellite_mask = ~static_cast<uint64_t>(0U);
    message.signal_mask = m7::signal_bit(32U);
    for (castle::size_type index = 0U; index < 64U; ++index)
    {
        message.cell_mask |= m7::cell_bit(index, 0U, 64U, 1U);
        msm_satellite sat{};
        sat.rough_range_ms = static_cast<uint8_t>(index + 100U);
        sat.extended_info = static_cast<uint8_t>(index & 0x0FU);
        sat.rough_range_mod = static_cast<uint16_t>(index * 16U);
        sat.rough_phase_range_rate = static_cast<int16_t>(static_cast<int>(index) * 100 - 3200);
        ASSERT_EQ(message.satellites.push_back(sat), castle::status::ok);
        msm_signal cell{};
        cell.fine_pseudorange = static_cast<int32_t>(index) * 1000 - 32000;
        cell.fine_phase_range = static_cast<int32_t>(index) * 10000 - 320000;
        cell.lock_time = static_cast<uint16_t>(index * 16U);
        cell.half_cycle = ((index & 1U) != 0U);
        cell.cnr = static_cast<uint16_t>(index * 15U);
        cell.fine_phase_range_rate = static_cast<int16_t>(static_cast<int>(index) * 500 - 16000);
        ASSERT_EQ(message.signals.push_back(cell), castle::status::ok);
    }
    EXPECT_EQ(message.cell_mask, ~static_cast<uint64_t>(0U));
    EXPECT_EQ(message.satellites.size(), 64U);
    EXPECT_EQ(message.signals.size(), 64U);

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    // 169 + 64 (cell mask) + 64 * 36 (satellite) + 64 * 80 (cell) = 7657 bits -> 958 bytes.
    EXPECT_EQ(payload_length_of(frame), 958U);
    EXPECT_EQ(frame.size, 964U);
    EXPECT_EQ(bits_at(frame, 73U, 64U), ~static_cast<uint64_t>(0U));
    EXPECT_EQ(bits_at(frame, 137U, 32U), 1U);
    EXPECT_EQ(bits_at(frame, 169U, 64U), ~static_cast<uint64_t>(0U));

    const castle::size_type sat_base = 233U;
    const castle::size_type cell_base = sat_base + (64U * 36U);
    for (castle::size_type index : {0U, 1U, 31U, 62U, 63U})
    {
        EXPECT_EQ(bits_at(frame, sat_base + index * 8U, 8U), index + 100U);
        EXPECT_EQ(bits_at(frame, sat_base + 64U * 8U + index * 4U, 4U), index & 0x0FU);
        EXPECT_EQ(bits_at(frame, sat_base + 64U * 12U + index * 10U, 10U), index * 16U);
        EXPECT_EQ(bits_at(frame, sat_base + 64U * 22U + index * 14U, 14U),
                  low_bits(static_cast<int64_t>(index) * 100 - 3200, 14U));
        EXPECT_EQ(bits_at(frame, cell_base + index * 20U, 20U),
                  low_bits(static_cast<int64_t>(index) * 1000 - 32000, 20U));
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 20U + index * 24U, 24U),
                  low_bits(static_cast<int64_t>(index) * 10000 - 320000, 24U));
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 44U + index * 10U, 10U), index * 16U);
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 54U + index, 1U), index & 1U);
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 55U + index * 10U, 10U), index * 15U);
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 65U + index * 15U, 15U),
                  low_bits(static_cast<int64_t>(index) * 500 - 16000, 15U));
    }
    EXPECT_EQ(cell_base + 64U * 80U, 7657U);

    // Exact-fit buffer works, one byte less does not.
    uint8_t exact[964U];
    castle::size_type written = 0U;
    EXPECT_EQ(encode_message(message, exact, sizeof(exact), written), castle::status::ok);
    EXPECT_EQ(written, 964U);
    EXPECT_EQ(memcmp(exact, frame.bytes, 964U), 0);
    EXPECT_EQ(encode_message(message, exact, sizeof(exact) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmMsm, TwoSatellitesThirtyTwoSignalsMsm4)
{
    m4 message{};
    message.satellite_mask = m4::satellite_bit(10U) | m4::satellite_bit(11U);
    message.signal_mask = ~static_cast<uint32_t>(0U);
    for (castle::size_type sat = 0U; sat < 2U; ++sat)
    {
        msm_satellite entry{};
        entry.rough_range_ms = static_cast<uint8_t>(60U + sat);
        static_cast<void>(message.satellites.push_back(entry));
        for (castle::size_type sig = 0U; sig < 32U; ++sig)
        {
            message.cell_mask |= m4::cell_bit(sat, sig, 2U, 32U);
            msm_signal cell{};
            cell.lock_time = static_cast<uint16_t>(sig % 16U);
            cell.cnr = static_cast<uint16_t>(sat * 32U + sig);
            static_cast<void>(message.signals.push_back(cell));
        }
    }
    EXPECT_EQ(message.cell_mask, ~static_cast<uint64_t>(0U));

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    // 169 + 64 + 2 * 18 + 64 * 48 = 3341 bits -> 418 bytes.
    EXPECT_EQ(payload_length_of(frame), 418U);
    EXPECT_EQ(frame.size, 424U);
    EXPECT_EQ(bits_at(frame, 137U, 32U), 0xFFFFFFFFU);
    EXPECT_EQ(bits_at(frame, 169U, 64U), ~static_cast<uint64_t>(0U));
    const castle::size_type cell_base = 233U + (2U * 18U);
    EXPECT_EQ(bits_at(frame, 233U, 8U), 60U);
    EXPECT_EQ(bits_at(frame, 241U, 8U), 61U);
    // CNR block: after 64 x (15 + 22 + 4 + 1) bits.
    for (castle::size_type index : {0U, 31U, 32U, 63U})
    {
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 42U + index * 6U, 6U), index);
        EXPECT_EQ(bits_at(frame, cell_base + 64U * 37U + index * 4U, 4U), index % 16U);
    }
}

TEST(RtcmMsm, SparseCellMaskKeepsSignalOrder)
{
    // 3 satellites x 3 signals, five cells set; the signals vector is in cell-mask order.
    m4 message{};
    message.satellite_mask = m4::satellite_bit(1U) | m4::satellite_bit(2U) | m4::satellite_bit(3U);
    message.signal_mask = m4::signal_bit(1U) | m4::signal_bit(2U) | m4::signal_bit(3U);
    const castle::size_type cells[5][2] = {{0U, 2U}, {1U, 0U}, {1U, 1U}, {2U, 0U}, {2U, 2U}};
    for (const auto& cell_id : cells)
    {
        message.cell_mask |= m4::cell_bit(cell_id[0U], cell_id[1U], 3U, 3U);
        msm_signal cell{};
        cell.lock_time = static_cast<uint16_t>(cell_id[0U] * 4U + cell_id[1U]);
        static_cast<void>(message.signals.push_back(cell));
    }
    for (int index = 0; index < 3; ++index)
    {
        static_cast<void>(message.satellites.push_back(msm_satellite{}));
    }
    EXPECT_EQ(message.cell_mask, 0x75U); // 0b001110101: row-major 001 110 101

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, 169U, 9U), 0x75U);
    const castle::size_type lock_base = 169U + 9U + (3U * 18U) + (5U * 37U);
    EXPECT_EQ(bits_at(frame, lock_base + 0U, 4U), 2U);
    EXPECT_EQ(bits_at(frame, lock_base + 4U, 4U), 4U);
    EXPECT_EQ(bits_at(frame, lock_base + 8U, 4U), 5U);
    EXPECT_EQ(bits_at(frame, lock_base + 12U, 4U), 8U);
    EXPECT_EQ(bits_at(frame, lock_base + 16U, 4U), 10U);
}

TEST(RtcmMsm, SmallCapacityMessages)
{
    using small = msm_message<msm_system::galileo, 7U, 2U, 3U>;
    small message{};
    message.satellite_mask = small::satellite_bit(1U) | small::satellite_bit(2U);
    message.signal_mask = small::signal_bit(1U) | small::signal_bit(2U);
    message.cell_mask = small::cell_bit(0U, 0U, 2U, 2U) | small::cell_bit(0U, 1U, 2U, 2U) | small::cell_bit(1U, 1U, 2U, 2U);
    ASSERT_EQ(message.satellites.push_back(msm_satellite{}), castle::status::ok);
    ASSERT_EQ(message.satellites.push_back(msm_satellite{}), castle::status::ok);
    EXPECT_EQ(message.satellites.push_back(msm_satellite{}), castle::status::full);
    for (int index = 0; index < 3; ++index)
    {
        ASSERT_EQ(message.signals.push_back(msm_signal{}), castle::status::ok);
    }
    EXPECT_EQ(message.signals.push_back(msm_signal{}), castle::status::full);

    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, 0U, 12U), 1097U);

    // A mask that needs more entries than the capacity holds cannot be satisfied.
    message.satellite_mask |= small::satellite_bit(3U);
    EXPECT_EQ(encode(message).result, castle::status::invalid_argument);
}

TEST(RtcmMsm, SmallestCapacityMessage)
{
    using tiny = msm_message<msm_system::beidou, 4U, 1U, 1U>;
    tiny message = minimal<tiny>();
    EXPECT_EQ(message.satellites.capacity(), 1U);
    EXPECT_EQ(message.signals.capacity(), 1U);
    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(bits_at(frame, 0U, 12U), 1124U);
    EXPECT_EQ(payload_length_of(frame), (minimal_layout<m4>::END + 7U) / 8U);
}

// ---------------------------------------------------------------------------
// Output buffer handling
// ---------------------------------------------------------------------------

TEST(RtcmMsm, ExactFitAndOneByteShortOutput)
{
    const gps_msm4 message = reference_epoch<gps_msm4>(false);
    const encoded reference = encode(message);
    ASSERT_EQ(reference.result, castle::status::ok);
    ASSERT_EQ(reference.size, 59U);

    uint8_t exact[59U] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, exact, sizeof(exact), written), castle::status::ok);
    EXPECT_EQ(written, 59U);
    EXPECT_EQ(memcmp(exact, reference.bytes, 59U), 0);

    uint8_t shorter[58U] = {};
    written = 99U;
    EXPECT_EQ(encode_message(message, shorter, sizeof(shorter), written), castle::status::full);
    EXPECT_EQ(written, 0U);

    // A larger buffer is fine; bytes after the frame stay untouched.
    uint8_t large[80U];
    memset(large, 0xEE, sizeof(large));
    EXPECT_EQ(encode_message(message, large, sizeof(large), written), castle::status::ok);
    EXPECT_EQ(written, 59U);
    EXPECT_EQ(memcmp(large, reference.bytes, 59U), 0);
    EXPECT_EQ(large[59U], 0xEEU);
}

TEST(RtcmMsm, EveryTooSmallBufferReportsFull)
{
    const gps_msm5 message = reference_epoch<gps_msm5>(false);
    const encoded reference = encode(message);
    ASSERT_EQ(reference.result, castle::status::ok);

    uint8_t buffer[RTCM_MAX_FRAME_SIZE];
    for (castle::size_type capacity = 0U; capacity < reference.size; ++capacity)
    {
        castle::size_type written = 99U;
        EXPECT_EQ(encode_message(message, buffer, capacity, written), castle::status::full) << capacity;
        EXPECT_EQ(written, 0U) << capacity;
    }
}

TEST(RtcmMsm, NullOutputIsInvalidArgument)
{
    const gps_msm7 message = reference_epoch<gps_msm7>(false);
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, nullptr, 0U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
    written = 99U;
    EXPECT_EQ(encode_message(message, nullptr, 2000U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmMsm, ZeroSizeBufferIsFull)
{
    const gps_msm7 message = reference_epoch<gps_msm7>(false);
    uint8_t buffer[4U] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(message, buffer, RTCM_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmMsm, EmptyMessageNeedsExactlyItsFrame)
{
    const m4 message{};
    uint8_t buffer[28U] = {};
    castle::size_type written = 0U;
    EXPECT_EQ(encode_message(message, buffer, sizeof(buffer), written), castle::status::ok);
    EXPECT_EQ(written, 28U);
    EXPECT_EQ(encode_message(message, buffer, sizeof(buffer) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmMsm, EncodeIntoSmallPayloadWriterLatchesFull)
{
    const gps_msm4 message = reference_epoch<gps_msm4>(false);
    uint8_t payload[10U] = {};
    bit_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    message.encode(writer);
    EXPECT_FALSE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::full);

    bit_writer rejecting(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    gps_msm4 invalid = message;
    invalid.satellite_mask = 0U;
    invalid.encode(rejecting);
    EXPECT_EQ(rejecting.status(), castle::status::invalid_argument);
    EXPECT_EQ(rejecting.bit_size(), 0U) << "validation runs before any bit is written";
}

TEST(RtcmMsm, EncodingIsDeterministicAndDoesNotModifyTheMessage)
{
    const gps_msm7 message = reference_epoch<gps_msm7>(false);
    const encoded first = encode(message);
    const encoded second = encode(message);
    ASSERT_EQ(first.result, castle::status::ok);
    EXPECT_EQ(first.size, second.size);
    EXPECT_EQ(memcmp(first.bytes, second.bytes, first.size), 0);
}

TEST(RtcmMsm, FrameCrcMatchesCrc24qOverHeaderAndPayload)
{
    const gps_msm7 message = reference_epoch<gps_msm7>(false);
    const encoded frame = encode(message);
    ASSERT_EQ(frame.result, castle::status::ok);
    const castle::size_type covered = frame.size - RTCM_CRC_SIZE;
    const uint32_t crc = castle::checksums::crc24q::calculate(frame.bytes, covered);
    EXPECT_EQ(frame.bytes[covered], static_cast<uint8_t>(crc >> 16U));
    EXPECT_EQ(frame.bytes[covered + 1U], static_cast<uint8_t>(crc >> 8U));
    EXPECT_EQ(frame.bytes[covered + 2U], static_cast<uint8_t>(crc));
    EXPECT_EQ(crc, 0xABA972U) << "golden CRC of message 1077";
}

} // namespace
