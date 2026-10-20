#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mga_ack.hpp"
#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"

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
struct spec<mga_ack>
{
    /// Spec 3.13.1.1: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = static_cast<uint8_t>(0x01U); // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x05U); // infoCode
        payload[3U] = static_cast<uint8_t>(0x26U); // msgId
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[4U + i] = static_cast<uint8_t>(0x40U + i); } // msgPayload
    }

    static mga_ack make()
    {
        mga_ack m{};
        m.type = static_cast<mga_ack_type>(0x01U);
        m.info_code = static_cast<mga_ack_info_code>(0x05U);
        m.acked_msg_id = static_cast<uint8_t>(0x26U);
        for (castle::size_type i = 0U; i < 4U; ++i) { m.msg_payload_start[i] = static_cast<uint8_t>(0x40U + i); }
        return m;
    }

    static void expect(CASTLE_CONST mga_ack& m)
    {
        EXPECT_EQ(static_cast<uint8_t>(m.type), static_cast<uint8_t>(0x01U));
        EXPECT_EQ(static_cast<uint8_t>(m.info_code), static_cast<uint8_t>(0x05U));
        EXPECT_EQ(m.acked_msg_id, static_cast<uint8_t>(0x26U));
        for (castle::size_type i = 0U; i < 4U; ++i) { EXPECT_EQ(m.msg_payload_start[i], static_cast<uint8_t>(0x40U + i)); }
    }
};

template <>
struct spec<mga_gps_eph>
{
    /// Spec 3.13.6.1: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x01U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        payload[4U] = static_cast<uint8_t>(0x2DU); // fitInterval
        payload[5U] = static_cast<uint8_t>(0x34U); // uraIndex
        payload[6U] = static_cast<uint8_t>(0x3BU); // svHealth
        payload[7U] = static_cast<uint8_t>(static_cast<int8_t>(-8)); // tgd
        castle::write_le16(&payload[8U], static_cast<uint16_t>(0x1808U)); // iodc
        castle::write_le16(&payload[10U], static_cast<uint16_t>(0x1A0AU)); // toc
        payload[12U] = reserved_fill;
        payload[13U] = static_cast<uint8_t>(static_cast<int8_t>(-14)); // af2
        castle::write_le16s(&payload[14U], static_cast<int16_t>(-1198)); // af1
        castle::write_le32s(&payload[16U], static_cast<int32_t>(-33686272)); // af0
        castle::write_le16s(&payload[20U], static_cast<int16_t>(-1492)); // crs
        castle::write_le16s(&payload[22U], static_cast<int16_t>(-1590)); // deltaN
        castle::write_le32s(&payload[24U], static_cast<int32_t>(-42140800)); // m0
        castle::write_le16s(&payload[28U], static_cast<int16_t>(-1884)); // cuc
        castle::write_le16s(&payload[30U], static_cast<int16_t>(-1982)); // cus
        castle::write_le32(&payload[32U], static_cast<uint32_t>(0x30202020U)); // e
        castle::write_le32(&payload[36U], static_cast<uint32_t>(0x34242424U)); // sqrtA
        castle::write_le16(&payload[40U], static_cast<uint16_t>(0x3828U)); // toe
        castle::write_le16s(&payload[42U], static_cast<int16_t>(-2570)); // cic
        castle::write_le32s(&payload[44U], static_cast<int32_t>(-63277120)); // omega0
        castle::write_le16s(&payload[48U], static_cast<int16_t>(-2864)); // cis
        castle::write_le16s(&payload[50U], static_cast<int16_t>(-2962)); // crc
        castle::write_le32s(&payload[52U], static_cast<int32_t>(-71731648)); // i0
        castle::write_le32s(&payload[56U], static_cast<int32_t>(-75958912)); // omega
        castle::write_le32s(&payload[60U], static_cast<int32_t>(-80186176)); // omegaDot
        castle::write_le16s(&payload[64U], static_cast<int16_t>(-3648)); // idot
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[66U + i] = reserved_fill; }
    }

    static mga_gps_eph make()
    {
        mga_gps_eph m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.fit_interval = static_cast<uint8_t>(0x2DU);
        m.ura_index = static_cast<uint8_t>(0x34U);
        m.sv_health = static_cast<uint8_t>(0x3BU);
        m.tgd = static_cast<int8_t>(-8);
        m.iodc = static_cast<uint16_t>(0x1808U);
        m.toc = static_cast<uint16_t>(0x1A0AU);
        m.af2 = static_cast<int8_t>(-14);
        m.af1 = static_cast<int16_t>(-1198);
        m.af0 = static_cast<int32_t>(-33686272);
        m.crs = static_cast<int16_t>(-1492);
        m.delta_n = static_cast<int16_t>(-1590);
        m.m0 = static_cast<int32_t>(-42140800);
        m.cuc = static_cast<int16_t>(-1884);
        m.cus = static_cast<int16_t>(-1982);
        m.e = static_cast<uint32_t>(0x30202020U);
        m.sqrt_a = static_cast<uint32_t>(0x34242424U);
        m.toe = static_cast<uint16_t>(0x3828U);
        m.cic = static_cast<int16_t>(-2570);
        m.omega0 = static_cast<int32_t>(-63277120);
        m.cis = static_cast<int16_t>(-2864);
        m.crc = static_cast<int16_t>(-2962);
        m.i0 = static_cast<int32_t>(-71731648);
        m.omega = static_cast<int32_t>(-75958912);
        m.omega_dot = static_cast<int32_t>(-80186176);
        m.idot = static_cast<int16_t>(-3648);
        return m;
    }

    static void expect(CASTLE_CONST mga_gps_eph& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.fit_interval, static_cast<uint8_t>(0x2DU));
        EXPECT_EQ(m.ura_index, static_cast<uint8_t>(0x34U));
        EXPECT_EQ(m.sv_health, static_cast<uint8_t>(0x3BU));
        EXPECT_EQ(m.tgd, static_cast<int8_t>(-8));
        EXPECT_EQ(m.iodc, static_cast<uint16_t>(0x1808U));
        EXPECT_EQ(m.toc, static_cast<uint16_t>(0x1A0AU));
        EXPECT_EQ(m.af2, static_cast<int8_t>(-14));
        EXPECT_EQ(m.af1, static_cast<int16_t>(-1198));
        EXPECT_EQ(m.af0, static_cast<int32_t>(-33686272));
        EXPECT_EQ(m.crs, static_cast<int16_t>(-1492));
        EXPECT_EQ(m.delta_n, static_cast<int16_t>(-1590));
        EXPECT_EQ(m.m0, static_cast<int32_t>(-42140800));
        EXPECT_EQ(m.cuc, static_cast<int16_t>(-1884));
        EXPECT_EQ(m.cus, static_cast<int16_t>(-1982));
        EXPECT_EQ(m.e, static_cast<uint32_t>(0x30202020U));
        EXPECT_EQ(m.sqrt_a, static_cast<uint32_t>(0x34242424U));
        EXPECT_EQ(m.toe, static_cast<uint16_t>(0x3828U));
        EXPECT_EQ(m.cic, static_cast<int16_t>(-2570));
        EXPECT_EQ(m.omega0, static_cast<int32_t>(-63277120));
        EXPECT_EQ(m.cis, static_cast<int16_t>(-2864));
        EXPECT_EQ(m.crc, static_cast<int16_t>(-2962));
        EXPECT_EQ(m.i0, static_cast<int32_t>(-71731648));
        EXPECT_EQ(m.omega, static_cast<int32_t>(-75958912));
        EXPECT_EQ(m.omega_dot, static_cast<int32_t>(-80186176));
        EXPECT_EQ(m.idot, static_cast<int16_t>(-3648));
    }
};

template <>
struct spec<mga_gps_alm>
{
    /// Spec 3.13.6.2: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x02U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = static_cast<uint8_t>(0x26U); // svHealth
        castle::write_le16(&payload[4U], static_cast<uint16_t>(0x1404U)); // e
        payload[6U] = static_cast<uint8_t>(0x3BU); // almWNa
        payload[7U] = static_cast<uint8_t>(0x42U); // toa
        castle::write_le16s(&payload[8U], static_cast<int16_t>(-904)); // deltaI
        castle::write_le16s(&payload[10U], static_cast<int16_t>(-1002)); // omegaDot
        castle::write_le32(&payload[12U], static_cast<uint32_t>(0x1C0C0C0CU)); // sqrtA
        castle::write_le32s(&payload[16U], static_cast<int32_t>(-33686272)); // omega0
        castle::write_le32s(&payload[20U], static_cast<int32_t>(-37913536)); // omega
        castle::write_le32s(&payload[24U], static_cast<int32_t>(-42140800)); // m0
        castle::write_le16s(&payload[28U], static_cast<int16_t>(-1884)); // af0
        castle::write_le16s(&payload[30U], static_cast<int16_t>(-1982)); // af1
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[32U + i] = reserved_fill; }
    }

    static mga_gps_alm make()
    {
        mga_gps_alm m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.sv_health = static_cast<uint8_t>(0x26U);
        m.e = static_cast<uint16_t>(0x1404U);
        m.alm_wn_a = static_cast<uint8_t>(0x3BU);
        m.toa = static_cast<uint8_t>(0x42U);
        m.delta_i = static_cast<int16_t>(-904);
        m.omega_dot = static_cast<int16_t>(-1002);
        m.sqrt_a = static_cast<uint32_t>(0x1C0C0C0CU);
        m.omega0 = static_cast<int32_t>(-33686272);
        m.omega = static_cast<int32_t>(-37913536);
        m.m0 = static_cast<int32_t>(-42140800);
        m.af0 = static_cast<int16_t>(-1884);
        m.af1 = static_cast<int16_t>(-1982);
        return m;
    }

    static void expect(CASTLE_CONST mga_gps_alm& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.sv_health, static_cast<uint8_t>(0x26U));
        EXPECT_EQ(m.e, static_cast<uint16_t>(0x1404U));
        EXPECT_EQ(m.alm_wn_a, static_cast<uint8_t>(0x3BU));
        EXPECT_EQ(m.toa, static_cast<uint8_t>(0x42U));
        EXPECT_EQ(m.delta_i, static_cast<int16_t>(-904));
        EXPECT_EQ(m.omega_dot, static_cast<int16_t>(-1002));
        EXPECT_EQ(m.sqrt_a, static_cast<uint32_t>(0x1C0C0C0CU));
        EXPECT_EQ(m.omega0, static_cast<int32_t>(-33686272));
        EXPECT_EQ(m.omega, static_cast<int32_t>(-37913536));
        EXPECT_EQ(m.m0, static_cast<int32_t>(-42140800));
        EXPECT_EQ(m.af0, static_cast<int16_t>(-1884));
        EXPECT_EQ(m.af1, static_cast<int16_t>(-1982));
    }
};

template <>
struct spec<mga_gps_health>
{
    /// Spec 3.13.6.3: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x04U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        for (castle::size_type i = 0U; i < 32U; ++i) { payload[4U + i] = static_cast<uint8_t>(0x40U + i); } // healthCode
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[36U + i] = reserved_fill; }
    }

    static mga_gps_health make()
    {
        mga_gps_health m{};
        for (castle::size_type i = 0U; i < 32U; ++i) { m.health_code[i] = static_cast<uint8_t>(0x40U + i); }
        return m;
    }

    static void expect(CASTLE_CONST mga_gps_health& m)
    {
        for (castle::size_type i = 0U; i < 32U; ++i) { EXPECT_EQ(m.health_code[i], static_cast<uint8_t>(0x40U + i)); }
    }
};

template <>
struct spec<mga_gps_utc>
{
    /// Spec 3.13.6.4: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x05U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // utcA0
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // utcA1
        payload[12U] = static_cast<uint8_t>(static_cast<int8_t>(-13)); // utcDtLS
        payload[13U] = static_cast<uint8_t>(0x6CU); // utcTot
        payload[14U] = static_cast<uint8_t>(0x73U); // utcWNt
        payload[15U] = static_cast<uint8_t>(0x7AU); // utcWNlsf
        payload[16U] = static_cast<uint8_t>(0x81U); // utcDn
        payload[17U] = static_cast<uint8_t>(static_cast<int8_t>(-18)); // utcDtLSF
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[18U + i] = reserved_fill; }
    }

    static mga_gps_utc make()
    {
        mga_gps_utc m{};
        m.utc_a0 = static_cast<int32_t>(-21004480);
        m.utc_a1 = static_cast<int32_t>(-25231744);
        m.utc_dt_ls = static_cast<int8_t>(-13);
        m.utc_tot = static_cast<uint8_t>(0x6CU);
        m.utc_wn_t = static_cast<uint8_t>(0x73U);
        m.utc_wn_lsf = static_cast<uint8_t>(0x7AU);
        m.utc_dn = static_cast<uint8_t>(0x81U);
        m.utc_dt_lsf = static_cast<int8_t>(-18);
        return m;
    }

    static void expect(CASTLE_CONST mga_gps_utc& m)
    {
        EXPECT_EQ(m.utc_a0, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.utc_a1, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.utc_dt_ls, static_cast<int8_t>(-13));
        EXPECT_EQ(m.utc_tot, static_cast<uint8_t>(0x6CU));
        EXPECT_EQ(m.utc_wn_t, static_cast<uint8_t>(0x73U));
        EXPECT_EQ(m.utc_wn_lsf, static_cast<uint8_t>(0x7AU));
        EXPECT_EQ(m.utc_dn, static_cast<uint8_t>(0x81U));
        EXPECT_EQ(m.utc_dt_lsf, static_cast<int8_t>(-18));
    }
};

template <>
struct spec<mga_gps_iono>
{
    /// Spec 3.13.6.5: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x06U; // type
        payload[1U] = 0x00U; // version
        for (castle::size_type i = 0U; i < 2U; ++i) { payload[2U + i] = reserved_fill; }
        payload[4U] = static_cast<uint8_t>(static_cast<int8_t>(-5)); // ionoAlpha0
        payload[5U] = static_cast<uint8_t>(static_cast<int8_t>(-6)); // ionoAlpha1
        payload[6U] = static_cast<uint8_t>(static_cast<int8_t>(-7)); // ionoAlpha2
        payload[7U] = static_cast<uint8_t>(static_cast<int8_t>(-8)); // ionoAlpha3
        payload[8U] = static_cast<uint8_t>(static_cast<int8_t>(-9)); // ionoBeta0
        payload[9U] = static_cast<uint8_t>(static_cast<int8_t>(-10)); // ionoBeta1
        payload[10U] = static_cast<uint8_t>(static_cast<int8_t>(-11)); // ionoBeta2
        payload[11U] = static_cast<uint8_t>(static_cast<int8_t>(-12)); // ionoBeta3
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[12U + i] = reserved_fill; }
    }

    static mga_gps_iono make()
    {
        mga_gps_iono m{};
        m.iono_alpha[0U] = static_cast<int8_t>(-5);
        m.iono_alpha[1U] = static_cast<int8_t>(-6);
        m.iono_alpha[2U] = static_cast<int8_t>(-7);
        m.iono_alpha[3U] = static_cast<int8_t>(-8);
        m.iono_beta[0U] = static_cast<int8_t>(-9);
        m.iono_beta[1U] = static_cast<int8_t>(-10);
        m.iono_beta[2U] = static_cast<int8_t>(-11);
        m.iono_beta[3U] = static_cast<int8_t>(-12);
        return m;
    }

    static void expect(CASTLE_CONST mga_gps_iono& m)
    {
        EXPECT_EQ(m.iono_alpha[0U], static_cast<int8_t>(-5));
        EXPECT_EQ(m.iono_alpha[1U], static_cast<int8_t>(-6));
        EXPECT_EQ(m.iono_alpha[2U], static_cast<int8_t>(-7));
        EXPECT_EQ(m.iono_alpha[3U], static_cast<int8_t>(-8));
        EXPECT_EQ(m.iono_beta[0U], static_cast<int8_t>(-9));
        EXPECT_EQ(m.iono_beta[1U], static_cast<int8_t>(-10));
        EXPECT_EQ(m.iono_beta[2U], static_cast<int8_t>(-11));
        EXPECT_EQ(m.iono_beta[3U], static_cast<int8_t>(-12));
    }
};

TEST(UbxMessageMgaAck, MessageSpecOffsets)
{
    check_spec_offsets<mga_ack>();
}

TEST(UbxMessageMgaAck, MessageRoundTrip)
{
    check_round_trip<mga_ack>();
}

TEST(UbxMessageMgaAck, MessageRejects)
{
    check_rejects_common<mga_ack>(1U);
}

TEST(UbxMessageMgaAck, MessageTooSmallBuffer)
{
    check_too_small<mga_ack>();
}

TEST(UbxMessageMgaAck, MessageDispatchExclusive)
{
    check_exclusive<mga_ack, mga_ack, mga_gps_eph, mga_gps_alm, mga_gps_health, mga_gps_utc, mga_gps_iono>();
}

TEST(UbxMessageMgaAck, ConstantsMatchSpec)
{
    EXPECT_EQ(mga_ack::msg_class, UBX_CLASS_MGA);
    EXPECT_EQ(mga_ack::msg_id, UBX_ID_MGA_ACK);
    EXPECT_EQ(mga_ack::payload_length, 8U);
    EXPECT_EQ(static_cast<uint8_t>(mga_ack_type::not_used), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mga_ack_type::accepted), 1U);
}

TEST(UbxMessageMgaAck, AcceptedHelper)
{
    mga_ack a{};
    EXPECT_FALSE(a.accepted());
    a.type = mga_ack_type::accepted;
    EXPECT_TRUE(a.accepted());
}

TEST(UbxMessageMgaAck, EveryInfoCodeDecodes)
{
    CASTLE_CONST mga_ack_info_code codes[] = {
        mga_ack_info_code::accepted, mga_ack_info_code::no_time, mga_ack_info_code::version_not_supported,
        mga_ack_info_code::size_mismatch, mga_ack_info_code::store_failed, mga_ack_info_code::not_ready,
        mga_ack_info_code::unknown_type};
    uint8_t expected_value = 0U;
    for (CASTLE_CONST mga_ack_info_code code : codes)
    {
        uint8_t payload[mga_ack::payload_length] = {};
        payload[0U] = 0x00U; // not used
        payload[2U] = expected_value;
        payload[3U] = UBX_ID_MGA_GPS;
        mga_ack out{};
        ASSERT_TRUE(mga_ack::decode(raw_of<mga_ack>(payload, sizeof(payload)), out));
        EXPECT_EQ(out.info_code, code);
        EXPECT_EQ(static_cast<uint8_t>(code), expected_value);
        EXPECT_FALSE(out.accepted());
        EXPECT_EQ(out.acked_msg_id, UBX_ID_MGA_GPS);
        ++expected_value;
    }
}

TEST(UbxMessageMgaAck, DbdEntryCountIsLittleEndianU4)
{
    uint8_t payload[mga_ack::payload_length] = {};
    payload[0U] = 0x01U;
    payload[3U] = UBX_ID_MGA_DBD;
    castle::write_le32(&payload[4U], 0x00012345U);
    mga_ack out{};
    ASSERT_TRUE(mga_ack::decode(raw_of<mga_ack>(payload, sizeof(payload)), out));
    EXPECT_EQ(out.dbd_entry_count(), 0x00012345U);
}

} // namespace
