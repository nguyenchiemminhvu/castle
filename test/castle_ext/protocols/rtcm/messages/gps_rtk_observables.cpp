#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/gps_rtk_observables.hpp"
#include "castle_ext/checksums/crc24q.hpp"

#include <stdlib.h>
#include <string.h>

#include <string>

namespace
{
using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

using satellite = gps_rtk_satellite;

CASTLE_CONSTEXPR uint8_t FILL = 0xAAU;

// BUILDERS-BEGIN
enum class profile
{
    typical,
    maximum,
    minimum
};

satellite make_satellite(profile kind, uint32_t index)
{
    satellite sat{};
    switch (kind)
    {
    case profile::typical:
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
        break;
    case profile::maximum:
        sat.satellite_id = 63U;
        sat.l1_code = 1U;
        sat.l1_pseudorange = 0xFFFFFFU;
        sat.l1_phase_minus_pseudorange = 524287;
        sat.l1_lock_time = 127U;
        sat.l1_ambiguity = 255U;
        sat.l1_cnr = 255U;
        sat.l2_code = 3U;
        sat.l2_pseudorange_difference = 8191;
        sat.l2_phase_minus_l1_pseudorange = 524287;
        sat.l2_lock_time = 127U;
        sat.l2_cnr = 255U;
        break;
    case profile::minimum:
        sat.l1_phase_minus_pseudorange = -524288;
        sat.l2_pseudorange_difference = -8192;
        sat.l2_phase_minus_l1_pseudorange = -524288;
        break;
    }
    return sat;
}

template <typename Message>
Message make_message(profile kind, uint32_t satellite_count = 2U)
{
    Message value{};
    switch (kind)
    {
    case profile::typical:
        value.station_id = 77U;
        value.tow_ms = 123456789U;
        value.synchronous = true;
        value.divergence_free = true;
        value.smoothing_interval = 5U;
        break;
    case profile::maximum:
        value.station_id = 4095U;
        value.tow_ms = 0x3FFFFFFFU;
        value.synchronous = true;
        value.divergence_free = true;
        value.smoothing_interval = 7U;
        break;
    case profile::minimum:
        break;
    }
    for (uint32_t index = 0U; index < satellite_count; ++index)
    {
        EXPECT_EQ(value.satellites.push_back(make_satellite(kind, index)), castle::status::ok);
    }
    return value;
}
// BUILDERS-END

struct encoded
{
    uint8_t bytes[RTCM_MAX_FRAME_SIZE];
    castle::size_type size;
    castle::status result;
};

template <typename Message>
encoded encode(const Message& value)
{
    encoded out{};
    memset(out.bytes, FILL, sizeof(out.bytes));
    out.size = 12345U;
    out.result = encode_message(value, out.bytes, sizeof(out.bytes), out.size);
    return out;
}

/** @brief Reads `width` (<= 32) bits starting at payload bit `position`, MSB first. */
uint32_t payload_bits(const uint8_t* frame, castle::size_type position, castle::size_type width)
{
    uint32_t value = 0U;
    for (castle::size_type index = 0U; index < width; ++index)
    {
        const castle::size_type bit = position + index;
        value = (value << 1U) | ((frame[RTCM_FRAME_INDEX_PAYLOAD + (bit / 8U)] >> (7U - (bit % 8U))) & 1U);
    }
    return value;
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

void expect_hex(const encoded& frame, const char* hex)
{
    uint8_t expected[RTCM_MAX_FRAME_SIZE];
    const castle::size_type length = parse_hex(hex, expected);
    ASSERT_EQ(frame.result, castle::status::ok);
    ASSERT_EQ(frame.size, length);
    for (castle::size_type index = 0U; index < length; ++index)
    {
        EXPECT_EQ(frame.bytes[index], expected[index]) << "byte " << index;
    }
    EXPECT_EQ(frame.bytes[length], FILL) << "bytes after the frame must stay untouched";
}

/** @brief Checks preamble, length field, message number and CRC-24Q residue of a complete frame. */
void expect_well_formed(const encoded& frame, uint16_t type)
{
    ASSERT_EQ(frame.result, castle::status::ok);
    ASSERT_GE(frame.size, RTCM_FRAME_OVERHEAD);
    EXPECT_EQ(frame.bytes[0U], RTCM_PREAMBLE);
    EXPECT_EQ(frame.bytes[1U] & 0xFCU, 0U);
    EXPECT_EQ((static_cast<castle::size_type>(frame.bytes[1U] & 0x03U) << 8U) | frame.bytes[2U],
              frame.size - RTCM_FRAME_OVERHEAD);
    EXPECT_EQ(payload_bits(frame.bytes, 0U, 12U), type);
    EXPECT_EQ(castle::checksums::crc24q::calculate(frame.bytes, frame.size), 0U);
}

template <typename Message>
void expect_failure(const Message& value, castle::status status)
{
    const encoded frame = encode(value);
    EXPECT_EQ(frame.result, status);
    EXPECT_EQ(frame.size, 0U);
}

/** @brief The four message numbers; tests that hold for each of them are typed over this list. */
using all_types = ::testing::Types<gps_rtk_l1_obs, gps_rtk_l1_obs_ext, gps_rtk_l1_l2_obs, gps_rtk_l1_l2_obs_ext>;

/** @brief Names each typed test instance after its message number, e.g. `RtcmGpsRtkObservablesPerType/1001`. */
struct message_number_name
{
    template <typename Message>
    static std::string GetName(int)
    {
        return std::to_string(Message::MESSAGE_TYPE);
    }
};

template <typename Message>
class RtcmGpsRtkObservablesPerType : public ::testing::Test
{
};
TYPED_TEST_SUITE(RtcmGpsRtkObservablesPerType, all_types, message_number_name);

TEST(RtcmGpsRtkObservables, ConstantsMatchSpec)
{
    EXPECT_DOUBLE_EQ(gps_rtk_l1_obs::PSEUDORANGE_RESOLUTION_M, 0.02);
    EXPECT_DOUBLE_EQ(gps_rtk_l1_obs::PHASE_RESOLUTION_M, 0.0005);
    EXPECT_DOUBLE_EQ(gps_rtk_l1_obs::AMBIGUITY_M, 299792.458);
    EXPECT_DOUBLE_EQ(gps_rtk_l1_obs::CNR_RESOLUTION_DBHZ, 0.25);
    EXPECT_EQ(gps_rtk_l1_l2_obs_ext{}.satellites.capacity(), 31U);
    EXPECT_EQ((gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS_EXT, 4U>{}.satellites.capacity()), 4U);
    EXPECT_EQ(RTCM_TYPE_GPS_L1_OBS, 1001U);
    EXPECT_EQ(RTCM_TYPE_GPS_L1L2_OBS_EXT, 1004U);
}

TEST(RtcmGpsRtkObservables, EachAliasCarriesItsMessageNumberAndFieldSelection)
{
    static_assert(gps_rtk_l1_obs::MESSAGE_TYPE == 1001U, "1001 is the L1 message");
    static_assert(gps_rtk_l1_obs_ext::MESSAGE_TYPE == 1002U, "1002 is the extended L1 message");
    static_assert(gps_rtk_l1_l2_obs::MESSAGE_TYPE == 1003U, "1003 is the L1/L2 message");
    static_assert(gps_rtk_l1_l2_obs_ext::MESSAGE_TYPE == 1004U, "1004 is the extended L1/L2 message");

    static_assert(!gps_rtk_l1_obs::IS_EXTENDED && !gps_rtk_l1_obs::HAS_L2, "1001 carries L1 only");
    static_assert(gps_rtk_l1_obs_ext::IS_EXTENDED && !gps_rtk_l1_obs_ext::HAS_L2, "1002 is extended L1");
    static_assert(!gps_rtk_l1_l2_obs::IS_EXTENDED && gps_rtk_l1_l2_obs::HAS_L2, "1003 carries L2");
    static_assert(gps_rtk_l1_l2_obs_ext::IS_EXTENDED && gps_rtk_l1_l2_obs_ext::HAS_L2, "1004 is extended L1/L2");
    SUCCEED();
}

TEST(RtcmGpsRtkObservables, EachAliasAcceptsOnlyItsOwnNumber)
{
    // Siblings of the family and the GLONASS numbers are rejected as well as unrelated numbers.
    const uint16_t types[] = {0U, 1000U, 1001U, 1002U, 1003U, 1004U, 1005U,
                              1009U, 1010U, 1011U, 1012U, 4072U, 65535U};
    for (const uint16_t type : types)
    {
        EXPECT_EQ(gps_rtk_l1_obs::accepts(type), type == 1001U) << type;
        EXPECT_EQ(gps_rtk_l1_obs_ext::accepts(type), type == 1002U) << type;
        EXPECT_EQ(gps_rtk_l1_l2_obs::accepts(type), type == 1003U) << type;
        EXPECT_EQ(gps_rtk_l1_l2_obs_ext::accepts(type), type == 1004U) << type;
    }
}

TEST(RtcmGpsRtkObservables, AcceptsIsAConstantExpression)
{
    static_assert(gps_rtk_l1_obs::accepts(1001U), "1001 accepts 1001");
    static_assert(!gps_rtk_l1_obs::accepts(1002U), "1001 rejects its sibling 1002");
    static_assert(!gps_rtk_l1_l2_obs_ext::accepts(1012U), "1004 rejects the GLONASS number 1012");
    SUCCEED();
}

TEST(RtcmGpsRtkObservables, MessageNumberIsIndependentOfCapacity)
{
    using small = gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS, 1U>;
    static_assert(small::MESSAGE_TYPE == gps_rtk_l1_l2_obs::MESSAGE_TYPE, "same number as the alias");
    static_assert(small::IS_EXTENDED == gps_rtk_l1_l2_obs::IS_EXTENDED, "same field selection as the alias");
    static_assert(small::HAS_L2 == gps_rtk_l1_l2_obs::HAS_L2, "same field selection as the alias");
    EXPECT_TRUE(small::accepts(1003U));
    EXPECT_FALSE(small::accepts(1004U));
}

struct golden_frame
{
    uint16_t type;
    profile kind;
    const char* hex;
};

// Frames produced by the encoder and decoded and CRC-checked independently with pyrtcm.
// `typical` entries are the reference vectors; `maximum` / `minimum` sit on the field limits.
const golden_frame GOLDEN[] = {
    {1001, profile::typical,
     "d300173e904d1d6f34562d1578c29df9f8f906de30a7fe7e46509fd6ba"},
    {1001, profile::maximum,
     "d300173e9ffffffffffe2ffffffffeffffffffffffffbffffff0b6edfe"},
    {1001, profile::minimum,
     "d300173e9000000000002000000001000000000000004000000096092f"},
    {1002, profile::typical,
     "d3001b3ea04d1d6f34562d1578c29df9f8f911a806de30a7fe7e46547a106a85e9"},
    {1002, profile::maximum,
     "d3001b3eaffffffffffe2ffffffffeffffffffffffffffffbffffffffff09a453d"},
    {1002, profile::minimum,
     "d3001b3ea000000000002000000001000000000000000000400000000000b9815f"},
    {1003, profile::typical,
     "d300223eb04d1d6f34562d1578c29df9f8f92b1e0350c6d0dbc614ffcfc8cb58f21a8656c0752736"},
    {1003, profile::maximum,
     "d300223ebffffffffffe2ffffffffefffffff7ffdffffffffffffff7ffffffbffeffffffc023a1c4"},
    {1003, profile::minimum,
     "d300223eb00000000000200000000100000008002000000000000008000000400100000000aab93b"},
    {1004, profile::typical,
     "d300283ec04d1d6f34562d1578c29df9f8f911a82b1e0350c6d4b0dbc614ffcfc8ca8f4358f21a8656e5c047f73c"},
    {1004, profile::maximum,
     "d300283ecffffffffffe2ffffffffefffffffffff7ffdffffffffffffffff7ffffffffffbffeffffffffc0f4bfb1"},
    {1004, profile::minimum,
     "d300283ec00000000000200000000100000000000800200000000000000008000000000040010000000000f7ca28"},
};

TYPED_TEST(RtcmGpsRtkObservablesPerType, GoldenFramesMatchIndependentlyDecodedBytes)
{
    castle::size_type checked = 0U;
    for (const golden_frame& golden : GOLDEN)
    {
        if (golden.type == TypeParam::MESSAGE_TYPE)
        {
            SCOPED_TRACE(::testing::Message() << "profile " << static_cast<int>(golden.kind));
            expect_hex(encode(make_message<TypeParam>(golden.kind)), golden.hex);
            ++checked;
        }
    }
    EXPECT_EQ(checked, 3U); // typical, maximum, minimum
}

TEST(RtcmGpsRtkObservables, DefaultMessageHasHeaderOnly)
{
    // Zero satellites: just the 64 header bits, an 8 byte payload.
    const encoded frame = encode(gps_rtk_l1_l2_obs_ext{});
    expect_hex(frame, "d300083ec00000000000009f5575");
    expect_well_formed(frame, 1004U);
    EXPECT_EQ(frame.size, frame_size(8U));
    EXPECT_EQ(payload_bits(frame.bytes, 12U + 12U + 30U + 1U, 5U), 0U);
}

TEST(RtcmGpsRtkObservables, HeaderFieldsAreWrittenMsbFirstAtSpecPositions)
{
    gps_rtk_l1_obs value = make_message<gps_rtk_l1_obs>(profile::typical, 1U);
    const encoded frame = encode(value);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_bits(frame.bytes, 0U, 12U), 1001U);
    EXPECT_EQ(payload_bits(frame.bytes, 12U, 12U), 77U);
    EXPECT_EQ(payload_bits(frame.bytes, 24U, 30U), 123456789U);
    EXPECT_EQ(payload_bits(frame.bytes, 54U, 1U), 1U); // DF005
    EXPECT_EQ(payload_bits(frame.bytes, 55U, 5U), 1U); // DF006
    EXPECT_EQ(payload_bits(frame.bytes, 60U, 1U), 1U); // DF007
    EXPECT_EQ(payload_bits(frame.bytes, 61U, 3U), 5U); // DF008
    EXPECT_EQ(payload_bits(frame.bytes, 64U, 6U), 5U); // DF009
    EXPECT_EQ(payload_bits(frame.bytes, 70U, 1U), 0U); // DF010
    EXPECT_EQ(payload_bits(frame.bytes, 71U, 24U), 12345678U);
    EXPECT_EQ(payload_bits(frame.bytes, 95U, 20U), static_cast<uint32_t>(-12345) & 0xFFFFFU);
    EXPECT_EQ(payload_bits(frame.bytes, 115U, 7U), 100U);

    value.synchronous = false;
    value.divergence_free = false;
    const encoded cleared = encode(value);
    EXPECT_EQ(payload_bits(cleared.bytes, 54U, 1U), 0U);
    EXPECT_EQ(payload_bits(cleared.bytes, 60U, 1U), 0U);
}

TEST(RtcmGpsRtkObservables, SatelliteCountIsTheVectorSizeInFiveBits)
{
    for (castle::size_type count = 0U; count <= 31U; ++count)
    {
        SCOPED_TRACE(count);
        gps_rtk_l1_l2_obs_ext value{};
        for (castle::size_type index = 0U; index < count; ++index)
        {
            ASSERT_EQ(value.satellites.push_back(make_satellite(profile::typical, 0U)), castle::status::ok);
        }
        const encoded frame = encode(value);
        ASSERT_EQ(frame.result, castle::status::ok);
        EXPECT_EQ(payload_bits(frame.bytes, 55U, 5U), count);
    }
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, FrameSizeFollowsPerTypeBitBudget)
{
    // 1001: 58 bit, 1002: 74 bit, 1003: 101 bit, 1004: 125 bit per satellite after 64 header bits.
    const castle::size_type per_satellite[] = {58U, 74U, 101U, 125U};
    const uint16_t type = TypeParam::MESSAGE_TYPE;
    for (castle::size_type count = 0U; count <= 31U; ++count)
    {
        SCOPED_TRACE(::testing::Message() << "satellites " << count);
        const castle::size_type bits = 64U + (count * per_satellite[type - 1001U]);
        const castle::size_type payload_bytes = (bits + 7U) / 8U;

        const encoded frame = encode(make_message<TypeParam>(profile::maximum, static_cast<uint32_t>(count)));
        expect_well_formed(frame, type);
        ASSERT_EQ(frame.size, frame_size(payload_bytes));

        // Unused bits of the last payload byte are zero, even in a pre-filled buffer.
        const castle::size_type padding = (payload_bytes * 8U) - bits;
        const uint8_t last = frame.bytes[RTCM_FRAME_INDEX_PAYLOAD + payload_bytes - 1U];
        EXPECT_EQ(last & ((1U << padding) - 1U), 0U);
    }
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, EveryFrameEndsWithAValidCrc)
{
    const profile kinds[] = {profile::typical, profile::maximum, profile::minimum};
    for (const profile kind : kinds)
    {
        expect_well_formed(encode(make_message<TypeParam>(kind)), TypeParam::MESSAGE_TYPE);
    }
}

TEST(RtcmGpsRtkObservables, ThirtyOneSatellitesFillTheFiveBitCountExactly)
{
    gps_rtk_l1_l2_obs_ext value{};
    value.station_id = 4095U;
    value.tow_ms = 604799999U;
    for (uint32_t index = 0U; index < 31U; ++index)
    {
        satellite sat = make_satellite(profile::typical, index);
        sat.satellite_id = static_cast<uint8_t>(index + 1U);
        sat.l1_code = static_cast<uint8_t>(index & 1U);
        sat.l1_lock_time = static_cast<uint8_t>(50U + index);
        ASSERT_EQ(value.satellites.push_back(sat), castle::status::ok);
    }
    EXPECT_EQ(value.satellites.size(), 31U);
    EXPECT_TRUE(value.satellites.full());

    const encoded frame = encode(value);
    expect_well_formed(frame, 1004U);
    EXPECT_EQ(frame.size, frame_size(493U)); // (64 + 31 * 125) bits = 3939 bits.
    EXPECT_EQ(payload_bits(frame.bytes, 55U, 5U), 31U);
    EXPECT_EQ(payload_bits(frame.bytes, 64U, 6U), 1U);
    EXPECT_EQ(payload_bits(frame.bytes, 64U + (30U * 125U), 6U), 31U);
    // CRC-24Q of the frame, taken from the independently checked pyrtcm decode.
    EXPECT_EQ(frame.bytes[frame.size - 3U], 0xA0U);
    EXPECT_EQ(frame.bytes[frame.size - 2U], 0x8CU);
    EXPECT_EQ(frame.bytes[frame.size - 1U], 0x25U);
}

TEST(RtcmGpsRtkObservables, SatelliteCapacityIsEnforcedByTheContainer)
{
    gps_rtk_l1_l2_obs_ext value{};
    for (uint32_t index = 0U; index < 31U; ++index)
    {
        ASSERT_EQ(value.satellites.push_back(satellite{}), castle::status::ok);
    }
    EXPECT_EQ(value.satellites.push_back(satellite{}), castle::status::full);
    EXPECT_EQ(value.satellites.size(), 31U);

    gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS_EXT, 1U> single{};
    ASSERT_EQ(single.satellites.push_back(make_satellite(profile::typical, 0U)), castle::status::ok);
    EXPECT_EQ(single.satellites.push_back(make_satellite(profile::typical, 1U)), castle::status::full);
    const encoded frame = encode(single);
    EXPECT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_bits(frame.bytes, 55U, 5U), 1U);
}

TEST(RtcmGpsRtkObservables, CapacityVariantsEncodeIdenticallyToTheDefault)
{
    gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS_EXT, 2U> small{};
    const gps_rtk_l1_l2_obs_ext reference = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical, 2U);
    small.station_id = reference.station_id;
    small.tow_ms = reference.tow_ms;
    small.synchronous = reference.synchronous;
    small.divergence_free = reference.divergence_free;
    small.smoothing_interval = reference.smoothing_interval;
    for (const satellite& sat : reference.satellites)
    {
        ASSERT_EQ(small.satellites.push_back(sat), castle::status::ok);
    }
    const encoded a = encode(small);
    const encoded b = encode(reference);
    ASSERT_EQ(a.result, castle::status::ok);
    ASSERT_EQ(a.size, b.size);
    EXPECT_EQ(memcmp(a.bytes, b.bytes, a.size), 0);
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, FieldsAreIgnoredForTypesThatDoNotCarryThem)
{
    // Each type produces identical bytes whatever the ignored fields hold, and ignored
    // out-of-range raw values are not validated.
    const TypeParam baseline = make_message<TypeParam>(profile::typical);
    TypeParam noisy = baseline;
    for (satellite& sat : noisy.satellites)
    {
        if (!TypeParam::IS_EXTENDED)
        {
            sat.l1_ambiguity = 0xFFU;
            sat.l1_cnr = 0xFFU;
            sat.l2_cnr = 0xFFU;
        }
        if (!TypeParam::HAS_L2)
        {
            sat.l2_code = 200U;                          // does not fit 2 bits
            sat.l2_pseudorange_difference = 32767;       // does not fit 14 bits
            sat.l2_phase_minus_l1_pseudorange = 1 << 30; // does not fit 20 bits
            sat.l2_lock_time = 255U;                     // does not fit 7 bits
        }
    }
    const encoded a = encode(baseline);
    const encoded b = encode(noisy);
    ASSERT_EQ(a.result, castle::status::ok);
    ASSERT_EQ(b.result, castle::status::ok);
    ASSERT_EQ(a.size, b.size);
    EXPECT_EQ(memcmp(a.bytes, b.bytes, a.size), 0);
}

struct field_case
{
    const char* name;
    void (*mutate)(satellite&);
    unsigned carried_by; ///< Bit n set: message type 1001 + n writes the field.
};

const field_case FIELD_CASES[] = {
    {"satellite_id", [](satellite& s) { s.satellite_id = static_cast<uint8_t>(s.satellite_id + 1U); }, 0xFU},
    {"l1_code", [](satellite& s) { s.l1_code = static_cast<uint8_t>(s.l1_code ^ 1U); }, 0xFU},
    {"l1_pseudorange", [](satellite& s) { s.l1_pseudorange += 1U; }, 0xFU},
    {"l1_phase_minus_pseudorange", [](satellite& s) { s.l1_phase_minus_pseudorange += 1; }, 0xFU},
    {"l1_lock_time", [](satellite& s) { s.l1_lock_time = static_cast<uint8_t>(s.l1_lock_time + 1U); }, 0xFU},
    {"l1_ambiguity", [](satellite& s) { s.l1_ambiguity = static_cast<uint8_t>(s.l1_ambiguity + 1U); }, 0xAU},
    {"l1_cnr", [](satellite& s) { s.l1_cnr = static_cast<uint8_t>(s.l1_cnr + 1U); }, 0xAU},
    {"l2_code", [](satellite& s) { s.l2_code = static_cast<uint8_t>(s.l2_code ^ 1U); }, 0xCU},
    {"l2_pseudorange_difference",
     [](satellite& s) { s.l2_pseudorange_difference = static_cast<int16_t>(s.l2_pseudorange_difference + 1); },
     0xCU},
    {"l2_phase_minus_l1_pseudorange", [](satellite& s) { s.l2_phase_minus_l1_pseudorange += 1; }, 0xCU},
    {"l2_lock_time", [](satellite& s) { s.l2_lock_time = static_cast<uint8_t>(s.l2_lock_time + 1U); }, 0xCU},
    {"l2_cnr", [](satellite& s) { s.l2_cnr = static_cast<uint8_t>(s.l2_cnr + 1U); }, 0x8U},
};

TYPED_TEST(RtcmGpsRtkObservablesPerType, EachSatelliteFieldIsCarriedExactlyByItsMessageTypes)
{
    for (const field_case& field : FIELD_CASES)
    {
        SCOPED_TRACE(field.name);
        const TypeParam baseline = make_message<TypeParam>(profile::typical, 1U);
        TypeParam changed = baseline;
        field.mutate(changed.satellites[0U]);

        const encoded a = encode(baseline);
        const encoded b = encode(changed);
        ASSERT_EQ(a.result, castle::status::ok);
        ASSERT_EQ(b.result, castle::status::ok);
        ASSERT_EQ(a.size, b.size);
        const bool carried = ((field.carried_by >> (TypeParam::MESSAGE_TYPE - 1001U)) & 1U) != 0U;
        const bool differs = memcmp(a.bytes, b.bytes, a.size) != 0;
        EXPECT_EQ(differs, carried);
    }
}

TEST(RtcmGpsRtkObservables, StationIdBoundaries)
{
    gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical);
    value.station_id = 4095U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
    value.station_id = 4096U;
    expect_failure(value, castle::status::out_of_range);
    value.station_id = 65535U;
    expect_failure(value, castle::status::out_of_range);
    value.station_id = 0U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
}

TEST(RtcmGpsRtkObservables, TimeOfWeekBoundaries)
{
    gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical);
    value.tow_ms = 0x3FFFFFFFU;
    EXPECT_EQ(encode(value).result, castle::status::ok);
    value.tow_ms = 0x40000000U;
    expect_failure(value, castle::status::out_of_range);
    value.tow_ms = 0xFFFFFFFFU;
    expect_failure(value, castle::status::out_of_range);
    value.tow_ms = 0U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
}

TEST(RtcmGpsRtkObservables, SmoothingIntervalBoundaries)
{
    gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical);
    value.smoothing_interval = 7U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
    value.smoothing_interval = 8U;
    expect_failure(value, castle::status::out_of_range);
    value.smoothing_interval = 255U;
    expect_failure(value, castle::status::out_of_range);
    value.smoothing_interval = 0U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, HeaderFieldBoundariesApplyToEveryType)
{
    TypeParam value = make_message<TypeParam>(profile::typical);
    value.station_id = 4096U;
    expect_failure(value, castle::status::out_of_range);
    value.station_id = 1U;
    value.tow_ms = 0x40000000U;
    expect_failure(value, castle::status::out_of_range);
    value.tow_ms = 1U;
    value.smoothing_interval = 8U;
    expect_failure(value, castle::status::out_of_range);
}

/** @brief Applies `set` to the second satellite of an otherwise valid type 1004 message and checks the status. */
template <typename Setter>
void expect_satellite_field(Setter set, castle::status status)
{
    gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical, 2U);
    set(value.satellites[1U]);
    const encoded frame = encode(value);
    EXPECT_EQ(frame.result, status);
    if (status == castle::status::ok)
    {
        EXPECT_GT(frame.size, 0U);
    }
    else
    {
        EXPECT_EQ(frame.size, 0U);
    }
}

TEST(RtcmGpsRtkObservables, SatelliteIdBoundaries)
{
    expect_satellite_field([](satellite& s) { s.satellite_id = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.satellite_id = 63U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.satellite_id = 64U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.satellite_id = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L1CodeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_code = 1U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_code = 2U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_code = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L1PseudorangeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_pseudorange = 0xFFFFFFU; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_pseudorange = 0x1000000U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_pseudorange = 0xFFFFFFFFU; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L1PhaseMinusPseudorangeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = 524287; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = -524288; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = 524288; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = -524289; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = INT32_MAX; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = INT32_MIN; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L1LockTimeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_lock_time = 127U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_lock_time = 128U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_lock_time = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, EightBitFieldsAcceptTheWholeByteRange)
{
    expect_satellite_field([](satellite& s) { s.l1_ambiguity = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_ambiguity = 255U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_cnr = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_cnr = 255U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_cnr = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_cnr = 255U; }, castle::status::ok);
}

TEST(RtcmGpsRtkObservables, L2CodeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_code = 3U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_code = 4U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_code = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L2PseudorangeDifferenceBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = 8191; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = -8192; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = 8192; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = -8193; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = INT16_MAX; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = INT16_MIN; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L2PhaseMinusL1PseudorangeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = 524287; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = -524288; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = 524288; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = -524289; }, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, L2LockTimeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_lock_time = 127U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_lock_time = 128U; }, castle::status::out_of_range);
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, SatelliteFieldLimitsOnlyApplyToTypesThatCarryThem)
{
    // L2 lock time 128 is invalid in 1003/1004 but ignored in 1001/1002.
    TypeParam value = make_message<TypeParam>(profile::typical, 1U);
    value.satellites[0U].l2_lock_time = 128U;
    const encoded frame = encode(value);
    EXPECT_EQ(frame.result, TypeParam::HAS_L2 ? castle::status::out_of_range : castle::status::ok);
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, ExactFitOutputBufferSucceeds)
{
    const TypeParam value = make_message<TypeParam>(profile::typical);
    const encoded reference = encode(value);
    ASSERT_EQ(reference.result, castle::status::ok);

    uint8_t exact[RTCM_MAX_FRAME_SIZE];
    memset(exact, FILL, sizeof(exact));
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(value, exact, reference.size, written), castle::status::ok);
    EXPECT_EQ(written, reference.size);
    EXPECT_EQ(memcmp(exact, reference.bytes, reference.size), 0);
    EXPECT_EQ(exact[reference.size], FILL);
}

TYPED_TEST(RtcmGpsRtkObservablesPerType, OneByteShortOutputBufferReportsFull)
{
    const TypeParam value = make_message<TypeParam>(profile::typical);
    const encoded reference = encode(value);
    ASSERT_EQ(reference.result, castle::status::ok);

    uint8_t buffer[RTCM_MAX_FRAME_SIZE];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, buffer, reference.size - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmGpsRtkObservables, TinyOutputBuffersReportFull)
{
    const gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical);
    uint8_t buffer[RTCM_FRAME_OVERHEAD + 8U] = {};
    castle::size_type written = 99U;

    EXPECT_EQ(encode_message(value, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(value, buffer, RTCM_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(encode_message(value, buffer, RTCM_FRAME_OVERHEAD, written), castle::status::full);
    EXPECT_EQ(encode_message(value, buffer, RTCM_FRAME_OVERHEAD + 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    // A header-only payload fits an 8 byte slot exactly; the first satellite then overflows.
    EXPECT_EQ(encode_message(gps_rtk_l1_l2_obs_ext{}, buffer, sizeof(buffer), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(buffer));
    EXPECT_EQ(encode_message(value, buffer, sizeof(buffer), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmGpsRtkObservables, OutOfRangeValueIsReportedBeforeRunningOutOfRoom)
{
    // In an 8 byte payload slot a valid first satellite overflows (full) but an invalid one is
    // diagnosed as out_of_range because the value is checked before the space.
    uint8_t buffer[RTCM_FRAME_OVERHEAD + 8U] = {};
    castle::size_type written = 99U;

    gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical, 1U);
    EXPECT_EQ(encode_message(value, buffer, sizeof(buffer), written), castle::status::full);
    value.satellites[0U].satellite_id = 64U;
    EXPECT_EQ(encode_message(value, buffer, sizeof(buffer), written), castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmGpsRtkObservables, FirstFailureEndsTheMessage)
{
    // Out-of-range station and satellite together: the first failure is kept, nothing is written.
    gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical);
    value.station_id = 4096U;
    value.satellites[0U].satellite_id = 64U;
    expect_failure(value, castle::status::out_of_range);
}

TEST(RtcmGpsRtkObservables, NullOutputIsAnInvalidArgument)
{
    const gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::typical);
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, static_cast<uint8_t*>(nullptr), 100U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(value, static_cast<uint8_t*>(nullptr), 0U, written), castle::status::invalid_argument);
}

TEST(RtcmGpsRtkObservables, EncodingIsDeterministicAndDoesNotModifyTheMessage)
{
    const gps_rtk_l1_l2_obs_ext value = make_message<gps_rtk_l1_l2_obs_ext>(profile::maximum, 3U);
    const encoded first = encode(value);
    const encoded second = encode(value);
    ASSERT_EQ(first.result, castle::status::ok);
    ASSERT_EQ(first.size, second.size);
    EXPECT_EQ(memcmp(first.bytes, second.bytes, first.size), 0);
    EXPECT_EQ(value.satellites.size(), 3U);
    EXPECT_EQ(value.tow_ms, 0x3FFFFFFFU);
}

TEST(RtcmGpsRtkObservables, RawIntegersAreWrittenWithoutScaling)
{
    gps_rtk_l1_obs value = make_message<gps_rtk_l1_obs>(profile::minimum, 1U);
    value.satellites[0U].l1_pseudorange = 1U;
    const encoded frame = encode(value);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_bits(frame.bytes, 71U, 24U), 1U);
    EXPECT_NEAR(static_cast<double>(value.satellites[0U].l1_pseudorange) * gps_rtk_l1_obs::PSEUDORANGE_RESOLUTION_M, 0.02, 1e-12);
}

} // namespace
