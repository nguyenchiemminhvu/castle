#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/messages.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

struct identity
{
    uint8_t msg_class;
    uint8_t msg_id;
    const char* name;
};

template <typename Message>
CASTLE_CONSTEXPR identity id_of(const char* name)
{
    return identity{Message::msg_class, Message::msg_id, name};
}

/**
 * Every class/ID pair that carries a single layout. Models sharing a class/ID
 * by design (NAV vs NAV2 are different classes; typed layouts such as MGA,
 * UPD-SOS, RXM-RLM and the CFG version pairs) are checked separately below.
 */
CASTLE_CONSTEXPR identity single_layout_ids[] = {
    id_of<ack_ack>("ack_ack"),
    id_of<ack_nak>("ack_nak"),
    id_of<cfg_rst>("cfg_rst"),
    id_of<cfg_spt>("cfg_spt"),
    id_of<esf_alg>("esf_alg"),
    id_of<esf_ins>("esf_ins"),
    id_of<esf_meas<>>("esf_meas"),
    id_of<esf_raw<>>("esf_raw"),
    id_of<esf_status<>>("esf_status"),
    id_of<mga_ack>("mga_ack"),
    id_of<mga_dbd<>>("mga_dbd"),
    id_of<mon_comms<>>("mon_comms"),
    id_of<mon_gnss>("mon_gnss"),
    id_of<mon_hw>("mon_hw"),
    id_of<mon_hw2>("mon_hw2"),
    id_of<mon_hw3<>>("mon_hw3"),
    id_of<mon_io<>>("mon_io"),
    id_of<mon_msgpp>("mon_msgpp"),
    id_of<mon_patch<>>("mon_patch"),
    id_of<mon_rf<>>("mon_rf"),
    id_of<mon_rxbuf>("mon_rxbuf"),
    id_of<mon_rxr>("mon_rxr"),
    id_of<mon_span<>>("mon_span"),
    id_of<mon_spt<>>("mon_spt"),
    id_of<mon_txbuf>("mon_txbuf"),
    id_of<mon_ver<>>("mon_ver"),
    id_of<nav2_dop>("nav2_dop"),
    id_of<nav2_eell>("nav2_eell"),
    id_of<nav2_pvt>("nav2_pvt"),
    id_of<nav2_timegps>("nav2_timegps"),
    id_of<nav_att>("nav_att"),
    id_of<nav_clock>("nav_clock"),
    id_of<nav_cov>("nav_cov"),
    id_of<nav_dop>("nav_dop"),
    id_of<nav_eell>("nav_eell"),
    id_of<nav_eoe>("nav_eoe"),
    id_of<nav_geofence<>>("nav_geofence"),
    id_of<nav_hpposecef>("nav_hpposecef"),
    id_of<nav_hpposllh>("nav_hpposllh"),
    id_of<nav_odo>("nav_odo"),
    id_of<nav_orb<>>("nav_orb"),
    id_of<nav_posecef>("nav_posecef"),
    id_of<nav_posllh>("nav_posllh"),
    id_of<nav_pvt>("nav_pvt"),
    id_of<nav_relposned>("nav_relposned"),
    id_of<nav_sat<>>("nav_sat"),
    id_of<nav_sbas<>>("nav_sbas"),
    id_of<nav_sig<>>("nav_sig"),
    id_of<nav_slas<>>("nav_slas"),
    id_of<nav_status>("nav_status"),
    id_of<nav_timebds>("nav_timebds"),
    id_of<nav_timegal>("nav_timegal"),
    id_of<nav_timeglo>("nav_timeglo"),
    id_of<nav_timegps>("nav_timegps"),
    id_of<nav_timels>("nav_timels"),
    id_of<nav_timeqzss>("nav_timeqzss"),
    id_of<nav_timeutc>("nav_timeutc"),
    id_of<nav_velecef>("nav_velecef"),
    id_of<nav_velned>("nav_velned"),
    id_of<rxm_measx<>>("rxm_measx"),
    id_of<rxm_rawx<>>("rxm_rawx"),
    id_of<rxm_rtcm>("rxm_rtcm"),
    id_of<sec_crc>("sec_crc"),
    id_of<sec_sig>("sec_sig"),
    id_of<sec_uniqid>("sec_uniqid"),
    id_of<tim_tm2>("tim_tm2"),
    id_of<tim_tp>("tim_tp"),
    id_of<tim_vrfy>("tim_vrfy"),
};

TEST(UbxMessages, SingleLayoutClassIdPairsAreUnique)
{
    CASTLE_CONST castle::size_type count = sizeof(single_layout_ids) / sizeof(single_layout_ids[0U]);
    for (castle::size_type i = 0U; i < count; ++i)
    {
        for (castle::size_type j = i + 1U; j < count; ++j)
        {
            CASTLE_CONST bool same = (single_layout_ids[i].msg_class == single_layout_ids[j].msg_class)
                && (single_layout_ids[i].msg_id == single_layout_ids[j].msg_id);
            EXPECT_FALSE(same) << single_layout_ids[i].name << " and " << single_layout_ids[j].name
                               << " share class/ID";
        }
    }
}

TEST(UbxMessages, IdentitiesMatchSpecHeaderBytes)
{
    // Spot-check one model per class against the interface description.
    EXPECT_EQ(nav_pvt::msg_class, 0x01U);
    EXPECT_EQ(nav_pvt::msg_id, 0x07U);
    EXPECT_EQ(rxm_rawx<>::msg_class, 0x02U);
    EXPECT_EQ(rxm_rawx<>::msg_id, 0x15U);
    EXPECT_EQ(inf_notice<>::msg_class, 0x04U);
    EXPECT_EQ(inf_notice<>::msg_id, 0x02U);
    EXPECT_EQ(ack_ack::msg_class, 0x05U);
    EXPECT_EQ(cfg_valdel<>::msg_id, 0x8CU);
    EXPECT_EQ(upd_sos_ack::msg_class, 0x09U);
    EXPECT_EQ(mon_comms<>::msg_class, 0x0AU);
    EXPECT_EQ(mon_comms<>::msg_id, 0x36U);
    EXPECT_EQ(tim_vrfy::msg_class, 0x0DU);
    EXPECT_EQ(esf_alg::msg_class, 0x10U);
    EXPECT_EQ(mga_ini_pos_llh::msg_class, 0x13U);
    EXPECT_EQ(mga_ini_pos_llh::msg_id, 0x40U);
    EXPECT_EQ(sec_uniqid::msg_class, 0x27U);
    EXPECT_EQ(nav2_pvt::msg_class, 0x29U);
}

TEST(UbxMessages, SharedClassIdLayoutsAreTellApartByTypeOrVersion)
{
    EXPECT_EQ(mga_gps_eph::msg_id, mga_gps_utc::msg_id);
    EXPECT_NE(mga_gps_eph::message_type, mga_gps_utc::message_type);
    EXPECT_NE(mga_gps_eph::msg_id, mga_qzss_eph::msg_id); // same layout template, different ID

    EXPECT_EQ(upd_sos_ack::msg_id, upd_sos_restored::msg_id);
    EXPECT_NE(upd_sos_ack::message_type, upd_sos_restored::message_type);

    EXPECT_EQ(rxm_rlm_short::msg_id, rxm_rlm_long::msg_id);
    EXPECT_NE(rxm_rlm_short::payload_length, rxm_rlm_long::payload_length);

    EXPECT_EQ(cfg_valget_poll<>::msg_id, cfg_valget<>::msg_id);
    EXPECT_NE(cfg_valget_poll<>::version, cfg_valget<>::version);
    EXPECT_NE(cfg_valset<>::version, cfg_valset_transaction<>::version);
    EXPECT_NE(cfg_valdel<>::version, cfg_valdel_transaction<>::version);

    EXPECT_EQ(rxm_pmreq_v0::msg_id, rxm_pmreq_v1::msg_id);
    EXPECT_NE(rxm_pmreq_v0::payload_length, rxm_pmreq_v1::payload_length);
}

TEST(UbxMessages, InfSeverityAliasesMatchOnlyTheirId)
{
    uint8_t text[2U] = {'h', 'i'};
    message_view raw{{UBX_CLASS_INF, UBX_ID_INF_WARNING, 2U},
                     castle::container::array_view<CASTLE_CONST uint8_t>(text, 2U),
                     checksum{}};
    EXPECT_TRUE(inf<>::matches(raw));
    EXPECT_TRUE(inf_warning<>::matches(raw));
    EXPECT_FALSE(inf_error<>::matches(raw));
    EXPECT_FALSE(inf_notice<>::matches(raw));
    EXPECT_FALSE(inf_test<>::matches(raw));
    EXPECT_FALSE(inf_debug<>::matches(raw));
}

} // namespace
