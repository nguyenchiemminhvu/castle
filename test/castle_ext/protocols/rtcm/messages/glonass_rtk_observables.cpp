#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp"
#include "castle_ext/checksums/crc24q.hpp"

#include <stdlib.h>
#include <string.h>

#include <string>

namespace
{
using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

using satellite = glonass_rtk_satellite;

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
        break;
    case profile::maximum:
        sat.satellite_id = 63U;
        sat.l1_code = 1U;
        sat.frequency_channel = 31U;
        sat.l1_pseudorange = 0x1FFFFFFU;
        sat.l1_phase_minus_pseudorange = 524287;
        sat.l1_lock_time = 127U;
        sat.l1_ambiguity = 127U;
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
        value.time_of_day_ms = 12345678U;
        value.synchronous = false;
        value.divergence_free = true;
        value.smoothing_interval = 5U;
        break;
    case profile::maximum:
        value.station_id = 4095U;
        value.time_of_day_ms = 0x7FFFFFFU;
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
using all_types =
    ::testing::Types<glonass_rtk_l1_obs, glonass_rtk_l1_obs_ext, glonass_rtk_l1_l2_obs, glonass_rtk_l1_l2_obs_ext>;

/** @brief Names each typed test instance after its message number, e.g. `RtcmGlonassRtkObservablesPerType/1009`. */
struct message_number_name
{
    template <typename Message>
    static std::string GetName(int)
    {
        return std::to_string(Message::MESSAGE_TYPE);
    }
};

template <typename Message>
class RtcmGlonassRtkObservablesPerType : public ::testing::Test
{
};
TYPED_TEST_SUITE(RtcmGlonassRtkObservablesPerType, all_types, message_number_name);

TEST(RtcmGlonassRtkObservables, ConstantsMatchSpec)
{
    EXPECT_DOUBLE_EQ(glonass_rtk_l1_obs::PSEUDORANGE_RESOLUTION_M, 0.02);
    EXPECT_DOUBLE_EQ(glonass_rtk_l1_obs::PHASE_RESOLUTION_M, 0.0005);
    EXPECT_DOUBLE_EQ(glonass_rtk_l1_obs::AMBIGUITY_M, 599584.916);
    EXPECT_DOUBLE_EQ(glonass_rtk_l1_obs::CNR_RESOLUTION_DBHZ, 0.25);
    EXPECT_EQ(glonass_rtk_l1_l2_obs_ext{}.satellites.capacity(), 31U);
    EXPECT_EQ((glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 4U>{}.satellites.capacity()), 4U);
    EXPECT_EQ(RTCM_TYPE_GLO_L1_OBS, 1009U);
    EXPECT_EQ(RTCM_TYPE_GLO_L1L2_OBS_EXT, 1012U);
}

TEST(RtcmGlonassRtkObservables, EachAliasCarriesItsMessageNumberAndFieldSelection)
{
    static_assert(glonass_rtk_l1_obs::MESSAGE_TYPE == 1009U, "1009 is the L1 message");
    static_assert(glonass_rtk_l1_obs_ext::MESSAGE_TYPE == 1010U, "1010 is the extended L1 message");
    static_assert(glonass_rtk_l1_l2_obs::MESSAGE_TYPE == 1011U, "1011 is the L1/L2 message");
    static_assert(glonass_rtk_l1_l2_obs_ext::MESSAGE_TYPE == 1012U, "1012 is the extended L1/L2 message");

    static_assert(!glonass_rtk_l1_obs::IS_EXTENDED && !glonass_rtk_l1_obs::HAS_L2, "1009 carries L1 only");
    static_assert(glonass_rtk_l1_obs_ext::IS_EXTENDED && !glonass_rtk_l1_obs_ext::HAS_L2, "1010 is extended L1");
    static_assert(!glonass_rtk_l1_l2_obs::IS_EXTENDED && glonass_rtk_l1_l2_obs::HAS_L2, "1011 carries L2");
    static_assert(glonass_rtk_l1_l2_obs_ext::IS_EXTENDED && glonass_rtk_l1_l2_obs_ext::HAS_L2, "1012 is extended L1/L2");
    SUCCEED();
}

TEST(RtcmGlonassRtkObservables, EachAliasAcceptsOnlyItsOwnNumber)
{
    // Siblings of the family and the GPS numbers are rejected as well as unrelated numbers.
    const uint16_t types[] = {0U, 1001U, 1002U, 1003U, 1004U, 1008U, 1009U,
                              1010U, 1011U, 1012U, 1013U, 4072U, 65535U};
    for (const uint16_t type : types)
    {
        EXPECT_EQ(glonass_rtk_l1_obs::accepts(type), type == 1009U) << type;
        EXPECT_EQ(glonass_rtk_l1_obs_ext::accepts(type), type == 1010U) << type;
        EXPECT_EQ(glonass_rtk_l1_l2_obs::accepts(type), type == 1011U) << type;
        EXPECT_EQ(glonass_rtk_l1_l2_obs_ext::accepts(type), type == 1012U) << type;
    }
}

TEST(RtcmGlonassRtkObservables, AcceptsIsAConstantExpression)
{
    static_assert(glonass_rtk_l1_obs::accepts(1009U), "1009 accepts 1009");
    static_assert(!glonass_rtk_l1_obs::accepts(1010U), "1009 rejects its sibling 1010");
    static_assert(!glonass_rtk_l1_l2_obs_ext::accepts(1004U), "1012 rejects the GPS number 1004");
    SUCCEED();
}

TEST(RtcmGlonassRtkObservables, MessageNumberIsIndependentOfCapacity)
{
    using small = glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS, 1U>;
    static_assert(small::MESSAGE_TYPE == glonass_rtk_l1_l2_obs::MESSAGE_TYPE, "same number as the alias");
    static_assert(small::IS_EXTENDED == glonass_rtk_l1_l2_obs::IS_EXTENDED, "same field selection as the alias");
    static_assert(small::HAS_L2 == glonass_rtk_l1_l2_obs::HAS_L2, "same field selection as the alias");
    EXPECT_TRUE(small::accepts(1011U));
    EXPECT_FALSE(small::accepts(1012U));
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
    {1009, profile::typical,
     "d300183f104d178c29c168a1d53df3bf3f1f20d2553df3ff3f232848b73c"},
    {1009, profile::maximum,
     "d300183f1ffffffffff17fffffffffdfffffffffffffffdffffff8c56025"},
    {1009, profile::minimum,
     "d300183f10000000000100000000002000000000000000200000007b515f"},
    {1010, profile::typical,
     "d3001c3f204d178c29c168a1d53df3bf3f1f246a01a4aa7be7fe7e4658f4209c5141"},
    {1010, profile::maximum,
     "d3001c3f2ffffffffff17fffffffffdfffffffffffffffffffbfffffffffe08e4f32"},
    {1010, profile::minimum,
     "d3001c3f2000000000010000000000200000000000000000004000000000008fd92d"},
    {1011, profile::typical,
     "d300233f304d178c29c168a1d53df3bf3f1f2363c06a18da1a4aa7be7fe7e4656c790d432b60030f72"},
    {1011, profile::maximum,
     "d300233f3ffffffffff17fffffffffdffffffefffbfffffffffffffffbffffffdfff7fffffe0f942d0"},
    {1011, profile::minimum,
     "d300233f300000000001000000000020000001000400000000000000040000002000800000003eabc7"},
    {1012, profile::typical,
     "d300293f404d178c29c168a1d53df3bf3f1f246a06c780d431b52c34954f7cffcfc8cb1e85b1e4350cadcb803e34af"},
    {1012, profile::maximum,
     "d300293f4ffffffffff17fffffffffdffffffffffdfff7fffffffffffffffff7ffffffffff7ffdffffffff8058e960"},
    {1012, profile::minimum,
     "d300293f400000000001000000000020000000000200080000000000000000080000000000800200000000004083a6"},
};

TYPED_TEST(RtcmGlonassRtkObservablesPerType, GoldenFramesMatchIndependentlyDecodedBytes)
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

TEST(RtcmGlonassRtkObservables, DefaultMessageHasHeaderOnly)
{
    // Zero satellites: just the 61 header bits, padded to an 8 byte payload.
    const encoded frame = encode(glonass_rtk_l1_l2_obs_ext{});
    expect_hex(frame, "d300083f40000000000000d3fd70");
    expect_well_formed(frame, 1012U);
    EXPECT_EQ(frame.size, frame_size(8U));
    EXPECT_EQ(payload_bits(frame.bytes, 12U + 12U + 27U + 1U, 5U), 0U);
}

TEST(RtcmGlonassRtkObservables, HeaderFieldsAreWrittenMsbFirstAtSpecPositions)
{
    glonass_rtk_l1_obs value = make_message<glonass_rtk_l1_obs>(profile::typical, 1U);
    value.synchronous = true;
    const encoded frame = encode(value);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_bits(frame.bytes, 0U, 12U), 1009U);
    EXPECT_EQ(payload_bits(frame.bytes, 12U, 12U), 77U);
    EXPECT_EQ(payload_bits(frame.bytes, 24U, 27U), 12345678U); // DF034
    EXPECT_EQ(payload_bits(frame.bytes, 51U, 1U), 1U);         // DF005
    EXPECT_EQ(payload_bits(frame.bytes, 52U, 5U), 1U);         // DF035
    EXPECT_EQ(payload_bits(frame.bytes, 57U, 1U), 1U);         // DF036
    EXPECT_EQ(payload_bits(frame.bytes, 58U, 3U), 5U);         // DF037
    EXPECT_EQ(payload_bits(frame.bytes, 61U, 6U), 5U);         // DF038
    EXPECT_EQ(payload_bits(frame.bytes, 67U, 1U), 0U);         // DF039
    EXPECT_EQ(payload_bits(frame.bytes, 68U, 5U), 3U);         // DF040
    EXPECT_EQ(payload_bits(frame.bytes, 73U, 25U), 22345678U); // DF041
    EXPECT_EQ(payload_bits(frame.bytes, 98U, 20U), static_cast<uint32_t>(-12345) & 0xFFFFFU); // DF042
    EXPECT_EQ(payload_bits(frame.bytes, 118U, 7U), 100U);      // DF043

    value.synchronous = false;
    value.divergence_free = false;
    const encoded cleared = encode(value);
    EXPECT_EQ(payload_bits(cleared.bytes, 51U, 1U), 0U);
    EXPECT_EQ(payload_bits(cleared.bytes, 57U, 1U), 0U);
}

TEST(RtcmGlonassRtkObservables, SatelliteCountIsTheVectorSizeInFiveBits)
{
    for (castle::size_type count = 0U; count <= 31U; ++count)
    {
        SCOPED_TRACE(count);
        glonass_rtk_l1_l2_obs_ext value{};
        for (castle::size_type index = 0U; index < count; ++index)
        {
            ASSERT_EQ(value.satellites.push_back(make_satellite(profile::typical, 0U)), castle::status::ok);
        }
        const encoded frame = encode(value);
        ASSERT_EQ(frame.result, castle::status::ok);
        EXPECT_EQ(payload_bits(frame.bytes, 52U, 5U), count);
    }
}

TYPED_TEST(RtcmGlonassRtkObservablesPerType, FrameSizeFollowsPerTypeBitBudget)
{
    // 1009: 64 bit, 1010: 79 bit, 1011: 107 bit, 1012: 130 bit per satellite after 61 header bits.
    const castle::size_type per_satellite[] = {64U, 79U, 107U, 130U};
    const uint16_t type = TypeParam::MESSAGE_TYPE;
    for (castle::size_type count = 0U; count <= 31U; ++count)
    {
        SCOPED_TRACE(::testing::Message() << "satellites " << count);
        const castle::size_type bits = 61U + (count * per_satellite[type - 1009U]);
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

TYPED_TEST(RtcmGlonassRtkObservablesPerType, EveryFrameEndsWithAValidCrc)
{
    const profile kinds[] = {profile::typical, profile::maximum, profile::minimum};
    for (const profile kind : kinds)
    {
        expect_well_formed(encode(make_message<TypeParam>(kind)), TypeParam::MESSAGE_TYPE);
    }
}

TEST(RtcmGlonassRtkObservables, ThirtyOneSatellitesFillTheFiveBitCountExactly)
{
    glonass_rtk_l1_l2_obs_ext value{};
    value.station_id = 4095U;
    value.time_of_day_ms = 86399999U;
    for (uint32_t index = 0U; index < 31U; ++index)
    {
        satellite sat = make_satellite(profile::typical, index);
        sat.satellite_id = static_cast<uint8_t>(index + 1U);
        sat.l1_code = static_cast<uint8_t>(index & 1U);
        sat.frequency_channel = static_cast<uint8_t>(3U + (index % 20U));
        sat.l1_lock_time = static_cast<uint8_t>(50U + index);
        ASSERT_EQ(value.satellites.push_back(sat), castle::status::ok);
    }
    EXPECT_EQ(value.satellites.size(), 31U);
    EXPECT_TRUE(value.satellites.full());

    const encoded frame = encode(value);
    expect_well_formed(frame, 1012U);
    EXPECT_EQ(frame.size, frame_size(512U)); // (61 + 31 * 130) bits = 4091 bits.
    EXPECT_EQ(payload_bits(frame.bytes, 52U, 5U), 31U);
    EXPECT_EQ(payload_bits(frame.bytes, 61U, 6U), 1U);
    EXPECT_EQ(payload_bits(frame.bytes, 61U + (30U * 130U), 6U), 31U);
    // CRC-24Q of the frame, taken from the independently checked pyrtcm decode.
    EXPECT_EQ(frame.bytes[frame.size - 3U], 0x97U);
    EXPECT_EQ(frame.bytes[frame.size - 2U], 0xF1U);
    EXPECT_EQ(frame.bytes[frame.size - 1U], 0xD7U);
}

TEST(RtcmGlonassRtkObservables, SatelliteCapacityIsEnforcedByTheContainer)
{
    glonass_rtk_l1_l2_obs_ext value{};
    for (uint32_t index = 0U; index < 31U; ++index)
    {
        ASSERT_EQ(value.satellites.push_back(satellite{}), castle::status::ok);
    }
    EXPECT_EQ(value.satellites.push_back(satellite{}), castle::status::full);
    EXPECT_EQ(value.satellites.size(), 31U);

    glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 1U> single{};
    ASSERT_EQ(single.satellites.push_back(make_satellite(profile::typical, 0U)), castle::status::ok);
    EXPECT_EQ(single.satellites.push_back(make_satellite(profile::typical, 1U)), castle::status::full);
    const encoded frame = encode(single);
    EXPECT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_bits(frame.bytes, 52U, 5U), 1U);
}

TEST(RtcmGlonassRtkObservables, CapacityVariantsEncodeIdenticallyToTheDefault)
{
    glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 2U> small{};
    const glonass_rtk_l1_l2_obs_ext reference = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical, 2U);
    small.station_id = reference.station_id;
    small.time_of_day_ms = reference.time_of_day_ms;
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

TYPED_TEST(RtcmGlonassRtkObservablesPerType, FieldsAreIgnoredForTypesThatDoNotCarryThem)
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
    unsigned carried_by; ///< Bit n set: message type 1009 + n writes the field.
};

const field_case FIELD_CASES[] = {
    {"satellite_id", [](satellite& s) { s.satellite_id = static_cast<uint8_t>(s.satellite_id + 1U); }, 0xFU},
    {"l1_code", [](satellite& s) { s.l1_code = static_cast<uint8_t>(s.l1_code ^ 1U); }, 0xFU},
    {"frequency_channel", [](satellite& s) { s.frequency_channel = static_cast<uint8_t>(s.frequency_channel + 1U); }, 0xFU},
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

TYPED_TEST(RtcmGlonassRtkObservablesPerType, EachSatelliteFieldIsCarriedExactlyByItsMessageTypes)
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
        const bool carried = ((field.carried_by >> (TypeParam::MESSAGE_TYPE - 1009U)) & 1U) != 0U;
        const bool differs = memcmp(a.bytes, b.bytes, a.size) != 0;
        EXPECT_EQ(differs, carried);
    }
}

TEST(RtcmGlonassRtkObservables, StationIdBoundaries)
{
    glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical);
    value.station_id = 4095U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
    value.station_id = 4096U;
    expect_failure(value, castle::status::out_of_range);
    value.station_id = 65535U;
    expect_failure(value, castle::status::out_of_range);
    value.station_id = 0U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
}

TEST(RtcmGlonassRtkObservables, TimeOfDayBoundaries)
{
    glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical);
    value.time_of_day_ms = 0x7FFFFFFU;
    EXPECT_EQ(encode(value).result, castle::status::ok);
    value.time_of_day_ms = 0x8000000U;
    expect_failure(value, castle::status::out_of_range);
    value.time_of_day_ms = 0xFFFFFFFFU;
    expect_failure(value, castle::status::out_of_range);
    value.time_of_day_ms = 0U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
}

TEST(RtcmGlonassRtkObservables, SmoothingIntervalBoundaries)
{
    glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical);
    value.smoothing_interval = 7U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
    value.smoothing_interval = 8U;
    expect_failure(value, castle::status::out_of_range);
    value.smoothing_interval = 255U;
    expect_failure(value, castle::status::out_of_range);
    value.smoothing_interval = 0U;
    EXPECT_EQ(encode(value).result, castle::status::ok);
}

TYPED_TEST(RtcmGlonassRtkObservablesPerType, HeaderFieldBoundariesApplyToEveryType)
{
    TypeParam value = make_message<TypeParam>(profile::typical);
    value.station_id = 4096U;
    expect_failure(value, castle::status::out_of_range);
    value.station_id = 1U;
    value.time_of_day_ms = 0x8000000U;
    expect_failure(value, castle::status::out_of_range);
    value.time_of_day_ms = 1U;
    value.smoothing_interval = 8U;
    expect_failure(value, castle::status::out_of_range);
}

/** @brief Applies `set` to the second satellite of an otherwise valid type 1012 message and checks the status. */
template <typename Setter>
void expect_satellite_field(Setter set, castle::status status)
{
    glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical, 2U);
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

TEST(RtcmGlonassRtkObservables, SatelliteIdBoundaries)
{
    expect_satellite_field([](satellite& s) { s.satellite_id = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.satellite_id = 63U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.satellite_id = 64U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.satellite_id = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L1CodeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_code = 1U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_code = 2U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_code = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, FrequencyChannelBoundaries)
{
    expect_satellite_field([](satellite& s) { s.frequency_channel = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.frequency_channel = 20U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.frequency_channel = 31U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.frequency_channel = 32U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.frequency_channel = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L1PseudorangeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_pseudorange = 0x1FFFFFFU; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_pseudorange = 0x2000000U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_pseudorange = 0xFFFFFFFFU; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L1PhaseMinusPseudorangeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = 524287; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = -524288; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = 524288; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = -524289; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = INT32_MAX; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_phase_minus_pseudorange = INT32_MIN; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L1LockTimeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_lock_time = 127U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_lock_time = 128U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_lock_time = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L1AmbiguityBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l1_ambiguity = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_ambiguity = 127U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_ambiguity = 128U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l1_ambiguity = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, EightBitFieldsAcceptTheWholeByteRange)
{
    expect_satellite_field([](satellite& s) { s.l1_cnr = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l1_cnr = 255U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_cnr = 0U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_cnr = 255U; }, castle::status::ok);
}

TEST(RtcmGlonassRtkObservables, L2CodeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_code = 3U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_code = 4U; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_code = 255U; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L2PseudorangeDifferenceBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = 8191; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = -8192; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = 8192; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = -8193; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = INT16_MAX; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_pseudorange_difference = INT16_MIN; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L2PhaseMinusL1PseudorangeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = 524287; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = -524288; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = 524288; }, castle::status::out_of_range);
    expect_satellite_field([](satellite& s) { s.l2_phase_minus_l1_pseudorange = -524289; }, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, L2LockTimeBoundaries)
{
    expect_satellite_field([](satellite& s) { s.l2_lock_time = 127U; }, castle::status::ok);
    expect_satellite_field([](satellite& s) { s.l2_lock_time = 128U; }, castle::status::out_of_range);
}

TYPED_TEST(RtcmGlonassRtkObservablesPerType, SatelliteFieldLimitsOnlyApplyToTypesThatCarryThem)
{
    // L2 lock time 128 is invalid in 1011/1012 but ignored in 1009/1010.
    TypeParam value = make_message<TypeParam>(profile::typical, 1U);
    value.satellites[0U].l2_lock_time = 128U;
    const encoded frame = encode(value);
    EXPECT_EQ(frame.result, TypeParam::HAS_L2 ? castle::status::out_of_range : castle::status::ok);
}

TYPED_TEST(RtcmGlonassRtkObservablesPerType, ExactFitOutputBufferSucceeds)
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

TYPED_TEST(RtcmGlonassRtkObservablesPerType, OneByteShortOutputBufferReportsFull)
{
    const TypeParam value = make_message<TypeParam>(profile::typical);
    const encoded reference = encode(value);
    ASSERT_EQ(reference.result, castle::status::ok);

    uint8_t buffer[RTCM_MAX_FRAME_SIZE];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, buffer, reference.size - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmGlonassRtkObservables, TinyOutputBuffersReportFull)
{
    const glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical);
    uint8_t buffer[RTCM_FRAME_OVERHEAD + 8U] = {};
    castle::size_type written = 99U;

    EXPECT_EQ(encode_message(value, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(value, buffer, RTCM_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(encode_message(value, buffer, RTCM_FRAME_OVERHEAD, written), castle::status::full);
    EXPECT_EQ(encode_message(value, buffer, RTCM_FRAME_OVERHEAD + 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    // A header-only payload fits an 8 byte slot exactly; the first satellite then overflows.
    EXPECT_EQ(encode_message(glonass_rtk_l1_l2_obs_ext{}, buffer, sizeof(buffer), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(buffer));
    EXPECT_EQ(encode_message(value, buffer, sizeof(buffer), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmGlonassRtkObservables, OutOfRangeValueIsReportedBeforeRunningOutOfRoom)
{
    // In an 8 byte payload slot a valid first satellite overflows (full) but an invalid one is
    // diagnosed as out_of_range because the value is checked before the space.
    uint8_t buffer[RTCM_FRAME_OVERHEAD + 8U] = {};
    castle::size_type written = 99U;

    glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical, 1U);
    EXPECT_EQ(encode_message(value, buffer, sizeof(buffer), written), castle::status::full);
    value.satellites[0U].satellite_id = 64U;
    EXPECT_EQ(encode_message(value, buffer, sizeof(buffer), written), castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmGlonassRtkObservables, FirstFailureEndsTheMessage)
{
    // Out-of-range station and satellite together: the first failure is kept, nothing is written.
    glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical);
    value.station_id = 4096U;
    value.satellites[0U].satellite_id = 64U;
    expect_failure(value, castle::status::out_of_range);
}

TEST(RtcmGlonassRtkObservables, NullOutputIsAnInvalidArgument)
{
    const glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::typical);
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, static_cast<uint8_t*>(nullptr), 100U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(value, static_cast<uint8_t*>(nullptr), 0U, written), castle::status::invalid_argument);
}

TEST(RtcmGlonassRtkObservables, EncodingIsDeterministicAndDoesNotModifyTheMessage)
{
    const glonass_rtk_l1_l2_obs_ext value = make_message<glonass_rtk_l1_l2_obs_ext>(profile::maximum, 3U);
    const encoded first = encode(value);
    const encoded second = encode(value);
    ASSERT_EQ(first.result, castle::status::ok);
    ASSERT_EQ(first.size, second.size);
    EXPECT_EQ(memcmp(first.bytes, second.bytes, first.size), 0);
    EXPECT_EQ(value.satellites.size(), 3U);
    EXPECT_EQ(value.time_of_day_ms, 0x7FFFFFFU);
}

TEST(RtcmGlonassRtkObservables, RawIntegersAreWrittenWithoutScaling)
{
    glonass_rtk_l1_obs value = make_message<glonass_rtk_l1_obs>(profile::minimum, 1U);
    value.satellites[0U].l1_pseudorange = 1U;
    const encoded frame = encode(value);
    ASSERT_EQ(frame.result, castle::status::ok);
    EXPECT_EQ(payload_bits(frame.bytes, 73U, 25U), 1U);
    EXPECT_NEAR(static_cast<double>(value.satellites[0U].l1_pseudorange) * glonass_rtk_l1_obs::PSEUDORANGE_RESOLUTION_M, 0.02, 1e-12);
}

} // namespace
