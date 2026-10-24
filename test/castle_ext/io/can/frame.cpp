#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/io/can/frame.hpp"

namespace
{

using namespace castle::io::can;

static_assert(castle::meta::is_trivially_copyable<frame>::value, "frame must be trivially copyable");
static_assert(castle::meta::is_trivially_destructible<frame>::value, "frame must be trivially destructible");
static_assert(castle::meta::is_standard_layout<frame>::value, "frame must be standard layout");
static_assert(sizeof(frame) <= 16U, "frame must stay a small value type");

static_assert(max_data_length == 8U, "classic CAN carries at most 8 bytes");
static_assert(max_standard_id == 0x7FFU, "11-bit identifier");
static_assert(max_extended_id == 0x1FFFFFFFU, "29-bit identifier");
static_assert(max_id(id_format::standard) == 0x7FFU, "standard max_id is constexpr");
static_assert(max_id(id_format::extended) == 0x1FFFFFFFU, "extended max_id is constexpr");

// constexpr use of the frame accessors
constexpr frame constexpr_default{};
static_assert(constexpr_default.id == 0U, "default id");
static_assert(!constexpr_default.is_extended(), "default is standard");
static_assert(!constexpr_default.is_remote(), "default is a data frame");
static_assert(constexpr_default.data_length() == 0U, "default has no payload");
constexpr frame constexpr_extended(0x18FF0001U, id_format::extended);
static_assert(constexpr_extended.is_extended(), "extended flag");
static_assert(constexpr_extended.id == 0x18FF0001U, "extended id");

frame make(uint32_t id, id_format format = id_format::standard)
{
    return frame(id, format);
}

TEST(CanFrame, EnumUnderlyingValuesAreStable)
{
    EXPECT_EQ(static_cast<uint8_t>(id_format::standard), 0U);
    EXPECT_EQ(static_cast<uint8_t>(id_format::extended), 1U);
    EXPECT_EQ(static_cast<uint8_t>(frame_type::data), 0U);
    EXPECT_EQ(static_cast<uint8_t>(frame_type::remote), 1U);
}

TEST(CanFrame, MaxIdDependsOnFormat)
{
    EXPECT_EQ(max_id(id_format::standard), 0x7FFU);
    EXPECT_EQ(max_id(id_format::extended), 0x1FFFFFFFU);
    EXPECT_LT(max_id(id_format::standard), max_id(id_format::extended));
}

TEST(CanFrame, DefaultConstructedIsEmptyStandardDataFrame)
{
    const frame f;
    EXPECT_EQ(f.id, 0U);
    EXPECT_EQ(f.format, id_format::standard);
    EXPECT_EQ(f.type, frame_type::data);
    EXPECT_EQ(f.dlc, 0U);
    EXPECT_FALSE(f.is_extended());
    EXPECT_FALSE(f.is_remote());
    EXPECT_EQ(f.data_length(), 0U);
    EXPECT_TRUE(f.payload().empty());
    for (size_t i = 0U; i < max_data_length; ++i)
    {
        EXPECT_EQ(f.data[i], 0U) << i;
    }
    EXPECT_EQ(validate_frame(f), castle::status::ok);
}

TEST(CanFrame, IdentifierConstructorDefaultsToStandard)
{
    const frame f(0x123U);
    EXPECT_EQ(f.id, 0x123U);
    EXPECT_EQ(f.format, id_format::standard);
    EXPECT_EQ(f.type, frame_type::data);
    EXPECT_EQ(f.dlc, 0U);
    EXPECT_EQ(f.data_length(), 0U);
    for (size_t i = 0U; i < max_data_length; ++i)
    {
        EXPECT_EQ(f.data[i], 0U) << i;
    }
}

TEST(CanFrame, IdentifierConstructorHonoursFormat)
{
    const frame f(0x18FF0001U, id_format::extended);
    EXPECT_EQ(f.id, 0x18FF0001U);
    EXPECT_TRUE(f.is_extended());
    EXPECT_FALSE(f.is_remote());
    EXPECT_EQ(f.dlc, 0U);

    const frame s(0x7FFU, id_format::standard);
    EXPECT_FALSE(s.is_extended());
}

TEST(CanFrame, SetDataFromPointerCopiesBytesAndSetsDlc)
{
    const uint8_t bytes[] = {0xDEU, 0xADU, 0xBEU, 0xEFU, 0x01U};
    frame f(0x10U);
    ASSERT_EQ(f.set_data(bytes, sizeof(bytes)), castle::status::ok);

    EXPECT_EQ(f.type, frame_type::data);
    EXPECT_EQ(f.dlc, 5U);
    EXPECT_EQ(f.data_length(), 5U);
    EXPECT_EQ(f.id, 0x10U);
    for (size_t i = 0U; i < sizeof(bytes); ++i)
    {
        EXPECT_EQ(f.data[i], bytes[i]) << i;
    }
    for (size_t i = sizeof(bytes); i < max_data_length; ++i)
    {
        EXPECT_EQ(f.data[i], 0U) << i;
    }
}

TEST(CanFrame, SetDataAcceptsFullEightBytes)
{
    const uint8_t bytes[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    frame f;
    ASSERT_EQ(f.set_data(bytes, 8U), castle::status::ok);
    EXPECT_EQ(f.dlc, 8U);
    EXPECT_EQ(f.data_length(), 8U);
    for (size_t i = 0U; i < 8U; ++i)
    {
        EXPECT_EQ(f.data[i], bytes[i]) << i;
    }
}

TEST(CanFrame, SetDataFromInitializerList)
{
    frame f;
    ASSERT_EQ(f.set_data({0x01U, 0x02U, 0x03U}), castle::status::ok);
    EXPECT_EQ(f.dlc, 3U);
    EXPECT_EQ(f.data[0], 0x01U);
    EXPECT_EQ(f.data[1], 0x02U);
    EXPECT_EQ(f.data[2], 0x03U);
    EXPECT_EQ(f.data[3], 0U);

    EXPECT_EQ(f.set_data({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}), castle::status::ok);
    EXPECT_EQ(f.dlc, 8U);
}

TEST(CanFrame, SetDataEmptyInitializerListClearsPayload)
{
    frame f;
    ASSERT_EQ(f.set_data({0xFFU, 0xFFU}), castle::status::ok);
    ASSERT_EQ(f.set_data({}), castle::status::ok);
    EXPECT_EQ(f.dlc, 0U);
    EXPECT_EQ(f.data_length(), 0U);
    EXPECT_EQ(f.data[0], 0U);
    EXPECT_EQ(f.data[1], 0U);
}

TEST(CanFrame, SetDataLengthZeroAllowsNullPointer)
{
    frame f;
    ASSERT_EQ(f.set_data({0xAAU, 0xBBU, 0xCCU}), castle::status::ok);
    ASSERT_EQ(f.set_data(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(f.dlc, 0U);
    EXPECT_EQ(f.type, frame_type::data);
    for (size_t i = 0U; i < max_data_length; ++i)
    {
        EXPECT_EQ(f.data[i], 0U) << i;
    }
}

TEST(CanFrame, SetDataZeroesTailBeyondNewLength)
{
    frame f;
    ASSERT_EQ(f.set_data({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}), castle::status::ok);
    ASSERT_EQ(f.set_data({9U, 9U}), castle::status::ok);
    EXPECT_EQ(f.dlc, 2U);
    EXPECT_EQ(f.data[0], 9U);
    EXPECT_EQ(f.data[1], 9U);
    for (size_t i = 2U; i < max_data_length; ++i)
    {
        EXPECT_EQ(f.data[i], 0U) << i;
    }
}

TEST(CanFrame, SetDataNullWithLengthIsRejectedAndFrameUnchanged)
{
    frame f(0x55U);
    ASSERT_EQ(f.set_data({1U, 2U, 3U}), castle::status::ok);
    const frame before = f;

    EXPECT_EQ(f.set_data(nullptr, 4U), castle::status::invalid_argument);
    EXPECT_EQ(f, before);
    EXPECT_EQ(f.dlc, 3U);
    EXPECT_EQ(f.data[0], 1U);
}

TEST(CanFrame, SetDataTooLongIsRejectedAndFrameUnchanged)
{
    const uint8_t bytes[9] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};
    frame f(0x55U);
    ASSERT_EQ(f.set_remote(4U), castle::status::ok);
    const frame before = f;

    EXPECT_EQ(f.set_data(bytes, 9U), castle::status::out_of_range);
    EXPECT_EQ(f.type, frame_type::remote);
    EXPECT_EQ(f.dlc, 4U);
    EXPECT_EQ(f, before);

    // The length check wins over the null check.
    EXPECT_EQ(f.set_data(nullptr, 9U), castle::status::out_of_range);
    EXPECT_EQ(f, before);

    EXPECT_EQ(f.set_data({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U}), castle::status::out_of_range);
    EXPECT_EQ(f, before);
}

TEST(CanFrame, SetDataConvertsRemoteFrameBackToData)
{
    frame f;
    ASSERT_EQ(f.set_remote(6U), castle::status::ok);
    ASSERT_TRUE(f.is_remote());
    ASSERT_EQ(f.set_data({7U}), castle::status::ok);
    EXPECT_FALSE(f.is_remote());
    EXPECT_EQ(f.type, frame_type::data);
    EXPECT_EQ(f.dlc, 1U);
    EXPECT_EQ(f.data_length(), 1U);
}

TEST(CanFrame, SetRemoteStoresRequestedLengthAndZeroesPayload)
{
    frame f(0x20U, id_format::extended);
    ASSERT_EQ(f.set_data({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}), castle::status::ok);

    ASSERT_EQ(f.set_remote(4U), castle::status::ok);
    EXPECT_TRUE(f.is_remote());
    EXPECT_EQ(f.type, frame_type::remote);
    EXPECT_EQ(f.dlc, 4U);
    EXPECT_EQ(f.data_length(), 0U);
    EXPECT_TRUE(f.payload().empty());
    EXPECT_EQ(f.id, 0x20U);
    EXPECT_TRUE(f.is_extended());
    for (size_t i = 0U; i < max_data_length; ++i)
    {
        EXPECT_EQ(f.data[i], 0U) << i;
    }
}

TEST(CanFrame, SetRemoteBoundaryLengths)
{
    frame f;
    EXPECT_EQ(f.set_remote(0U), castle::status::ok);
    EXPECT_EQ(f.dlc, 0U);
    EXPECT_TRUE(f.is_remote());

    EXPECT_EQ(f.set_remote(8U), castle::status::ok);
    EXPECT_EQ(f.dlc, 8U);
}

TEST(CanFrame, SetRemoteTooLongIsRejectedAndFrameUnchanged)
{
    frame f(0x42U);
    ASSERT_EQ(f.set_data({1U, 2U}), castle::status::ok);
    const frame before = f;

    EXPECT_EQ(f.set_remote(9U), castle::status::out_of_range);
    EXPECT_EQ(f, before);
    EXPECT_EQ(f.type, frame_type::data);
    EXPECT_EQ(f.dlc, 2U);
    EXPECT_EQ(f.data[0], 1U);
    EXPECT_EQ(f.data[1], 2U);

    EXPECT_EQ(f.set_remote(0xFFFFU), castle::status::out_of_range);
    EXPECT_EQ(f, before);
}

TEST(CanFrame, PayloadViewCoversExactlyValidBytes)
{
    frame f;
    ASSERT_EQ(f.set_data({0x11U, 0x22U, 0x33U}), castle::status::ok);

    const auto view = f.payload();
    ASSERT_EQ(view.size(), 3U);
    EXPECT_EQ(view.data(), f.data.data());
    EXPECT_EQ(view[0], 0x11U);
    EXPECT_EQ(view[1], 0x22U);
    EXPECT_EQ(view[2], 0x33U);
}

TEST(CanFrame, PayloadOfRemoteFrameIsEmpty)
{
    frame f;
    ASSERT_EQ(f.set_remote(5U), castle::status::ok);
    EXPECT_TRUE(f.payload().empty());
    EXPECT_EQ(f.payload().size(), 0U);
}

TEST(CanFrame, DataLengthClampsDlcAboveEight)
{
    frame f;
    f.dlc = 8U;
    EXPECT_EQ(f.data_length(), 8U);
    f.dlc = 9U;
    EXPECT_EQ(f.data_length(), 8U);
    f.dlc = 15U;
    EXPECT_EQ(f.data_length(), 8U);
    f.dlc = 255U;
    EXPECT_EQ(f.data_length(), 8U);
    EXPECT_EQ(f.payload().size(), 8U);
    f.dlc = 7U;
    EXPECT_EQ(f.data_length(), 7U);
    f.dlc = 0U;
    EXPECT_EQ(f.data_length(), 0U);
}

TEST(CanFrame, DataLengthOfRemoteFrameIsZeroRegardlessOfDlc)
{
    frame f;
    f.type = frame_type::remote;
    f.dlc = 8U;
    EXPECT_EQ(f.data_length(), 0U);
    f.dlc = 200U;
    EXPECT_EQ(f.data_length(), 0U);
}

TEST(CanFrame, FormatAndTypeFlags)
{
    frame f;
    f.format = id_format::extended;
    EXPECT_TRUE(f.is_extended());
    f.format = id_format::standard;
    EXPECT_FALSE(f.is_extended());
    f.type = frame_type::remote;
    EXPECT_TRUE(f.is_remote());
    f.type = frame_type::data;
    EXPECT_FALSE(f.is_remote());
}

TEST(CanFrame, EqualityOfIdenticalFrames)
{
    frame a(0x123U);
    frame b(0x123U);
    ASSERT_EQ(a.set_data({1U, 2U, 3U}), castle::status::ok);
    ASSERT_EQ(b.set_data({1U, 2U, 3U}), castle::status::ok);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a == a);
    EXPECT_TRUE(frame() == frame());
}

TEST(CanFrame, EqualityDetectsEveryDifferingField)
{
    frame base(0x123U);
    ASSERT_EQ(base.set_data({1U, 2U, 3U}), castle::status::ok);

    frame other_id = base;
    other_id.id = 0x124U;
    EXPECT_TRUE(base != other_id);
    EXPECT_FALSE(base == other_id);

    frame other_format = base;
    other_format.format = id_format::extended;
    EXPECT_TRUE(base != other_format);

    frame other_type = base;
    other_type.type = frame_type::remote;
    EXPECT_TRUE(base != other_type);

    frame other_dlc = base;
    other_dlc.dlc = 4U;
    EXPECT_TRUE(base != other_dlc);

    frame other_first = base;
    other_first.data[0] = 0xFFU;
    EXPECT_TRUE(base != other_first);

    frame other_last_valid = base;
    other_last_valid.data[2] = 0xFFU;
    EXPECT_TRUE(base != other_last_valid);
    EXPECT_FALSE(base == other_last_valid);
}

TEST(CanFrame, EqualityIgnoresBytesBeyondDataLength)
{
    frame a(0x123U);
    frame b(0x123U);
    ASSERT_EQ(a.set_data({1U, 2U}), castle::status::ok);
    ASSERT_EQ(b.set_data({1U, 2U}), castle::status::ok);
    b.data[2] = 0xAAU;
    b.data[7] = 0x55U;
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(CanFrame, EqualityOfRemoteFramesIgnoresPayloadButNotDlc)
{
    frame a(0x30U);
    frame b(0x30U);
    ASSERT_EQ(a.set_remote(3U), castle::status::ok);
    ASSERT_EQ(b.set_remote(3U), castle::status::ok);
    b.data[0] = 0x77U;
    EXPECT_TRUE(a == b);

    ASSERT_EQ(b.set_remote(4U), castle::status::ok);
    EXPECT_TRUE(a != b);

    frame data_frame(0x30U);
    ASSERT_EQ(data_frame.set_data({0U, 0U, 0U}), castle::status::ok);
    EXPECT_TRUE(a != data_frame);
}

TEST(CanFrame, EqualityUsesClampedLengthForOversizedDlc)
{
    frame a(0x1U);
    frame b(0x1U);
    a.dlc = 12U;
    b.dlc = 12U;
    a.data[7] = 1U;
    EXPECT_TRUE(a != b);
    b.data[7] = 1U;
    EXPECT_TRUE(a == b);
}

TEST(CanFrame, CopyIsDeepAndIndependent)
{
    frame a(0x99U, id_format::extended);
    ASSERT_EQ(a.set_data({1U, 2U, 3U, 4U}), castle::status::ok);
    frame b = a;
    EXPECT_TRUE(a == b);
    b.data[0] = 0x7FU;
    EXPECT_EQ(a.data[0], 1U);
    EXPECT_TRUE(a != b);

    frame c;
    c = a;
    EXPECT_TRUE(c == a);
}

TEST(CanFrame, ValidateAcceptsRepresentableFrames)
{
    frame f(0x7FFU);
    EXPECT_EQ(validate_frame(f), castle::status::ok);

    frame ext(0x1FFFFFFFU, id_format::extended);
    EXPECT_EQ(validate_frame(ext), castle::status::ok);

    frame zero_ext(0U, id_format::extended);
    EXPECT_EQ(validate_frame(zero_ext), castle::status::ok);

    frame full;
    ASSERT_EQ(full.set_data({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}), castle::status::ok);
    EXPECT_EQ(validate_frame(full), castle::status::ok);

    frame rtr;
    ASSERT_EQ(rtr.set_remote(8U), castle::status::ok);
    EXPECT_EQ(validate_frame(rtr), castle::status::ok);
}

TEST(CanFrame, ValidateRejectsIdentifierWiderThanFormat)
{
    EXPECT_EQ(validate_frame(make(0x800U)), castle::status::invalid_argument);
    EXPECT_EQ(validate_frame(make(0xFFFFFFFFU)), castle::status::invalid_argument);
    EXPECT_EQ(validate_frame(make(0x800U, id_format::extended)), castle::status::ok);
    EXPECT_EQ(validate_frame(make(0x1FFFFFFFU, id_format::extended)), castle::status::ok);
    EXPECT_EQ(validate_frame(make(0x20000000U, id_format::extended)), castle::status::invalid_argument);
    EXPECT_EQ(validate_frame(make(0xFFFFFFFFU, id_format::extended)), castle::status::invalid_argument);
}

TEST(CanFrame, ValidateRejectsDlcAboveEight)
{
    frame f(0x1U);
    f.dlc = 9U;
    EXPECT_EQ(validate_frame(f), castle::status::invalid_argument);
    f.dlc = 255U;
    EXPECT_EQ(validate_frame(f), castle::status::invalid_argument);
    f.dlc = 8U;
    EXPECT_EQ(validate_frame(f), castle::status::ok);

    f.type = frame_type::remote;
    f.dlc = 9U;
    EXPECT_EQ(validate_frame(f), castle::status::invalid_argument);
}

TEST(CanFrame, ValidateRejectsOutOfRangeEnumerators)
{
    frame bad_format(0x1U);
    bad_format.format = static_cast<id_format>(2U);
    EXPECT_EQ(validate_frame(bad_format), castle::status::invalid_argument);
    bad_format.format = static_cast<id_format>(0xFFU);
    EXPECT_EQ(validate_frame(bad_format), castle::status::invalid_argument);

    frame bad_type(0x1U);
    bad_type.type = static_cast<frame_type>(2U);
    EXPECT_EQ(validate_frame(bad_type), castle::status::invalid_argument);
    bad_type.type = static_cast<frame_type>(0xFFU);
    EXPECT_EQ(validate_frame(bad_type), castle::status::invalid_argument);
}

TEST(CanFrame, ValidateReportsInvalidArgumentForEachIndependentFault)
{
    // Each fault alone must trip validation while all other fields stay valid.
    frame f(0x10U);
    EXPECT_EQ(validate_frame(f), castle::status::ok);

    frame bad_id = f;
    bad_id.id = 0x800U;
    EXPECT_EQ(validate_frame(bad_id), castle::status::invalid_argument);

    frame bad_dlc = f;
    bad_dlc.dlc = 9U;
    EXPECT_EQ(validate_frame(bad_dlc), castle::status::invalid_argument);

    frame bad_type = f;
    bad_type.type = static_cast<frame_type>(7U);
    EXPECT_EQ(validate_frame(bad_type), castle::status::invalid_argument);

    frame bad_format = f;
    bad_format.format = static_cast<id_format>(7U);
    EXPECT_EQ(validate_frame(bad_format), castle::status::invalid_argument);
}

} // namespace
