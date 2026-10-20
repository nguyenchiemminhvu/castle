#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mga_bds.hpp"

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
struct spec<mga_bds_eph>
{
    /// Spec 3.13.2.1: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x01U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        payload[4U] = static_cast<uint8_t>(0x2DU); // SatH1
        payload[5U] = static_cast<uint8_t>(0x34U); // IODC
        castle::write_le16s(&payload[6U], static_cast<int16_t>(-806)); // a2
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // a1
        castle::write_le32s(&payload[12U], static_cast<int32_t>(-29459008)); // a0
        castle::write_le32(&payload[16U], static_cast<uint32_t>(0x20101010U)); // toc
        castle::write_le16s(&payload[20U], static_cast<int16_t>(-1492)); // TGD1
        payload[22U] = static_cast<uint8_t>(0xABU); // URAI
        payload[23U] = static_cast<uint8_t>(0xB2U); // IODE
        castle::write_le32(&payload[24U], static_cast<uint32_t>(0x28181818U)); // toe
        castle::write_le32(&payload[28U], static_cast<uint32_t>(0x2C1C1C1CU)); // sqrtA
        castle::write_le32(&payload[32U], static_cast<uint32_t>(0x30202020U)); // e
        castle::write_le32s(&payload[36U], static_cast<int32_t>(-54822592)); // omega
        castle::write_le16s(&payload[40U], static_cast<int16_t>(-2472)); // Deltan
        castle::write_le16s(&payload[42U], static_cast<int16_t>(-2570)); // IDOT
        castle::write_le32s(&payload[44U], static_cast<int32_t>(-63277120)); // M0
        castle::write_le32s(&payload[48U], static_cast<int32_t>(-67504384)); // Omega0
        castle::write_le32s(&payload[52U], static_cast<int32_t>(-71731648)); // OmegaDot
        castle::write_le32s(&payload[56U], static_cast<int32_t>(-75958912)); // i0
        castle::write_le32s(&payload[60U], static_cast<int32_t>(-80186176)); // Cuc
        castle::write_le32s(&payload[64U], static_cast<int32_t>(-84413440)); // Cus
        castle::write_le32s(&payload[68U], static_cast<int32_t>(-88640704)); // Crc
        castle::write_le32s(&payload[72U], static_cast<int32_t>(-92867968)); // Crs
        castle::write_le32s(&payload[76U], static_cast<int32_t>(-97095232)); // Cic
        castle::write_le32s(&payload[80U], static_cast<int32_t>(-101322496)); // Cis
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[84U + i] = reserved_fill; }
    }

    static mga_bds_eph make()
    {
        mga_bds_eph m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.sat_h1 = static_cast<uint8_t>(0x2DU);
        m.iodc = static_cast<uint8_t>(0x34U);
        m.a2 = static_cast<int16_t>(-806);
        m.a1 = static_cast<int32_t>(-25231744);
        m.a0 = static_cast<int32_t>(-29459008);
        m.toc = static_cast<uint32_t>(0x20101010U);
        m.tgd1 = static_cast<int16_t>(-1492);
        m.urai = static_cast<uint8_t>(0xABU);
        m.iode = static_cast<uint8_t>(0xB2U);
        m.toe = static_cast<uint32_t>(0x28181818U);
        m.sqrt_a = static_cast<uint32_t>(0x2C1C1C1CU);
        m.e = static_cast<uint32_t>(0x30202020U);
        m.omega = static_cast<int32_t>(-54822592);
        m.delta_n = static_cast<int16_t>(-2472);
        m.idot = static_cast<int16_t>(-2570);
        m.m0 = static_cast<int32_t>(-63277120);
        m.omega0 = static_cast<int32_t>(-67504384);
        m.omega_dot = static_cast<int32_t>(-71731648);
        m.i0 = static_cast<int32_t>(-75958912);
        m.cuc = static_cast<int32_t>(-80186176);
        m.cus = static_cast<int32_t>(-84413440);
        m.crc = static_cast<int32_t>(-88640704);
        m.crs = static_cast<int32_t>(-92867968);
        m.cic = static_cast<int32_t>(-97095232);
        m.cis = static_cast<int32_t>(-101322496);
        return m;
    }

    static void expect(CASTLE_CONST mga_bds_eph& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.sat_h1, static_cast<uint8_t>(0x2DU));
        EXPECT_EQ(m.iodc, static_cast<uint8_t>(0x34U));
        EXPECT_EQ(m.a2, static_cast<int16_t>(-806));
        EXPECT_EQ(m.a1, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.a0, static_cast<int32_t>(-29459008));
        EXPECT_EQ(m.toc, static_cast<uint32_t>(0x20101010U));
        EXPECT_EQ(m.tgd1, static_cast<int16_t>(-1492));
        EXPECT_EQ(m.urai, static_cast<uint8_t>(0xABU));
        EXPECT_EQ(m.iode, static_cast<uint8_t>(0xB2U));
        EXPECT_EQ(m.toe, static_cast<uint32_t>(0x28181818U));
        EXPECT_EQ(m.sqrt_a, static_cast<uint32_t>(0x2C1C1C1CU));
        EXPECT_EQ(m.e, static_cast<uint32_t>(0x30202020U));
        EXPECT_EQ(m.omega, static_cast<int32_t>(-54822592));
        EXPECT_EQ(m.delta_n, static_cast<int16_t>(-2472));
        EXPECT_EQ(m.idot, static_cast<int16_t>(-2570));
        EXPECT_EQ(m.m0, static_cast<int32_t>(-63277120));
        EXPECT_EQ(m.omega0, static_cast<int32_t>(-67504384));
        EXPECT_EQ(m.omega_dot, static_cast<int32_t>(-71731648));
        EXPECT_EQ(m.i0, static_cast<int32_t>(-75958912));
        EXPECT_EQ(m.cuc, static_cast<int32_t>(-80186176));
        EXPECT_EQ(m.cus, static_cast<int32_t>(-84413440));
        EXPECT_EQ(m.crc, static_cast<int32_t>(-88640704));
        EXPECT_EQ(m.crs, static_cast<int32_t>(-92867968));
        EXPECT_EQ(m.cic, static_cast<int32_t>(-97095232));
        EXPECT_EQ(m.cis, static_cast<int32_t>(-101322496));
    }
};

template <>
struct spec<mga_bds_alm>
{
    /// Spec 3.13.2.2: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x02U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        payload[4U] = static_cast<uint8_t>(0x2DU); // Wna
        payload[5U] = static_cast<uint8_t>(0x34U); // toa
        castle::write_le16s(&payload[6U], static_cast<int16_t>(-806)); // deltaI
        castle::write_le32(&payload[8U], static_cast<uint32_t>(0x18080808U)); // sqrtA
        castle::write_le32(&payload[12U], static_cast<uint32_t>(0x1C0C0C0CU)); // e
        castle::write_le32s(&payload[16U], static_cast<int32_t>(-33686272)); // omega
        castle::write_le32s(&payload[20U], static_cast<int32_t>(-37913536)); // M0
        castle::write_le32s(&payload[24U], static_cast<int32_t>(-42140800)); // Omega0
        castle::write_le32s(&payload[28U], static_cast<int32_t>(-46368064)); // omegaDot
        castle::write_le16s(&payload[32U], static_cast<int16_t>(-2080)); // a0
        castle::write_le16s(&payload[34U], static_cast<int16_t>(-2178)); // a1
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[36U + i] = reserved_fill; }
    }

    static mga_bds_alm make()
    {
        mga_bds_alm m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.wna = static_cast<uint8_t>(0x2DU);
        m.toa = static_cast<uint8_t>(0x34U);
        m.delta_i = static_cast<int16_t>(-806);
        m.sqrt_a = static_cast<uint32_t>(0x18080808U);
        m.e = static_cast<uint32_t>(0x1C0C0C0CU);
        m.omega = static_cast<int32_t>(-33686272);
        m.m0 = static_cast<int32_t>(-37913536);
        m.omega0 = static_cast<int32_t>(-42140800);
        m.omega_dot = static_cast<int32_t>(-46368064);
        m.a0 = static_cast<int16_t>(-2080);
        m.a1 = static_cast<int16_t>(-2178);
        return m;
    }

    static void expect(CASTLE_CONST mga_bds_alm& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.wna, static_cast<uint8_t>(0x2DU));
        EXPECT_EQ(m.toa, static_cast<uint8_t>(0x34U));
        EXPECT_EQ(m.delta_i, static_cast<int16_t>(-806));
        EXPECT_EQ(m.sqrt_a, static_cast<uint32_t>(0x18080808U));
        EXPECT_EQ(m.e, static_cast<uint32_t>(0x1C0C0C0CU));
        EXPECT_EQ(m.omega, static_cast<int32_t>(-33686272));
        EXPECT_EQ(m.m0, static_cast<int32_t>(-37913536));
        EXPECT_EQ(m.omega0, static_cast<int32_t>(-42140800));
        EXPECT_EQ(m.omega_dot, static_cast<int32_t>(-46368064));
        EXPECT_EQ(m.a0, static_cast<int16_t>(-2080));
        EXPECT_EQ(m.a1, static_cast<int16_t>(-2178));
    }
};

template <>
struct spec<mga_bds_health>
{
    /// Spec 3.13.2.3: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x04U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        for (castle::size_type i = 0U; i < 30U; ++i) { castle::write_le16(&payload[4U + (i * 2U)], static_cast<uint16_t>(0x0100U + (i * 0x11U))); } // healthCode
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[64U + i] = reserved_fill; }
    }

    static mga_bds_health make()
    {
        mga_bds_health m{};
        for (castle::size_type i = 0U; i < 30U; ++i) { m.health_code[i] = static_cast<uint16_t>(0x0100U + (i * 0x11U)); }
        return m;
    }

    static void expect(CASTLE_CONST mga_bds_health& m)
    {
        for (castle::size_type i = 0U; i < 30U; ++i) { EXPECT_EQ(m.health_code[i], static_cast<uint16_t>(0x0100U + (i * 0x11U))); }
    }
};

template <>
struct spec<mga_bds_utc>
{
    /// Spec 3.13.2.4: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x05U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // a0UTC
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // a1UTC
        payload[12U] = static_cast<uint8_t>(static_cast<int8_t>(-13)); // dtLS
        payload[13U] = reserved_fill;
        payload[14U] = static_cast<uint8_t>(0x73U); // wnRec
        payload[15U] = static_cast<uint8_t>(0x7AU); // wnLSF
        payload[16U] = static_cast<uint8_t>(0x81U); // dN
        payload[17U] = static_cast<uint8_t>(static_cast<int8_t>(-18)); // dtLSF
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[18U + i] = reserved_fill; }
    }

    static mga_bds_utc make()
    {
        mga_bds_utc m{};
        m.a0_utc = static_cast<int32_t>(-21004480);
        m.a1_utc = static_cast<int32_t>(-25231744);
        m.dt_ls = static_cast<int8_t>(-13);
        m.wn_rec = static_cast<uint8_t>(0x73U);
        m.wn_lsf = static_cast<uint8_t>(0x7AU);
        m.dn = static_cast<uint8_t>(0x81U);
        m.dt_lsf = static_cast<int8_t>(-18);
        return m;
    }

    static void expect(CASTLE_CONST mga_bds_utc& m)
    {
        EXPECT_EQ(m.a0_utc, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.a1_utc, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.dt_ls, static_cast<int8_t>(-13));
        EXPECT_EQ(m.wn_rec, static_cast<uint8_t>(0x73U));
        EXPECT_EQ(m.wn_lsf, static_cast<uint8_t>(0x7AU));
        EXPECT_EQ(m.dn, static_cast<uint8_t>(0x81U));
        EXPECT_EQ(m.dt_lsf, static_cast<int8_t>(-18));
    }
};

template <>
struct spec<mga_bds_iono>
{
    /// Spec 3.13.2.5: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x06U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        payload[4U] = static_cast<uint8_t>(static_cast<int8_t>(-5)); // alpha0
        payload[5U] = static_cast<uint8_t>(static_cast<int8_t>(-6)); // alpha1
        payload[6U] = static_cast<uint8_t>(static_cast<int8_t>(-7)); // alpha2
        payload[7U] = static_cast<uint8_t>(static_cast<int8_t>(-8)); // alpha3
        payload[8U] = static_cast<uint8_t>(static_cast<int8_t>(-9)); // beta0
        payload[9U] = static_cast<uint8_t>(static_cast<int8_t>(-10)); // beta1
        payload[10U] = static_cast<uint8_t>(static_cast<int8_t>(-11)); // beta2
        payload[11U] = static_cast<uint8_t>(static_cast<int8_t>(-12)); // beta3
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[12U + i] = reserved_fill; }
    }

    static mga_bds_iono make()
    {
        mga_bds_iono m{};
        m.alpha[0U] = static_cast<int8_t>(-5);
        m.alpha[1U] = static_cast<int8_t>(-6);
        m.alpha[2U] = static_cast<int8_t>(-7);
        m.alpha[3U] = static_cast<int8_t>(-8);
        m.beta[0U] = static_cast<int8_t>(-9);
        m.beta[1U] = static_cast<int8_t>(-10);
        m.beta[2U] = static_cast<int8_t>(-11);
        m.beta[3U] = static_cast<int8_t>(-12);
        return m;
    }

    static void expect(CASTLE_CONST mga_bds_iono& m)
    {
        EXPECT_EQ(m.alpha[0U], static_cast<int8_t>(-5));
        EXPECT_EQ(m.alpha[1U], static_cast<int8_t>(-6));
        EXPECT_EQ(m.alpha[2U], static_cast<int8_t>(-7));
        EXPECT_EQ(m.alpha[3U], static_cast<int8_t>(-8));
        EXPECT_EQ(m.beta[0U], static_cast<int8_t>(-9));
        EXPECT_EQ(m.beta[1U], static_cast<int8_t>(-10));
        EXPECT_EQ(m.beta[2U], static_cast<int8_t>(-11));
        EXPECT_EQ(m.beta[3U], static_cast<int8_t>(-12));
    }
};

TEST(UbxMessageMgaBds, EphSpecOffsets)
{
    check_spec_offsets<mga_bds_eph>();
}

TEST(UbxMessageMgaBds, EphRoundTrip)
{
    check_round_trip<mga_bds_eph>();
}

TEST(UbxMessageMgaBds, EphRejects)
{
    check_rejects_typed<mga_bds_eph>();
}

TEST(UbxMessageMgaBds, EphTooSmallBuffer)
{
    check_too_small<mga_bds_eph>();
}

TEST(UbxMessageMgaBds, EphDispatchExclusive)
{
    check_exclusive<mga_bds_eph, mga_bds_eph, mga_bds_alm, mga_bds_health, mga_bds_utc, mga_bds_iono>();
}

TEST(UbxMessageMgaBds, AlmSpecOffsets)
{
    check_spec_offsets<mga_bds_alm>();
}

TEST(UbxMessageMgaBds, AlmRoundTrip)
{
    check_round_trip<mga_bds_alm>();
}

TEST(UbxMessageMgaBds, AlmRejects)
{
    check_rejects_typed<mga_bds_alm>();
}

TEST(UbxMessageMgaBds, AlmTooSmallBuffer)
{
    check_too_small<mga_bds_alm>();
}

TEST(UbxMessageMgaBds, AlmDispatchExclusive)
{
    check_exclusive<mga_bds_alm, mga_bds_eph, mga_bds_alm, mga_bds_health, mga_bds_utc, mga_bds_iono>();
}

TEST(UbxMessageMgaBds, HealthSpecOffsets)
{
    check_spec_offsets<mga_bds_health>();
}

TEST(UbxMessageMgaBds, HealthRoundTrip)
{
    check_round_trip<mga_bds_health>();
}

TEST(UbxMessageMgaBds, HealthRejects)
{
    check_rejects_typed<mga_bds_health>();
}

TEST(UbxMessageMgaBds, HealthTooSmallBuffer)
{
    check_too_small<mga_bds_health>();
}

TEST(UbxMessageMgaBds, HealthDispatchExclusive)
{
    check_exclusive<mga_bds_health, mga_bds_eph, mga_bds_alm, mga_bds_health, mga_bds_utc, mga_bds_iono>();
}

TEST(UbxMessageMgaBds, UtcSpecOffsets)
{
    check_spec_offsets<mga_bds_utc>();
}

TEST(UbxMessageMgaBds, UtcRoundTrip)
{
    check_round_trip<mga_bds_utc>();
}

TEST(UbxMessageMgaBds, UtcRejects)
{
    check_rejects_typed<mga_bds_utc>();
}

TEST(UbxMessageMgaBds, UtcTooSmallBuffer)
{
    check_too_small<mga_bds_utc>();
}

TEST(UbxMessageMgaBds, UtcDispatchExclusive)
{
    check_exclusive<mga_bds_utc, mga_bds_eph, mga_bds_alm, mga_bds_health, mga_bds_utc, mga_bds_iono>();
}

TEST(UbxMessageMgaBds, IonoSpecOffsets)
{
    check_spec_offsets<mga_bds_iono>();
}

TEST(UbxMessageMgaBds, IonoRoundTrip)
{
    check_round_trip<mga_bds_iono>();
}

TEST(UbxMessageMgaBds, IonoRejects)
{
    check_rejects_typed<mga_bds_iono>();
}

TEST(UbxMessageMgaBds, IonoTooSmallBuffer)
{
    check_too_small<mga_bds_iono>();
}

TEST(UbxMessageMgaBds, IonoDispatchExclusive)
{
    check_exclusive<mga_bds_iono, mga_bds_eph, mga_bds_alm, mga_bds_health, mga_bds_utc, mga_bds_iono>();
}

TEST(UbxMessageMgaBds, ConstantsMatchSpec)
{
    EXPECT_EQ(mga_bds_eph::msg_id, UBX_ID_MGA_BDS);
    EXPECT_EQ(MGA_BDS_TYPE_EPH, 0x01U);
    EXPECT_EQ(MGA_BDS_TYPE_ALM, 0x02U);
    EXPECT_EQ(MGA_BDS_TYPE_HEALTH, 0x04U);
    EXPECT_EQ(MGA_BDS_TYPE_UTC, 0x05U);
    EXPECT_EQ(MGA_BDS_TYPE_IONO, 0x06U);
    EXPECT_EQ(mga_bds_health::NUM_SVS, 30U);
}

TEST(UbxMessageMgaBds, HealthIsSixteenBitPerSvWithNineBitCode)
{
    uint8_t payload[mga_bds_health::payload_length] = {};
    spec<mga_bds_health>::fill(payload, 0x00U);
    castle::write_le16(&payload[4U], 0xFE01U);         // SV 1: spare bits set, code 0x001
    castle::write_le16(&payload[4U + (29U * 2U)], 0x01FFU); // SV 30
    mga_bds_health out{};
    ASSERT_TRUE(mga_bds_health::decode(raw_of<mga_bds_health>(payload, sizeof(payload)), out));
    EXPECT_EQ(out.health_code[0U], 0xFE01U);
    EXPECT_EQ(static_cast<uint16_t>(out.health_code[0U] & mga_bds_health::HEALTH_CODE_MASK), 0x0001U);
    EXPECT_EQ(static_cast<uint16_t>(out.health_code[29U] & mga_bds_health::HEALTH_CODE_MASK), 0x01FFU);
}

} // namespace
