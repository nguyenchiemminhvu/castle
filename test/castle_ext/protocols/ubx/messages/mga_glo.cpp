#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mga_glo.hpp"

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
struct spec<mga_glo_eph>
{
    /// Spec 3.13.5.1: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x01U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        payload[4U] = static_cast<uint8_t>(0x2DU); // FT
        payload[5U] = static_cast<uint8_t>(0x34U); // B
        payload[6U] = static_cast<uint8_t>(0x3BU); // M
        payload[7U] = static_cast<uint8_t>(static_cast<int8_t>(-8)); // H
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // x
        castle::write_le32s(&payload[12U], static_cast<int32_t>(-29459008)); // y
        castle::write_le32s(&payload[16U], static_cast<int32_t>(-33686272)); // z
        castle::write_le32s(&payload[20U], static_cast<int32_t>(-37913536)); // dx
        castle::write_le32s(&payload[24U], static_cast<int32_t>(-42140800)); // dy
        castle::write_le32s(&payload[28U], static_cast<int32_t>(-46368064)); // dz
        payload[32U] = static_cast<uint8_t>(static_cast<int8_t>(-33)); // ddx
        payload[33U] = static_cast<uint8_t>(static_cast<int8_t>(-34)); // ddy
        payload[34U] = static_cast<uint8_t>(static_cast<int8_t>(-35)); // ddz
        payload[35U] = static_cast<uint8_t>(0x06U); // tb
        castle::write_le16s(&payload[36U], static_cast<int16_t>(-2276)); // gamma
        payload[38U] = static_cast<uint8_t>(0x1BU); // E
        payload[39U] = static_cast<uint8_t>(static_cast<int8_t>(-40)); // deltaTau
        castle::write_le32s(&payload[40U], static_cast<int32_t>(-59049856)); // tau
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[44U + i] = reserved_fill; }
    }

    static mga_glo_eph make()
    {
        mga_glo_eph m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.ft = static_cast<uint8_t>(0x2DU);
        m.b = static_cast<uint8_t>(0x34U);
        m.m = static_cast<uint8_t>(0x3BU);
        m.h = static_cast<int8_t>(-8);
        m.x = static_cast<int32_t>(-25231744);
        m.y = static_cast<int32_t>(-29459008);
        m.z = static_cast<int32_t>(-33686272);
        m.dx = static_cast<int32_t>(-37913536);
        m.dy = static_cast<int32_t>(-42140800);
        m.dz = static_cast<int32_t>(-46368064);
        m.ddx = static_cast<int8_t>(-33);
        m.ddy = static_cast<int8_t>(-34);
        m.ddz = static_cast<int8_t>(-35);
        m.tb = static_cast<uint8_t>(0x06U);
        m.gamma = static_cast<int16_t>(-2276);
        m.e = static_cast<uint8_t>(0x1BU);
        m.delta_tau = static_cast<int8_t>(-40);
        m.tau = static_cast<int32_t>(-59049856);
        return m;
    }

    static void expect(CASTLE_CONST mga_glo_eph& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.ft, static_cast<uint8_t>(0x2DU));
        EXPECT_EQ(m.b, static_cast<uint8_t>(0x34U));
        EXPECT_EQ(m.m, static_cast<uint8_t>(0x3BU));
        EXPECT_EQ(m.h, static_cast<int8_t>(-8));
        EXPECT_EQ(m.x, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.y, static_cast<int32_t>(-29459008));
        EXPECT_EQ(m.z, static_cast<int32_t>(-33686272));
        EXPECT_EQ(m.dx, static_cast<int32_t>(-37913536));
        EXPECT_EQ(m.dy, static_cast<int32_t>(-42140800));
        EXPECT_EQ(m.dz, static_cast<int32_t>(-46368064));
        EXPECT_EQ(m.ddx, static_cast<int8_t>(-33));
        EXPECT_EQ(m.ddy, static_cast<int8_t>(-34));
        EXPECT_EQ(m.ddz, static_cast<int8_t>(-35));
        EXPECT_EQ(m.tb, static_cast<uint8_t>(0x06U));
        EXPECT_EQ(m.gamma, static_cast<int16_t>(-2276));
        EXPECT_EQ(m.e, static_cast<uint8_t>(0x1BU));
        EXPECT_EQ(m.delta_tau, static_cast<int8_t>(-40));
        EXPECT_EQ(m.tau, static_cast<int32_t>(-59049856));
    }
};

template <>
struct spec<mga_glo_alm>
{
    /// Spec 3.13.5.2: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x02U; // type
        payload[1U] = 0x00U; // version
        payload[2U] = static_cast<uint8_t>(0x1FU); // svId
        payload[3U] = reserved_fill;
        castle::write_le16(&payload[4U], static_cast<uint16_t>(0x1404U)); // N
        payload[6U] = static_cast<uint8_t>(0x3BU); // M
        payload[7U] = static_cast<uint8_t>(0x42U); // C
        castle::write_le16s(&payload[8U], static_cast<int16_t>(-904)); // tau
        castle::write_le16(&payload[10U], static_cast<uint16_t>(0x1A0AU)); // epsilon
        castle::write_le32s(&payload[12U], static_cast<int32_t>(-29459008)); // lambda
        castle::write_le32s(&payload[16U], static_cast<int32_t>(-33686272)); // deltaI
        castle::write_le32(&payload[20U], static_cast<uint32_t>(0x24141414U)); // tLambda
        castle::write_le32s(&payload[24U], static_cast<int32_t>(-42140800)); // deltaT
        payload[28U] = static_cast<uint8_t>(static_cast<int8_t>(-29)); // deltaDT
        payload[29U] = static_cast<uint8_t>(static_cast<int8_t>(-30)); // H
        castle::write_le16s(&payload[30U], static_cast<int16_t>(-1982)); // omega
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[32U + i] = reserved_fill; }
    }

    static mga_glo_alm make()
    {
        mga_glo_alm m{};
        m.sv_id = static_cast<uint8_t>(0x1FU);
        m.n = static_cast<uint16_t>(0x1404U);
        m.m = static_cast<uint8_t>(0x3BU);
        m.c = static_cast<uint8_t>(0x42U);
        m.tau = static_cast<int16_t>(-904);
        m.epsilon = static_cast<uint16_t>(0x1A0AU);
        m.lambda = static_cast<int32_t>(-29459008);
        m.delta_i = static_cast<int32_t>(-33686272);
        m.t_lambda = static_cast<uint32_t>(0x24141414U);
        m.delta_t = static_cast<int32_t>(-42140800);
        m.delta_dt = static_cast<int8_t>(-29);
        m.h = static_cast<int8_t>(-30);
        m.omega = static_cast<int16_t>(-1982);
        return m;
    }

    static void expect(CASTLE_CONST mga_glo_alm& m)
    {
        EXPECT_EQ(m.sv_id, static_cast<uint8_t>(0x1FU));
        EXPECT_EQ(m.n, static_cast<uint16_t>(0x1404U));
        EXPECT_EQ(m.m, static_cast<uint8_t>(0x3BU));
        EXPECT_EQ(m.c, static_cast<uint8_t>(0x42U));
        EXPECT_EQ(m.tau, static_cast<int16_t>(-904));
        EXPECT_EQ(m.epsilon, static_cast<uint16_t>(0x1A0AU));
        EXPECT_EQ(m.lambda, static_cast<int32_t>(-29459008));
        EXPECT_EQ(m.delta_i, static_cast<int32_t>(-33686272));
        EXPECT_EQ(m.t_lambda, static_cast<uint32_t>(0x24141414U));
        EXPECT_EQ(m.delta_t, static_cast<int32_t>(-42140800));
        EXPECT_EQ(m.delta_dt, static_cast<int8_t>(-29));
        EXPECT_EQ(m.h, static_cast<int8_t>(-30));
        EXPECT_EQ(m.omega, static_cast<int16_t>(-1982));
    }
};

template <>
struct spec<mga_glo_timeoffset>
{
    /// Spec 3.13.5.3: every field written at its documented byte offset.
    static void fill(uint8_t* payload, uint8_t reserved_fill)
    {
        static_cast<void>(reserved_fill);
        payload[0U] = 0x03U; // type
        payload[1U] = 0x00U; // version
        castle::write_le16(&payload[2U], static_cast<uint16_t>(0x1202U)); // N
        castle::write_le32s(&payload[4U], static_cast<int32_t>(-21004480)); // tauC
        castle::write_le32s(&payload[8U], static_cast<int32_t>(-25231744)); // tauGps
        castle::write_le16s(&payload[12U], static_cast<int16_t>(-1100)); // B1
        castle::write_le16s(&payload[14U], static_cast<int16_t>(-1198)); // B2
        for (castle::size_type i = 0U; i < 4U; ++i) { payload[16U + i] = reserved_fill; }
    }

    static mga_glo_timeoffset make()
    {
        mga_glo_timeoffset m{};
        m.n = static_cast<uint16_t>(0x1202U);
        m.tau_c = static_cast<int32_t>(-21004480);
        m.tau_gps = static_cast<int32_t>(-25231744);
        m.b1 = static_cast<int16_t>(-1100);
        m.b2 = static_cast<int16_t>(-1198);
        return m;
    }

    static void expect(CASTLE_CONST mga_glo_timeoffset& m)
    {
        EXPECT_EQ(m.n, static_cast<uint16_t>(0x1202U));
        EXPECT_EQ(m.tau_c, static_cast<int32_t>(-21004480));
        EXPECT_EQ(m.tau_gps, static_cast<int32_t>(-25231744));
        EXPECT_EQ(m.b1, static_cast<int16_t>(-1100));
        EXPECT_EQ(m.b2, static_cast<int16_t>(-1198));
    }
};

TEST(UbxMessageMgaGlo, EphSpecOffsets)
{
    check_spec_offsets<mga_glo_eph>();
}

TEST(UbxMessageMgaGlo, EphRoundTrip)
{
    check_round_trip<mga_glo_eph>();
}

TEST(UbxMessageMgaGlo, EphRejects)
{
    check_rejects_typed<mga_glo_eph>();
}

TEST(UbxMessageMgaGlo, EphTooSmallBuffer)
{
    check_too_small<mga_glo_eph>();
}

TEST(UbxMessageMgaGlo, EphDispatchExclusive)
{
    check_exclusive<mga_glo_eph, mga_glo_eph, mga_glo_alm, mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, AlmSpecOffsets)
{
    check_spec_offsets<mga_glo_alm>();
}

TEST(UbxMessageMgaGlo, AlmRoundTrip)
{
    check_round_trip<mga_glo_alm>();
}

TEST(UbxMessageMgaGlo, AlmRejects)
{
    check_rejects_typed<mga_glo_alm>();
}

TEST(UbxMessageMgaGlo, AlmTooSmallBuffer)
{
    check_too_small<mga_glo_alm>();
}

TEST(UbxMessageMgaGlo, AlmDispatchExclusive)
{
    check_exclusive<mga_glo_alm, mga_glo_eph, mga_glo_alm, mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, TimeoffsetSpecOffsets)
{
    check_spec_offsets<mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, TimeoffsetRoundTrip)
{
    check_round_trip<mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, TimeoffsetRejects)
{
    check_rejects_typed<mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, TimeoffsetTooSmallBuffer)
{
    check_too_small<mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, TimeoffsetDispatchExclusive)
{
    check_exclusive<mga_glo_timeoffset, mga_glo_eph, mga_glo_alm, mga_glo_timeoffset>();
}

TEST(UbxMessageMgaGlo, ConstantsMatchSpec)
{
    EXPECT_EQ(mga_glo_eph::msg_id, UBX_ID_MGA_GLO);
    EXPECT_EQ(MGA_GLO_TYPE_EPH, 0x01U);
    EXPECT_EQ(MGA_GLO_TYPE_ALM, 0x02U);
    EXPECT_EQ(MGA_GLO_TYPE_TIMEOFFSET, 0x03U);
}

TEST(UbxMessageMgaGlo, UnknownFrequencyChannelRoundTrips)
{
    mga_glo_eph in = spec<mga_glo_eph>::make();
    in.h = MGA_GLO_FREQ_CHANNEL_UNKNOWN;
    uint8_t frame[mga_glo_eph::payload_length + UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 7U], 0x80U);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    mga_glo_eph out{};
    ASSERT_TRUE(mga_glo_eph::decode(raw, out));
    EXPECT_EQ(out.h, -128);
}

} // namespace
