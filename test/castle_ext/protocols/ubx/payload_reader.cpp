#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/payload_reader.hpp"

namespace
{

using namespace castle::protocols::ubx;
using castle::container::array_view;

TEST(PayloadReader, ReadsAllLittleEndianWidthsAndAdvances)
{
    uint8_t payload[32U] = {};
    castle::write_le16(&payload[0U], 0x1234U);
    castle::write_le16(&payload[2U], 0xFFFEU);
    castle::write_le32(&payload[4U], 0x89ABCDEFU);
    castle::write_le32(&payload[8U], 0xFFFFFFFEU);
    castle::write_le64(&payload[12U], 0x0123456789ABCDEFULL);
    castle::write_le64(&payload[20U], 0xFFFFFFFFFFFFFFFEULL);
    payload[28U] = 0x7EU;
    payload[29U] = 0x80U;
    payload[30U] = 0x81U;
    payload[31U] = 0x82U;

    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    EXPECT_EQ(reader.position(), 0U);
    EXPECT_EQ(reader.remaining(), sizeof(payload));
    EXPECT_EQ(reader.read_u16(), 0x1234U);
    EXPECT_EQ(reader.read_i16(), -2);
    EXPECT_EQ(reader.read_u32(), 0x89ABCDEFU);
    EXPECT_EQ(reader.read_i32(), -2);
    EXPECT_EQ(reader.read_u64(), 0x0123456789ABCDEFULL);
    EXPECT_EQ(reader.read_i64(), -2);
    EXPECT_EQ(reader.read_u8(), 0x7EU);
    EXPECT_EQ(reader.read_i8(), static_cast<int8_t>(0x80U));
    const uint8_t* bytes = reader.read_bytes(2U);
    ASSERT_NE(bytes, nullptr);
    EXPECT_EQ(bytes[0U], 0x81U);
    EXPECT_EQ(bytes[1U], 0x82U);
    EXPECT_EQ(reader.position(), sizeof(payload));
    EXPECT_EQ(reader.remaining(), 0U);
    EXPECT_TRUE(reader.ok());
}

TEST(PayloadReader, FailedReadLatchesErrorAndPreventsFurtherAdvancement)
{
    uint8_t payload[1U] = {0xAAU};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));

    EXPECT_EQ(reader.read_u16(), 0U);
    EXPECT_FALSE(reader.ok());
    EXPECT_EQ(reader.position(), 0U);
    EXPECT_EQ(reader.remaining(), 1U);
    EXPECT_FALSE(reader.skip(0U));
    EXPECT_EQ(reader.read_u8(), 0U);
    EXPECT_EQ(reader.position(), 0U);
}

TEST(PayloadReader, ExactSkipAndOverskipAreDeterministic)
{
    uint8_t payload[4U] = {1U, 2U, 3U, 4U};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    EXPECT_TRUE(reader.skip(4U));
    EXPECT_EQ(reader.position(), 4U);
    EXPECT_EQ(reader.remaining(), 0U);
    EXPECT_FALSE(reader.skip(1U));
    EXPECT_FALSE(reader.ok());
}

enum class reader_enum : uint16_t
{
    alpha = 0x0102U,
    beta = 0xFEDCU
};

TEST(PayloadReader, ReadsFloatingPointAndTypedFields)
{
    uint8_t payload[40U] = {};
    castle::write_le32(&payload[0U], castle::bit_cast<uint32_t>(-1.25F));
    castle::write_le64(&payload[4U], castle::bit_cast<uint64_t>(6.5));
    payload[12U] = static_cast<uint8_t>('Q');
    payload[13U] = 0xFEU;
    castle::write_le16(&payload[14U], 0xFEDCU);
    payload[16U] = static_cast<uint8_t>(-4);
    payload[17U] = static_cast<uint8_t>(5);
    payload[18U] = static_cast<uint8_t>(-6);
    castle::write_le16(&payload[19U], static_cast<uint16_t>(-9));
    castle::write_le32(&payload[21U], 0xCAFEBABEU);
    castle::write_le32(&payload[25U], static_cast<uint32_t>(-11));
    castle::write_le64(&payload[29U], 0x1122334455667788ULL);
    payload[37U] = 0x01U;
    payload[38U] = 0x02U;
    payload[39U] = 0x03U;

    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    float r4 = 0.0F;
    double r8 = 0.0;
    char ch = '\0';
    int8_t i8 = 0;
    reader_enum kind = reader_enum::alpha;
    castle::container::array<int8_t, 3U> trio{};
    int16_t i16 = 0;
    uint32_t u32 = 0U;
    int32_t i32 = 0;
    uint64_t u64 = 0U;
    castle::container::array<uint8_t, 3U> tail{};

    reader.read(r4);
    reader.read(r8);
    reader.read(ch);
    reader.read(i8);
    reader.read(kind);
    reader.read(trio);
    reader.read(i16);
    reader.read(u32);
    reader.read(i32);
    reader.read(u64);
    reader.field(tail);

    ASSERT_TRUE(reader.ok());
    EXPECT_FLOAT_EQ(r4, -1.25F);
    EXPECT_DOUBLE_EQ(r8, 6.5);
    EXPECT_EQ(ch, 'Q');
    EXPECT_EQ(i8, -2);
    EXPECT_EQ(kind, reader_enum::beta);
    EXPECT_EQ(trio[0U], -4);
    EXPECT_EQ(trio[1U], 5);
    EXPECT_EQ(trio[2U], -6);
    EXPECT_EQ(i16, -9);
    EXPECT_EQ(u32, 0xCAFEBABEU);
    EXPECT_EQ(i32, -11);
    EXPECT_EQ(u64, 0x1122334455667788ULL);
    EXPECT_EQ(tail[2U], 0x03U);
    EXPECT_EQ(reader.remaining(), 0U);

    payload_reader wide(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    int64_t i64 = 0;
    wide.read(u8);
    wide.skip(3U);
    wide.read(u16);
    wide.skip(2U);
    wide.read(i64);
    ASSERT_TRUE(wide.ok());
    EXPECT_EQ(u8, castle::bit_cast<uint32_t>(-1.25F) & 0xFFU);
    EXPECT_EQ(u16, static_cast<uint16_t>(castle::bit_cast<uint64_t>(6.5)));
    EXPECT_EQ(i64, static_cast<int64_t>(castle::read_le64(&payload[8U])));
}

TEST(PayloadReader, LayoutVerbsReservedConstantAndFail)
{
    uint8_t payload[5U] = {0x00U, 0x00U, 0x7FU, 0x10U, 0x20U};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));

    reader.reserved(2U);
    reader.constant(0x7FU);
    EXPECT_TRUE(reader.ok());
    EXPECT_EQ(reader.position(), 3U);

    reader.constant(0x11U); // actual byte is 0x10
    EXPECT_FALSE(reader.ok());
    EXPECT_EQ(reader.read_u8(), 0U);

    payload_reader explicit_fail(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    explicit_fail.fail();
    EXPECT_FALSE(explicit_fail.ok());
    EXPECT_EQ(explicit_fail.read_u8(), 0U);
    EXPECT_EQ(explicit_fail.position(), 0U);

    payload_reader overrun(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    overrun.reserved(6U);
    EXPECT_FALSE(overrun.ok());
}

TEST(PayloadReader, TextReadsNulPaddedAndFullWidthFields)
{
    uint8_t payload[9U] = {'R', 'O', 'M', 0U, 'x', 'A', 'B', 'C', 'D'};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));

    castle::container::string<5U> padded{};
    reader.text(padded); // "ROM\0x": stops at NUL but consumes all 5 bytes
    EXPECT_TRUE(padded.view() == castle::container::string_view("ROM", 3U));
    EXPECT_EQ(reader.position(), 5U);

    castle::container::string<4U> full("old");
    reader.text(full); // no NUL: all 4 characters kept, previous content replaced
    EXPECT_TRUE(full.view() == castle::container::string_view("ABCD", 4U));
    EXPECT_TRUE(reader.ok());

    castle::container::string<2U> beyond("zz");
    reader.text(beyond);
    EXPECT_FALSE(reader.ok());
    EXPECT_TRUE(beyond.empty());
}

TEST(PayloadReader, NullDataViewStartsFailed)
{
    payload_reader empty{array_view<CASTLE_CONST uint8_t>{}};
    EXPECT_FALSE(empty.ok());
    EXPECT_EQ(empty.remaining(), 0U);
    EXPECT_EQ(empty.read_u8(), 0U);

    payload_reader dangling{array_view<CASTLE_CONST uint8_t>(nullptr, 4U)};
    EXPECT_FALSE(dangling.ok());
    EXPECT_EQ(dangling.remaining(), 0U);
    EXPECT_EQ(dangling.read_u32(), 0U);
}

} // namespace
