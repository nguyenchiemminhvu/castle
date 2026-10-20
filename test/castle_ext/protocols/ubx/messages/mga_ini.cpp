#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mga_ini.hpp"

#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

template <typename M>
struct spec;

template <typename M>
message_view raw_of(CASTLE_CONST uint8_t* payload, castle::size_type length)
{
    return message_view{
        message_header{M::msg_class, M::msg_id, static_cast<uint16_t>(length)},
        array_view<CASTLE_CONST uint8_t>(payload, length),
        checksum{}};
}

/// Number of models in `Ms` whose `matches()` accepts `raw` (registry dispatch).
template <typename... Ms>
int match_count(CASTLE_CONST message_view& raw)
{
    return (0 + ... + static_cast<int>(Ms::matches(raw)));
}

/// `true` when `m` is still value-initialised (compared through its encoding).
template <typename M>
bool is_default(CASTLE_CONST M& m)
{
    uint8_t lhs[M::payload_length + UBX_FRAME_OVERHEAD] = {};
    uint8_t rhs[M::payload_length + UBX_FRAME_OVERHEAD] = {};
    castle::size_type lhs_written = 0U;
    castle::size_type rhs_written = 0U;
    if ((encode_message(m, lhs, sizeof(lhs), lhs_written) != castle::status::ok)
        || (encode_message(M{}, rhs, sizeof(rhs), rhs_written) != castle::status::ok)
        || (lhs_written != rhs_written))
    {
        return false;
    }
    for (castle::size_type i = 0U; i < lhs_written; ++i)
    {
        if (lhs[i] != rhs[i])
        {
            return false;
        }
    }
    return true;
}

/// Hand-built payload at spec offsets (reserved bytes non-zero) decodes field-by-field.
template <typename M>
void check_spec_offsets()
{
    uint8_t payload[M::payload_length] = {};
    spec<M>::fill(payload, 0xA5U);
    CASTLE_CONST message_view raw = raw_of<M>(payload, M::payload_length);
    EXPECT_TRUE(M::matches(raw));
    M out{};
    ASSERT_TRUE(M::decode(raw, out));
    spec<M>::expect(out);
}

/// encode_message produces exactly the spec payload (reserved = 0) and decodes back.
template <typename M>
void check_round_trip()
{
    CASTLE_CONST M in = spec<M>::make();
    uint8_t frame[M::payload_length + UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_MGA);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], M::msg_id);

    uint8_t expected[M::payload_length] = {};
    spec<M>::fill(expected, 0x00U);
    for (castle::size_type i = 0U; i < M::payload_length; ++i)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + i], expected[i]) << "payload byte " << i;
    }

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(M::matches(raw));
    M out{};
    ASSERT_TRUE(M::decode(raw, out));
    spec<M>::expect(out);
}

/// Wrong class/ID, wrong length and a bad version byte are rejected and reset `out`.
template <typename M>
void check_rejects_common(uint8_t version_offset)
{
    uint8_t payload[M::payload_length + 1U] = {};
    spec<M>::fill(payload, 0x00U);
    M out{};

    message_view raw = raw_of<M>(payload, M::payload_length);
    raw.header.msg_class = static_cast<uint8_t>(M::msg_class ^ 0xFFU);
    EXPECT_FALSE(M::matches(raw));
    out = spec<M>::make();
    EXPECT_FALSE(M::decode(raw, out));
    EXPECT_TRUE(is_default(out));

    raw = raw_of<M>(payload, M::payload_length);
    raw.header.msg_id = static_cast<uint8_t>(M::msg_id ^ 0xFFU);
    EXPECT_FALSE(M::matches(raw));
    out = spec<M>::make();
    EXPECT_FALSE(M::decode(raw, out));
    EXPECT_TRUE(is_default(out));

    raw = raw_of<M>(payload, M::payload_length - 1U);
    out = spec<M>::make();
    EXPECT_FALSE(M::decode(raw, out));
    EXPECT_TRUE(is_default(out));

    raw = raw_of<M>(payload, M::payload_length + 1U);
    out = spec<M>::make();
    EXPECT_FALSE(M::decode(raw, out));
    EXPECT_TRUE(is_default(out));

    payload[version_offset] = 0x01U;
    raw = raw_of<M>(payload, M::payload_length);
    out = spec<M>::make();
    EXPECT_FALSE(M::decode(raw, out));
    EXPECT_TRUE(is_default(out));
}

/// typed_message layouts also narrow matches() by exact length and type byte.
template <typename M>
void check_rejects_typed()
{
    check_rejects_common<M>(1U);

    uint8_t payload[M::payload_length + 1U] = {};
    spec<M>::fill(payload, 0x00U);
    EXPECT_FALSE(M::matches(raw_of<M>(payload, M::payload_length - 1U)));
    EXPECT_FALSE(M::matches(raw_of<M>(payload, M::payload_length + 1U)));

    payload[1U] = 0x01U;
    EXPECT_TRUE(M::matches(raw_of<M>(payload, M::payload_length))); // version is checked by decode()
    payload[1U] = 0x00U;

    payload[0U] = static_cast<uint8_t>(M::message_type ^ 0x80U);
    CASTLE_CONST message_view raw = raw_of<M>(payload, M::payload_length);
    EXPECT_FALSE(M::matches(raw));
    M out = spec<M>::make();
    EXPECT_FALSE(M::decode(raw, out));
    EXPECT_TRUE(is_default(out));
}

/// encode_message into a buffer one byte short reports `full` and writes nothing.
template <typename M>
void check_too_small()
{
    uint8_t frame[M::payload_length + UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(spec<M>::make(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

/// A frame of layout `M` is accepted by exactly one of `All` (and that one is `M`).
template <typename M, typename... All>
void check_exclusive()
{
    uint8_t frame[M::payload_length + UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(spec<M>::make(), frame, sizeof(frame), written), castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(M::matches(raw));
    EXPECT_EQ(match_count<All...>(raw), 1);
}

template <>
struct spec<mga_ini_pos_xyz>
{
    /// Spec 3.13.7.1: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x00U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // ecefX
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // ecefY
        castle::write_le32s(&payload[12U], static_cast<int32_t>(-29459008)); // ecefZ
        castle::write_le32(&payload[16U], static_cast<uint32_t>(0x20101010U)); // posAcc
    }

    static mga_ini_pos_xyz make()
    {
        mga_ini_pos_xyz m{};
        m.ecef_x = static_cast<int32_t>(-21004480);
        m.ecef_y = static_cast<int32_t>(-25231744);
        m.ecef_z = static_cast<int32_t>(-29459008);
        m.pos_acc = static_cast<uint32_t>(0x20101010U);
        return m;
    }

    static void expect(CASTLE_CONST mga_ini_pos_xyz& m)
    {
        EXPECT_EQ(m.ecef_x, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.ecef_y, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.ecef_z, static_cast<int32_t>(-29459008));
        EXPECT_EQ(m.pos_acc, static_cast<uint32_t>(0x20101010U));
    }
};

template <>
struct spec<mga_ini_pos_llh>
{
    /// Spec 3.13.7.2: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x01U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // lat
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // lon
        castle::write_le32s(&payload[12U], static_cast<int32_t>(-29459008)); // alt
        castle::write_le32(&payload[16U], static_cast<uint32_t>(0x20101010U)); // posAcc
    }

    static mga_ini_pos_llh make()
    {
        mga_ini_pos_llh m{};
        m.lat = static_cast<int32_t>(-21004480);
        m.lon = static_cast<int32_t>(-25231744);
        m.alt = static_cast<int32_t>(-29459008);
        m.pos_acc = static_cast<uint32_t>(0x20101010U);
        return m;
    }

    static void expect(CASTLE_CONST mga_ini_pos_llh& m)
    {
        EXPECT_EQ(m.lat, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.lon, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.alt, static_cast<int32_t>(-29459008));
        EXPECT_EQ(m.pos_acc, static_cast<uint32_t>(0x20101010U));
    }
};

template <>
struct spec<mga_ini_time_utc>
{
    /// Spec 3.13.7.3: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x10U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // ref
        payload[3U] = static_cast<uint8_t>(static_cast<int8_t>(-4)); // leapSecs
        castle::write_le16(&payload[4U], static_cast<uint16_t>(0x1404U)); // year
        payload[6U] = static_cast<uint8_t>(0x3BU); // month
        payload[7U] = static_cast<uint8_t>(0x42U); // day
        payload[8U] = static_cast<uint8_t>(0x49U); // hour
        payload[9U] = static_cast<uint8_t>(0x50U); // minute
        payload[10U] = static_cast<uint8_t>(0x57U); // second
        payload[11U] = reserved_fill;
        castle::write_le32(&payload[12U], static_cast<uint32_t>(0x1C0C0C0CU)); // ns
        castle::write_le16(&payload[16U], static_cast<uint16_t>(0x2010U)); // tAccS
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[18U + i] = reserved_fill; }
        castle::write_le32(&payload[20U], static_cast<uint32_t>(0x24141414U)); // tAccNs
    }

    static mga_ini_time_utc make()
    {
        mga_ini_time_utc m{};
        m.ref = static_cast<uint8_t>(0x1FU);
        m.leap_secs = static_cast<int8_t>(-4);
        m.year = static_cast<uint16_t>(0x1404U);
        m.month = static_cast<uint8_t>(0x3BU);
        m.day = static_cast<uint8_t>(0x42U);
        m.hour = static_cast<uint8_t>(0x49U);
        m.minute = static_cast<uint8_t>(0x50U);
        m.second = static_cast<uint8_t>(0x57U);
        m.ns = static_cast<uint32_t>(0x1C0C0C0CU);
        m.t_acc_s = static_cast<uint16_t>(0x2010U);
        m.t_acc_ns = static_cast<uint32_t>(0x24141414U);
        return m;
    }

    static void expect(CASTLE_CONST mga_ini_time_utc& m)
    {
        EXPECT_EQ(m.ref, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.leap_secs, static_cast<int8_t>(-4));
        EXPECT_EQ(m.year, static_cast<uint16_t>(0x1404U));
        EXPECT_EQ(m.month, static_cast<uint8_t>(0x3BU));
        EXPECT_EQ(m.day, static_cast<uint8_t>(0x42U));
        EXPECT_EQ(m.hour, static_cast<uint8_t>(0x49U));
        EXPECT_EQ(m.minute, static_cast<uint8_t>(0x50U));
        EXPECT_EQ(m.second, static_cast<uint8_t>(0x57U));
        EXPECT_EQ(m.ns, static_cast<uint32_t>(0x1C0C0C0CU));
        EXPECT_EQ(m.t_acc_s, static_cast<uint16_t>(0x2010U));
        EXPECT_EQ(m.t_acc_ns, static_cast<uint32_t>(0x24141414U));
    }
};

template <>
struct spec<mga_ini_time_gnss>
{
    /// Spec 3.13.7.4: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x11U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // ref
        payload[3U] = static_cast<uint8_t>(0x03U); // gnssId
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[4U + i] = reserved_fill; }
        castle::write_le16(&payload[6U], static_cast<uint16_t>(0x1606U)); // week
        castle::write_le32(&payload[8U], static_cast<uint32_t>(0x18080808U)); // tow
        castle::write_le32(&payload[12U], static_cast<uint32_t>(0x1C0C0C0CU)); // ns
        castle::write_le16(&payload[16U], static_cast<uint16_t>(0x2010U)); // tAccS
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[18U + i] = reserved_fill; }
        castle::write_le32(&payload[20U], static_cast<uint32_t>(0x24141414U)); // tAccNs
    }

    static mga_ini_time_gnss make()
    {
        mga_ini_time_gnss m{};
        m.ref = static_cast<uint8_t>(0x1FU);
        m.gnss_id = static_cast<mga_ini_gnss_id>(0x03U);
        m.week = static_cast<uint16_t>(0x1606U);
        m.tow = static_cast<uint32_t>(0x18080808U);
        m.ns = static_cast<uint32_t>(0x1C0C0C0CU);
        m.t_acc_s = static_cast<uint16_t>(0x2010U);
        m.t_acc_ns = static_cast<uint32_t>(0x24141414U);
        return m;
    }

    static void expect(CASTLE_CONST mga_ini_time_gnss& m)
    {
        EXPECT_EQ(m.ref, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(static_cast<uint8_t>(m.gnss_id), static_cast<uint8_t>(0x03U));
        EXPECT_EQ(m.week, static_cast<uint16_t>(0x1606U));
        EXPECT_EQ(m.tow, static_cast<uint32_t>(0x18080808U));
        EXPECT_EQ(m.ns, static_cast<uint32_t>(0x1C0C0C0CU));
        EXPECT_EQ(m.t_acc_s, static_cast<uint16_t>(0x2010U));
        EXPECT_EQ(m.t_acc_ns, static_cast<uint32_t>(0x24141414U));
    }
};

template <>
struct spec<mga_ini_clkd>
{
    /// Spec 3.13.7.5: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x20U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // clkD
        castle::write_le32(&payload[8U], static_cast<uint32_t>(0x18080808U)); // clkDAcc
    }

    static mga_ini_clkd make()
    {
        mga_ini_clkd m{};
        m.clk_d = static_cast<int32_t>(-21004480);
        m.clk_d_acc = static_cast<uint32_t>(0x18080808U);
        return m;
    }

    static void expect(CASTLE_CONST mga_ini_clkd& m)
    {
        EXPECT_EQ(m.clk_d, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.clk_d_acc, static_cast<uint32_t>(0x18080808U));
    }
};

template <>
struct spec<mga_ini_freq>
{
    /// Spec 3.13.7.6: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x21U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = reserved_fill;
        payload[3U] = static_cast<uint8_t>(0x26U); // flags
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // freq
        castle::write_le32(&payload[8U], static_cast<uint32_t>(0x18080808U)); // freqAcc
    }

    static mga_ini_freq make()
    {
        mga_ini_freq m{};
        m.flags = static_cast<uint8_t>(0x26U);
        m.freq = static_cast<int32_t>(-21004480);
        m.freq_acc = static_cast<uint32_t>(0x18080808U);
        return m;
    }

    static void expect(CASTLE_CONST mga_ini_freq& m)
    {
        EXPECT_EQ(m.flags, static_cast<uint8_t>(0x26U));
        EXPECT_EQ(m.freq, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.freq_acc, static_cast<uint32_t>(0x18080808U));
    }
};

TEST(UbxMessageMgaIni, PosXyzSpecOffsets)
{
    check_spec_offsets<mga_ini_pos_xyz>();
}

TEST(UbxMessageMgaIni, PosXyzRoundTrip)
{
    check_round_trip<mga_ini_pos_xyz>();
}

TEST(UbxMessageMgaIni, PosXyzRejects)
{
    check_rejects_typed<mga_ini_pos_xyz>();
}

TEST(UbxMessageMgaIni, PosXyzTooSmallBuffer)
{
    check_too_small<mga_ini_pos_xyz>();
}

TEST(UbxMessageMgaIni, PosXyzDispatchExclusive)
{
    check_exclusive<mga_ini_pos_xyz, mga_ini_pos_xyz, mga_ini_pos_llh, mga_ini_time_utc, mga_ini_time_gnss, mga_ini_clkd, mga_ini_freq>();
}

TEST(UbxMessageMgaIni, PosLlhSpecOffsets)
{
    check_spec_offsets<mga_ini_pos_llh>();
}

TEST(UbxMessageMgaIni, PosLlhRoundTrip)
{
    check_round_trip<mga_ini_pos_llh>();
}

TEST(UbxMessageMgaIni, PosLlhRejects)
{
    check_rejects_typed<mga_ini_pos_llh>();
}

TEST(UbxMessageMgaIni, PosLlhTooSmallBuffer)
{
    check_too_small<mga_ini_pos_llh>();
}

TEST(UbxMessageMgaIni, PosLlhDispatchExclusive)
{
    check_exclusive<mga_ini_pos_llh, mga_ini_pos_xyz, mga_ini_pos_llh, mga_ini_time_utc, mga_ini_time_gnss, mga_ini_clkd, mga_ini_freq>();
}

TEST(UbxMessageMgaIni, TimeUtcSpecOffsets)
{
    check_spec_offsets<mga_ini_time_utc>();
}

TEST(UbxMessageMgaIni, TimeUtcRoundTrip)
{
    check_round_trip<mga_ini_time_utc>();
}

TEST(UbxMessageMgaIni, TimeUtcRejects)
{
    check_rejects_typed<mga_ini_time_utc>();
}

TEST(UbxMessageMgaIni, TimeUtcTooSmallBuffer)
{
    check_too_small<mga_ini_time_utc>();
}

TEST(UbxMessageMgaIni, TimeUtcDispatchExclusive)
{
    check_exclusive<mga_ini_time_utc, mga_ini_pos_xyz, mga_ini_pos_llh, mga_ini_time_utc, mga_ini_time_gnss, mga_ini_clkd, mga_ini_freq>();
}

TEST(UbxMessageMgaIni, TimeGnssSpecOffsets)
{
    check_spec_offsets<mga_ini_time_gnss>();
}

TEST(UbxMessageMgaIni, TimeGnssRoundTrip)
{
    check_round_trip<mga_ini_time_gnss>();
}

TEST(UbxMessageMgaIni, TimeGnssRejects)
{
    check_rejects_typed<mga_ini_time_gnss>();
}

TEST(UbxMessageMgaIni, TimeGnssTooSmallBuffer)
{
    check_too_small<mga_ini_time_gnss>();
}

TEST(UbxMessageMgaIni, TimeGnssDispatchExclusive)
{
    check_exclusive<mga_ini_time_gnss, mga_ini_pos_xyz, mga_ini_pos_llh, mga_ini_time_utc, mga_ini_time_gnss, mga_ini_clkd, mga_ini_freq>();
}

TEST(UbxMessageMgaIni, ClkdSpecOffsets)
{
    check_spec_offsets<mga_ini_clkd>();
}

TEST(UbxMessageMgaIni, ClkdRoundTrip)
{
    check_round_trip<mga_ini_clkd>();
}

TEST(UbxMessageMgaIni, ClkdRejects)
{
    check_rejects_typed<mga_ini_clkd>();
}

TEST(UbxMessageMgaIni, ClkdTooSmallBuffer)
{
    check_too_small<mga_ini_clkd>();
}

TEST(UbxMessageMgaIni, ClkdDispatchExclusive)
{
    check_exclusive<mga_ini_clkd, mga_ini_pos_xyz, mga_ini_pos_llh, mga_ini_time_utc, mga_ini_time_gnss, mga_ini_clkd, mga_ini_freq>();
}

TEST(UbxMessageMgaIni, FreqSpecOffsets)
{
    check_spec_offsets<mga_ini_freq>();
}

TEST(UbxMessageMgaIni, FreqRoundTrip)
{
    check_round_trip<mga_ini_freq>();
}

TEST(UbxMessageMgaIni, FreqRejects)
{
    check_rejects_typed<mga_ini_freq>();
}

TEST(UbxMessageMgaIni, FreqTooSmallBuffer)
{
    check_too_small<mga_ini_freq>();
}

TEST(UbxMessageMgaIni, FreqDispatchExclusive)
{
    check_exclusive<mga_ini_freq, mga_ini_pos_xyz, mga_ini_pos_llh, mga_ini_time_utc, mga_ini_time_gnss, mga_ini_clkd, mga_ini_freq>();
}

TEST(UbxMessageMgaIni, ConstantsMatchSpec)
{
    EXPECT_EQ(mga_ini_pos_xyz::msg_id, UBX_ID_MGA_INI);
    EXPECT_EQ(MGA_INI_TYPE_POS_XYZ, 0x00U);
    EXPECT_EQ(MGA_INI_TYPE_POS_LLH, 0x01U);
    EXPECT_EQ(MGA_INI_TYPE_TIME_UTC, 0x10U);
    EXPECT_EQ(MGA_INI_TYPE_TIME_GNSS, 0x11U);
    EXPECT_EQ(MGA_INI_TYPE_CLKD, 0x20U);
    EXPECT_EQ(MGA_INI_TYPE_FREQ, 0x21U);
    EXPECT_EQ(static_cast<uint8_t>(mga_ini_gnss_id::gps), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mga_ini_gnss_id::galileo), 2U);
    EXPECT_EQ(static_cast<uint8_t>(mga_ini_gnss_id::beidou), 3U);
    EXPECT_EQ(static_cast<uint8_t>(mga_ini_gnss_id::glonass), 6U);
}

TEST(UbxMessageMgaIni, PositionScaledHelpers)
{
    mga_ini_pos_xyz xyz{};
    xyz.ecef_x = 123456;
    xyz.ecef_y = -654321;
    xyz.ecef_z = 50;
    xyz.pos_acc = 2500U;
    EXPECT_DOUBLE_EQ(xyz.ecef_x_m(), 1234.56);
    EXPECT_DOUBLE_EQ(xyz.ecef_y_m(), -6543.21);
    EXPECT_DOUBLE_EQ(xyz.ecef_z_m(), 0.5);
    EXPECT_DOUBLE_EQ(xyz.pos_acc_m(), 25.0);

    mga_ini_pos_llh llh{};
    llh.lat = 473977000;
    llh.lon = -85456000;
    llh.alt = 41050;
    llh.pos_acc = 150U;
    EXPECT_DOUBLE_EQ(llh.latitude_deg(), 47.3977);
    EXPECT_DOUBLE_EQ(llh.longitude_deg(), -8.5456);
    EXPECT_DOUBLE_EQ(llh.alt_m(), 410.5);
    EXPECT_DOUBLE_EQ(llh.pos_acc_m(), 1.5);
}

template <typename Time>
void check_time_ref_bits()
{
    Time t{};
    EXPECT_EQ(t.source(), mga_ini_time_source::none);
    EXPECT_FALSE(t.fall());
    EXPECT_FALSE(t.last());

    t.ref = static_cast<uint8_t>(static_cast<uint8_t>(mga_ini_time_source::extint1) | Time::REF_FALL);
    EXPECT_EQ(t.source(), mga_ini_time_source::extint1);
    EXPECT_TRUE(t.fall());
    EXPECT_FALSE(t.last());

    t.ref = static_cast<uint8_t>(static_cast<uint8_t>(mga_ini_time_source::extint0) | Time::REF_LAST);
    EXPECT_EQ(t.source(), mga_ini_time_source::extint0);
    EXPECT_FALSE(t.fall());
    EXPECT_TRUE(t.last());

    EXPECT_EQ(Time::REF_SOURCE, 0x0FU);
    EXPECT_EQ(Time::REF_FALL, 0x10U);
    EXPECT_EQ(Time::REF_LAST, 0x20U);
}

TEST(UbxMessageMgaIni, TimeUtcRefBits)
{
    check_time_ref_bits<mga_ini_time_utc>();
    EXPECT_EQ(mga_ini_time_utc::LEAP_SECS_UNKNOWN, -128);
}

TEST(UbxMessageMgaIni, TimeGnssRefBits)
{
    check_time_ref_bits<mga_ini_time_gnss>();
}

TEST(UbxMessageMgaIni, FreqFlagsAndScale)
{
    mga_ini_freq f{};
    EXPECT_EQ(f.source(), mga_ini_freq_source::extint0);
    EXPECT_FALSE(f.fall());
    f.flags = static_cast<uint8_t>(static_cast<uint8_t>(mga_ini_freq_source::extint1) | mga_ini_freq::FLAGS_FALL);
    EXPECT_EQ(f.source(), mga_ini_freq_source::extint1);
    EXPECT_TRUE(f.fall());
    EXPECT_EQ(mga_ini_freq::FLAGS_SOURCE, 0x0FU);
    EXPECT_EQ(mga_ini_freq::FLAGS_FALL, 0x10U);
    f.freq = 1000000025;
    EXPECT_DOUBLE_EQ(f.freq_hz(), 10000000.25);
}

TEST(UbxMessageMgaIni, SameLengthLayoutsSplitOnlyByType)
{
    EXPECT_EQ(mga_ini_pos_xyz::payload_length, mga_ini_pos_llh::payload_length);
    EXPECT_EQ(mga_ini_time_utc::payload_length, mga_ini_time_gnss::payload_length);
    EXPECT_EQ(mga_ini_clkd::payload_length, mga_ini_freq::payload_length);

    uint8_t payload[mga_ini_pos_xyz::payload_length] = {};
    spec<mga_ini_pos_xyz>::fill(payload, 0x00U);
    CASTLE_CONST message_view raw = raw_of<mga_ini_pos_xyz>(payload, sizeof(payload));
    mga_ini_pos_llh wrong{};
    EXPECT_FALSE(mga_ini_pos_llh::decode(raw, wrong));
}

} // namespace
