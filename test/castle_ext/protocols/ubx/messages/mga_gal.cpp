#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mga_gal.hpp"

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
struct spec<mga_gal_eph>
{
    /// Spec 3.13.4.1: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x01U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        castle::write_le16(&payload[4U], static_cast<uint16_t>(0x1404U)); // iodNav
        castle::write_le16s(&payload[6U], static_cast<int16_t>(-806)); // deltaN
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // m0
        castle::write_le32(&payload[12U], static_cast<uint32_t>(0x1C0C0C0CU)); // e
        castle::write_le32(&payload[16U], static_cast<uint32_t>(0x20101010U)); // sqrtA
        castle::write_le32s(&payload[20U], static_cast<int32_t>(-37913536)); // omega0
        castle::write_le32s(&payload[24U], static_cast<int32_t>(-42140800)); // i0
        castle::write_le32s(&payload[28U], static_cast<int32_t>(-46368064)); // omega
        castle::write_le32s(&payload[32U], static_cast<int32_t>(-50595328)); // omegaDot
        castle::write_le16s(&payload[36U], static_cast<int16_t>(-2276)); // iDot
        castle::write_le16s(&payload[38U], static_cast<int16_t>(-2374)); // cuc
        castle::write_le16s(&payload[40U], static_cast<int16_t>(-2472)); // cus
        castle::write_le16s(&payload[42U], static_cast<int16_t>(-2570)); // crc
        castle::write_le16s(&payload[44U], static_cast<int16_t>(-2668)); // crs
        castle::write_le16s(&payload[46U], static_cast<int16_t>(-2766)); // cic
        castle::write_le16s(&payload[48U], static_cast<int16_t>(-2864)); // cis
        castle::write_le16(&payload[50U], static_cast<uint16_t>(0x4232U)); // toe
        castle::write_le32s(&payload[52U], static_cast<int32_t>(-71731648)); // af0
        castle::write_le32s(&payload[56U], static_cast<int32_t>(-75958912)); // af1
        payload[60U] = static_cast<uint8_t>(static_cast<int8_t>(-61)); // af2
        payload[61U] = static_cast<uint8_t>(0xBCU); // sisaIndexE1
        castle::write_le16(&payload[62U], static_cast<uint16_t>(0x4E3EU)); // toc
        castle::write_le16s(&payload[64U], static_cast<int16_t>(-3648)); // bgdE1E5b
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[66U + i] = reserved_fill; }
        payload[68U] = static_cast<uint8_t>(0xEDU); // healthE1B
        payload[69U] = static_cast<uint8_t>(0xF4U); // dataValidityE1
        payload[70U] = static_cast<uint8_t>(0xFBU); // healthE5b
        payload[71U] = static_cast<uint8_t>(0x02U); // dataValidity
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[72U + i] = reserved_fill; }
    }

    static mga_gal_eph make()
    {
        mga_gal_eph m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.iod_nav = static_cast<uint16_t>(0x1404U);
        m.delta_n = static_cast<int16_t>(-806);
        m.m0 = static_cast<int32_t>(-25231744);
        m.e = static_cast<uint32_t>(0x1C0C0C0CU);
        m.sqrt_a = static_cast<uint32_t>(0x20101010U);
        m.omega0 = static_cast<int32_t>(-37913536);
        m.i0 = static_cast<int32_t>(-42140800);
        m.omega = static_cast<int32_t>(-46368064);
        m.omega_dot = static_cast<int32_t>(-50595328);
        m.i_dot = static_cast<int16_t>(-2276);
        m.cuc = static_cast<int16_t>(-2374);
        m.cus = static_cast<int16_t>(-2472);
        m.crc = static_cast<int16_t>(-2570);
        m.crs = static_cast<int16_t>(-2668);
        m.cic = static_cast<int16_t>(-2766);
        m.cis = static_cast<int16_t>(-2864);
        m.toe = static_cast<uint16_t>(0x4232U);
        m.af0 = static_cast<int32_t>(-71731648);
        m.af1 = static_cast<int32_t>(-75958912);
        m.af2 = static_cast<int8_t>(-61);
        m.sisa_index_e1_e5b = static_cast<uint8_t>(0xBCU);
        m.toc = static_cast<uint16_t>(0x4E3EU);
        m.bgd_e1_e5b = static_cast<int16_t>(-3648);
        m.health_e1b = static_cast<uint8_t>(0xEDU);
        m.data_validity_e1b = static_cast<uint8_t>(0xF4U);
        m.health_e5b = static_cast<uint8_t>(0xFBU);
        m.data_validity_e5b = static_cast<uint8_t>(0x02U);
        return m;
    }

    static void expect(CASTLE_CONST mga_gal_eph& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.iod_nav, static_cast<uint16_t>(0x1404U));
        EXPECT_EQ(m.delta_n, static_cast<int16_t>(-806));
        EXPECT_EQ(m.m0, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.e, static_cast<uint32_t>(0x1C0C0C0CU));
        EXPECT_EQ(m.sqrt_a, static_cast<uint32_t>(0x20101010U));
        EXPECT_EQ(m.omega0, static_cast<int32_t>(-37913536));
        EXPECT_EQ(m.i0, static_cast<int32_t>(-42140800));
        EXPECT_EQ(m.omega, static_cast<int32_t>(-46368064));
        EXPECT_EQ(m.omega_dot, static_cast<int32_t>(-50595328));
        EXPECT_EQ(m.i_dot, static_cast<int16_t>(-2276));
        EXPECT_EQ(m.cuc, static_cast<int16_t>(-2374));
        EXPECT_EQ(m.cus, static_cast<int16_t>(-2472));
        EXPECT_EQ(m.crc, static_cast<int16_t>(-2570));
        EXPECT_EQ(m.crs, static_cast<int16_t>(-2668));
        EXPECT_EQ(m.cic, static_cast<int16_t>(-2766));
        EXPECT_EQ(m.cis, static_cast<int16_t>(-2864));
        EXPECT_EQ(m.toe, static_cast<uint16_t>(0x4232U));
        EXPECT_EQ(m.af0, static_cast<int32_t>(-71731648));
        EXPECT_EQ(m.af1, static_cast<int32_t>(-75958912));
        EXPECT_EQ(m.af2, static_cast<int8_t>(-61));
        EXPECT_EQ(m.sisa_index_e1_e5b, static_cast<uint8_t>(0xBCU));
        EXPECT_EQ(m.toc, static_cast<uint16_t>(0x4E3EU));
        EXPECT_EQ(m.bgd_e1_e5b, static_cast<int16_t>(-3648));
        EXPECT_EQ(m.health_e1b, static_cast<uint8_t>(0xEDU));
        EXPECT_EQ(m.data_validity_e1b, static_cast<uint8_t>(0xF4U));
        EXPECT_EQ(m.health_e5b, static_cast<uint8_t>(0xFBU));
        EXPECT_EQ(m.data_validity_e5b, static_cast<uint8_t>(0x02U));
    }
};

template <>
struct spec<mga_gal_alm>
{
    /// Spec 3.13.4.2: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x02U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        payload[4U] = static_cast<uint8_t>(0x2DU); // ioda
        payload[5U] = static_cast<uint8_t>(0x34U); // almWNa
        castle::write_le16(&payload[6U], static_cast<uint16_t>(0x1606U)); // toa
        castle::write_le16s(&payload[8U], static_cast<int16_t>(-904)); // deltaSqrtA
        castle::write_le16(&payload[10U], static_cast<uint16_t>(0x1A0AU)); // e
        castle::write_le16s(&payload[12U], static_cast<int16_t>(-1100)); // deltaI
        castle::write_le16s(&payload[14U], static_cast<int16_t>(-1198)); // omega0
        castle::write_le16s(&payload[16U], static_cast<int16_t>(-1296)); // omegaDot
        castle::write_le16s(&payload[18U], static_cast<int16_t>(-1394)); // omega
        castle::write_le16s(&payload[20U], static_cast<int16_t>(-1492)); // m0
        castle::write_le16s(&payload[22U], static_cast<int16_t>(-1590)); // af0
        castle::write_le16s(&payload[24U], static_cast<int16_t>(-1688)); // af1
        payload[26U] = static_cast<uint8_t>(0xC7U); // healthE1B
        payload[27U] = static_cast<uint8_t>(0xCEU); // healthE5b
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[28U + i] = reserved_fill; }
    }

    static mga_gal_alm make()
    {
        mga_gal_alm m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.ioda = static_cast<uint8_t>(0x2DU);
        m.alm_wn_a = static_cast<uint8_t>(0x34U);
        m.toa = static_cast<uint16_t>(0x1606U);
        m.delta_sqrt_a = static_cast<int16_t>(-904);
        m.e = static_cast<uint16_t>(0x1A0AU);
        m.delta_i = static_cast<int16_t>(-1100);
        m.omega0 = static_cast<int16_t>(-1198);
        m.omega_dot = static_cast<int16_t>(-1296);
        m.omega = static_cast<int16_t>(-1394);
        m.m0 = static_cast<int16_t>(-1492);
        m.af0 = static_cast<int16_t>(-1590);
        m.af1 = static_cast<int16_t>(-1688);
        m.health_e1b = static_cast<uint8_t>(0xC7U);
        m.health_e5b = static_cast<uint8_t>(0xCEU);
        return m;
    }

    static void expect(CASTLE_CONST mga_gal_alm& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.ioda, static_cast<uint8_t>(0x2DU));
        EXPECT_EQ(m.alm_wn_a, static_cast<uint8_t>(0x34U));
        EXPECT_EQ(m.toa, static_cast<uint16_t>(0x1606U));
        EXPECT_EQ(m.delta_sqrt_a, static_cast<int16_t>(-904));
        EXPECT_EQ(m.e, static_cast<uint16_t>(0x1A0AU));
        EXPECT_EQ(m.delta_i, static_cast<int16_t>(-1100));
        EXPECT_EQ(m.omega0, static_cast<int16_t>(-1198));
        EXPECT_EQ(m.omega_dot, static_cast<int16_t>(-1296));
        EXPECT_EQ(m.omega, static_cast<int16_t>(-1394));
        EXPECT_EQ(m.m0, static_cast<int16_t>(-1492));
        EXPECT_EQ(m.af0, static_cast<int16_t>(-1590));
        EXPECT_EQ(m.af1, static_cast<int16_t>(-1688));
        EXPECT_EQ(m.health_e1b, static_cast<uint8_t>(0xC7U));
        EXPECT_EQ(m.health_e5b, static_cast<uint8_t>(0xCEU));
    }
};

template <>
struct spec<mga_gal_timeoffset>
{
    /// Spec 3.13.4.3: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x03U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le16s(&payload[4U], static_cast<int16_t>(-708)); // a0G
        castle::write_le16s(&payload[6U], static_cast<int16_t>(-806)); // a1G
        payload[8U] = static_cast<uint8_t>(0x49U); // t0G
        payload[9U] = static_cast<uint8_t>(0x50U); // wn0G
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[10U + i] = reserved_fill; }
    }

    static mga_gal_timeoffset make()
    {
        mga_gal_timeoffset m{};
        m.a0_g = static_cast<int16_t>(-708);
        m.a1_g = static_cast<int16_t>(-806);
        m.t0_g = static_cast<uint8_t>(0x49U);
        m.wn0_g = static_cast<uint8_t>(0x50U);
        return m;
    }

    static void expect(CASTLE_CONST mga_gal_timeoffset& m)
    {
        EXPECT_EQ(m.a0_g, static_cast<int16_t>(-708));
        EXPECT_EQ(m.a1_g, static_cast<int16_t>(-806));
        EXPECT_EQ(m.t0_g, static_cast<uint8_t>(0x49U));
        EXPECT_EQ(m.wn0_g, static_cast<uint8_t>(0x50U));
    }
};

template <>
struct spec<mga_gal_utc>
{
    /// Spec 3.13.4.4: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x05U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // a0
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // a1
        payload[12U] = static_cast<uint8_t>(static_cast<int8_t>(-13)); // dtLS
        payload[13U] = static_cast<uint8_t>(0x6CU); // tot
        payload[14U] = static_cast<uint8_t>(0x73U); // wnt
        payload[15U] = static_cast<uint8_t>(0x7AU); // wnLSF
        payload[16U] = static_cast<uint8_t>(0x81U); // dN
        payload[17U] = static_cast<uint8_t>(static_cast<int8_t>(-18)); // dTLSF
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[18U + i] = reserved_fill; }
    }

    static mga_gal_utc make()
    {
        mga_gal_utc m{};
        m.a0 = static_cast<int32_t>(-21004480);
        m.a1 = static_cast<int32_t>(-25231744);
        m.dt_ls = static_cast<int8_t>(-13);
        m.tot = static_cast<uint8_t>(0x6CU);
        m.wnt = static_cast<uint8_t>(0x73U);
        m.wn_lsf = static_cast<uint8_t>(0x7AU);
        m.dn = static_cast<uint8_t>(0x81U);
        m.dt_lsf = static_cast<int8_t>(-18);
        return m;
    }

    static void expect(CASTLE_CONST mga_gal_utc& m)
    {
        EXPECT_EQ(m.a0, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.a1, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.dt_ls, static_cast<int8_t>(-13));
        EXPECT_EQ(m.tot, static_cast<uint8_t>(0x6CU));
        EXPECT_EQ(m.wnt, static_cast<uint8_t>(0x73U));
        EXPECT_EQ(m.wn_lsf, static_cast<uint8_t>(0x7AU));
        EXPECT_EQ(m.dn, static_cast<uint8_t>(0x81U));
        EXPECT_EQ(m.dt_lsf, static_cast<int8_t>(-18));
    }
};

TEST(UbxMessageMgaGal, EphSpecOffsets)
{
    check_spec_offsets<mga_gal_eph>();
}

TEST(UbxMessageMgaGal, EphRoundTrip)
{
    check_round_trip<mga_gal_eph>();
}

TEST(UbxMessageMgaGal, EphRejects)
{
    check_rejects_typed<mga_gal_eph>();
}

TEST(UbxMessageMgaGal, EphTooSmallBuffer)
{
    check_too_small<mga_gal_eph>();
}

TEST(UbxMessageMgaGal, EphDispatchExclusive)
{
    check_exclusive<mga_gal_eph, mga_gal_eph, mga_gal_alm, mga_gal_timeoffset, mga_gal_utc>();
}

TEST(UbxMessageMgaGal, AlmSpecOffsets)
{
    check_spec_offsets<mga_gal_alm>();
}

TEST(UbxMessageMgaGal, AlmRoundTrip)
{
    check_round_trip<mga_gal_alm>();
}

TEST(UbxMessageMgaGal, AlmRejects)
{
    check_rejects_typed<mga_gal_alm>();
}

TEST(UbxMessageMgaGal, AlmTooSmallBuffer)
{
    check_too_small<mga_gal_alm>();
}

TEST(UbxMessageMgaGal, AlmDispatchExclusive)
{
    check_exclusive<mga_gal_alm, mga_gal_eph, mga_gal_alm, mga_gal_timeoffset, mga_gal_utc>();
}

TEST(UbxMessageMgaGal, TimeoffsetSpecOffsets)
{
    check_spec_offsets<mga_gal_timeoffset>();
}

TEST(UbxMessageMgaGal, TimeoffsetRoundTrip)
{
    check_round_trip<mga_gal_timeoffset>();
}

TEST(UbxMessageMgaGal, TimeoffsetRejects)
{
    check_rejects_typed<mga_gal_timeoffset>();
}

TEST(UbxMessageMgaGal, TimeoffsetTooSmallBuffer)
{
    check_too_small<mga_gal_timeoffset>();
}

TEST(UbxMessageMgaGal, TimeoffsetDispatchExclusive)
{
    check_exclusive<mga_gal_timeoffset, mga_gal_eph, mga_gal_alm, mga_gal_timeoffset, mga_gal_utc>();
}

TEST(UbxMessageMgaGal, UtcSpecOffsets)
{
    check_spec_offsets<mga_gal_utc>();
}

TEST(UbxMessageMgaGal, UtcRoundTrip)
{
    check_round_trip<mga_gal_utc>();
}

TEST(UbxMessageMgaGal, UtcRejects)
{
    check_rejects_typed<mga_gal_utc>();
}

TEST(UbxMessageMgaGal, UtcTooSmallBuffer)
{
    check_too_small<mga_gal_utc>();
}

TEST(UbxMessageMgaGal, UtcDispatchExclusive)
{
    check_exclusive<mga_gal_utc, mga_gal_eph, mga_gal_alm, mga_gal_timeoffset, mga_gal_utc>();
}

TEST(UbxMessageMgaGal, ConstantsMatchSpec)
{
    EXPECT_EQ(mga_gal_eph::msg_id, UBX_ID_MGA_GAL);
    EXPECT_EQ(MGA_GAL_TYPE_EPH, 0x01U);
    EXPECT_EQ(MGA_GAL_TYPE_ALM, 0x02U);
    EXPECT_EQ(MGA_GAL_TYPE_TIMEOFFSET, 0x03U);
    EXPECT_EQ(MGA_GAL_TYPE_UTC, 0x05U);
}

} // namespace
