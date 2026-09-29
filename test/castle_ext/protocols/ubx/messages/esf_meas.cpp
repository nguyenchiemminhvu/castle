#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageEsfMeasMirror, DecodesSignedMeasurementAndCalibrationTimestamp)
{
    uint8_t payload[16U] = {};
    castle::write_le32(&payload[0U], 123U);
    castle::write_le16(&payload[4U], esf_meas<>::FLAGS_TIME_TAG_TYPE_MASK);
    castle::write_le16(&payload[6U], 7U);
    const uint32_t word = (5U << 24U) | 0x00800001U;
    castle::write_le32(&payload[8U], word);
    castle::write_le32(&payload[12U], 0x55667788U);

    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_MEAS, static_cast<uint16_t>(sizeof(payload))},
                     array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)), checksum{}};
    esf_meas<8U> out;
    ASSERT_TRUE(esf_meas<8U>::decode(raw, out));
    EXPECT_EQ(out.time_tag, 123U);
    EXPECT_EQ(out.flags, esf_meas<8U>::FLAGS_TIME_TAG_TYPE_MASK);
    EXPECT_EQ(out.id, 7U);
    EXPECT_EQ(out.num_meas, 1U);
    EXPECT_EQ(out.data[0U].data_type, 5U);
    EXPECT_EQ(out.data[0U].data_value, -0x7FFFFF);
    EXPECT_TRUE(out.has_calib_ttag);
    EXPECT_EQ(out.calib_ttag, 0x55667788U);
}

TEST(UbxMessageEsfMeasMirror, RejectsCalibrationWithoutEnoughTrailingBytesAndPartialMeasurement)
{
    uint8_t payload[12U] = {};
    castle::write_le16(&payload[4U], 1U);
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_MEAS, 12U}, array_view<CASTLE_CONST uint8_t>(payload, 12U), checksum{}};
    esf_meas<> out;
    EXPECT_TRUE(esf_meas<>::decode(raw, out));

    raw.header.payload_length = 11U;
    EXPECT_FALSE(esf_meas<>::decode(raw, out));
    raw.header.payload_length = 9U;
    EXPECT_FALSE(esf_meas<>::decode(raw, out));
}

} // namespace

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageEsfMeasMirror, RejectsNonIntegralMeasurementByteCount)
{
    uint8_t payload[9U] = {};
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_MEAS, 9U},
                     array_view<CASTLE_CONST uint8_t>(payload, 9U), checksum{}};
    esf_meas<> out;
    EXPECT_FALSE(esf_meas<>::decode(raw, out));
}

TEST(UbxMessageEsfMeasMirror, ClampsStoredMeasurementsAndSkipsTheRest)
{
    uint8_t payload[16U] = {};
    castle::write_le32(&payload[8U], (1U << 24U) | 5U);
    castle::write_le32(&payload[12U], (2U << 24U) | 6U);
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_MEAS, 16U},
                     array_view<CASTLE_CONST uint8_t>(payload, 16U), checksum{}};
    esf_meas<1U> out;
    ASSERT_TRUE(esf_meas<1U>::decode(raw, out));
    EXPECT_EQ(out.num_meas, 1U);
    EXPECT_EQ(out.data[0U].data_type, 1U);
    EXPECT_EQ(out.data[0U].data_value, 5);
}

} // namespace
