#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/comm/can/filter.hpp"

namespace
{

using namespace castle::comm::can;

static_assert(castle::meta::is_trivially_copyable<filter>::value, "filter must be trivially copyable");
static_assert(castle::meta::is_trivially_destructible<filter>::value, "filter must be trivially destructible");
static_assert(castle::meta::is_standard_layout<filter>::value, "filter must be standard layout");
static_assert(invalid_filter < 0, "invalid_filter must never be a valid slot index");

// constexpr construction and matching
constexpr filter constexpr_exact = filter::exact(0x123U);
constexpr filter constexpr_any_ext = filter::any(id_format::extended);
constexpr filter constexpr_block(0x120U, 0x7F0U);
constexpr frame constexpr_frame(0x125U);
static_assert(constexpr_exact.id == 0x123U && constexpr_exact.mask == 0x7FFU, "exact standard filter");
static_assert(constexpr_any_ext.mask == 0U && constexpr_any_ext.format == id_format::extended, "any extended filter");
static_assert(!constexpr_exact.matches(constexpr_frame), "0x125 is not 0x123");
static_assert(constexpr_block.matches(constexpr_frame), "0x125 is inside 0x120..0x12F");
static_assert(!constexpr_any_ext.matches(constexpr_frame), "format mismatch");

TEST(CanFilter, InvalidFilterIsNegativeOne)
{
    EXPECT_EQ(invalid_filter, -1);
    const filter_handle h = invalid_filter;
    EXPECT_LT(h, 0);
}

TEST(CanFilter, DefaultAcceptsEveryStandardFrame)
{
    const filter f;
    EXPECT_EQ(f.id, 0U);
    EXPECT_EQ(f.mask, 0U);
    EXPECT_EQ(f.format, id_format::standard);
    EXPECT_TRUE(f.matches(frame(0U)));
    EXPECT_TRUE(f.matches(frame(0x7FFU)));
    EXPECT_FALSE(f.matches(frame(0x1U, id_format::extended)));
    EXPECT_EQ(validate_filter(f), castle::status::ok);
}

TEST(CanFilter, ConstructorStoresFieldsAndDefaultsToStandard)
{
    const filter f(0x120U, 0x7F0U);
    EXPECT_EQ(f.id, 0x120U);
    EXPECT_EQ(f.mask, 0x7F0U);
    EXPECT_EQ(f.format, id_format::standard);

    const filter e(0x18FF0000U, 0x1FFF0000U, id_format::extended);
    EXPECT_EQ(e.id, 0x18FF0000U);
    EXPECT_EQ(e.mask, 0x1FFF0000U);
    EXPECT_EQ(e.format, id_format::extended);
}

TEST(CanFilter, ExactStandardUsesFullElevenBitMask)
{
    const filter f = filter::exact(0x123U);
    EXPECT_EQ(f.id, 0x123U);
    EXPECT_EQ(f.mask, 0x7FFU);
    EXPECT_EQ(f.format, id_format::standard);
    EXPECT_TRUE(f.matches(frame(0x123U)));
    EXPECT_FALSE(f.matches(frame(0x122U)));
    EXPECT_FALSE(f.matches(frame(0x124U)));
    EXPECT_FALSE(f.matches(frame(0x523U)));
}

TEST(CanFilter, ExactExtendedUsesFullTwentyNineBitMask)
{
    const filter f = filter::exact(0x18FF0001U, id_format::extended);
    EXPECT_EQ(f.id, 0x18FF0001U);
    EXPECT_EQ(f.mask, 0x1FFFFFFFU);
    EXPECT_EQ(f.format, id_format::extended);
    EXPECT_TRUE(f.matches(frame(0x18FF0001U, id_format::extended)));
    EXPECT_FALSE(f.matches(frame(0x18FF0002U, id_format::extended)));
    EXPECT_FALSE(f.matches(frame(0x08FF0001U, id_format::extended)));
}

TEST(CanFilter, ExactDoesNotCrossFormats)
{
    const filter std_filter = filter::exact(0x123U);
    const filter ext_filter = filter::exact(0x123U, id_format::extended);
    EXPECT_FALSE(std_filter.matches(frame(0x123U, id_format::extended)));
    EXPECT_FALSE(ext_filter.matches(frame(0x123U)));
}

TEST(CanFilter, AnyAcceptsEveryIdentifierOfItsFormatOnly)
{
    const filter std_any = filter::any();
    EXPECT_EQ(std_any.id, 0U);
    EXPECT_EQ(std_any.mask, 0U);
    EXPECT_EQ(std_any.format, id_format::standard);
    EXPECT_TRUE(std_any.matches(frame(0U)));
    EXPECT_TRUE(std_any.matches(frame(0x555U)));
    EXPECT_TRUE(std_any.matches(frame(0x7FFU)));
    EXPECT_FALSE(std_any.matches(frame(0x555U, id_format::extended)));

    const filter ext_any = filter::any(id_format::extended);
    EXPECT_EQ(ext_any.mask, 0U);
    EXPECT_EQ(ext_any.format, id_format::extended);
    EXPECT_TRUE(ext_any.matches(frame(0U, id_format::extended)));
    EXPECT_TRUE(ext_any.matches(frame(0x1FFFFFFFU, id_format::extended)));
    EXPECT_FALSE(ext_any.matches(frame(0x555U)));
}

TEST(CanFilter, MaskedFilterIgnoresDontCareBits)
{
    const filter block(0x120U, 0x7F0U);
    for (uint32_t id = 0x120U; id <= 0x12FU; ++id)
    {
        EXPECT_TRUE(block.matches(frame(id))) << id;
    }
    EXPECT_FALSE(block.matches(frame(0x11FU)));
    EXPECT_FALSE(block.matches(frame(0x130U)));
    EXPECT_FALSE(block.matches(frame(0x220U)));
}

TEST(CanFilter, FilterIdBitsOutsideMaskDoNotMatter)
{
    // Low nibble of the filter id is not covered by the mask, so it is ignored as well.
    const filter f(0x12AU, 0x7F0U);
    EXPECT_TRUE(f.matches(frame(0x120U)));
    EXPECT_TRUE(f.matches(frame(0x12FU)));
    EXPECT_FALSE(f.matches(frame(0x130U)));
}

TEST(CanFilter, NonContiguousMask)
{
    // Only bits 0, 8 and 10 are compared; the id must have bit 0 and bit 8 set, bit 10 clear.
    const filter f(0x101U, 0x501U);
    EXPECT_TRUE(f.matches(frame(0x101U)));
    EXPECT_TRUE(f.matches(frame(0x101U | 0x0AEU)));
    EXPECT_TRUE(f.matches(frame(0x101U | 0x2FEU)));
    EXPECT_FALSE(f.matches(frame(0x100U)));
    EXPECT_FALSE(f.matches(frame(0x001U)));
    EXPECT_FALSE(f.matches(frame(0x501U)));
}

TEST(CanFilter, ZeroMaskMatchesAnyIdentifierButStillChecksFormat)
{
    const filter f(0x7FFU, 0U);
    EXPECT_TRUE(f.matches(frame(0U)));
    EXPECT_TRUE(f.matches(frame(0x123U)));
    EXPECT_FALSE(f.matches(frame(0x123U, id_format::extended)));
}

TEST(CanFilter, RemoteAndDataFramesBothPass)
{
    const filter f = filter::exact(0x40U);
    frame data_frame(0x40U);
    ASSERT_EQ(data_frame.set_data({1U, 2U}), castle::status::ok);
    frame remote_frame(0x40U);
    ASSERT_EQ(remote_frame.set_remote(2U), castle::status::ok);
    EXPECT_TRUE(f.matches(data_frame));
    EXPECT_TRUE(f.matches(remote_frame));
}

TEST(CanFilter, ValidateAcceptsInRangeFilters)
{
    EXPECT_EQ(validate_filter(filter::exact(0x7FFU)), castle::status::ok);
    EXPECT_EQ(validate_filter(filter::any()), castle::status::ok);
    EXPECT_EQ(validate_filter(filter::exact(0x1FFFFFFFU, id_format::extended)), castle::status::ok);
    EXPECT_EQ(validate_filter(filter::any(id_format::extended)), castle::status::ok);
    EXPECT_EQ(validate_filter(filter(0U, 0x1FFFFFFFU, id_format::extended)), castle::status::ok);
}

TEST(CanFilter, ValidateRejectsIdentifierWiderThanFormat)
{
    EXPECT_EQ(validate_filter(filter(0x800U, 0x7FFU)), castle::status::invalid_argument);
    EXPECT_EQ(validate_filter(filter(0x800U, 0x7FFU, id_format::extended)), castle::status::ok);
    EXPECT_EQ(validate_filter(filter(0x20000000U, 0U, id_format::extended)), castle::status::invalid_argument);
    EXPECT_EQ(validate_filter(filter(0xFFFFFFFFU, 0U)), castle::status::invalid_argument);
}

TEST(CanFilter, ValidateRejectsMaskWiderThanFormat)
{
    EXPECT_EQ(validate_filter(filter(0x1U, 0x800U)), castle::status::invalid_argument);
    EXPECT_EQ(validate_filter(filter(0x1U, 0x800U, id_format::extended)), castle::status::ok);
    EXPECT_EQ(validate_filter(filter(0x1U, 0x20000000U, id_format::extended)), castle::status::invalid_argument);
    EXPECT_EQ(validate_filter(filter(0x1U, 0xFFFFFFFFU)), castle::status::invalid_argument);
}

TEST(CanFilter, ValidateRejectsUnknownFormat)
{
    filter f(0x1U, 0x1U);
    f.format = static_cast<id_format>(2U);
    EXPECT_EQ(validate_filter(f), castle::status::invalid_argument);
    f.format = static_cast<id_format>(0xFFU);
    EXPECT_EQ(validate_filter(f), castle::status::invalid_argument);
}

TEST(CanFilter, CopyPreservesAllFields)
{
    const filter original(0x18FF0000U, 0x1FFF0000U, id_format::extended);
    const filter copy = original;
    EXPECT_EQ(copy.id, original.id);
    EXPECT_EQ(copy.mask, original.mask);
    EXPECT_EQ(copy.format, original.format);
}

} // namespace
