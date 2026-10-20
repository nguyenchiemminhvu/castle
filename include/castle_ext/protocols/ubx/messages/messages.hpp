// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file messages.hpp
 * @brief Umbrella header for every UBX message model shipped with Castle.
 *
 * Including this header makes all typed UBX message models available. For a
 * smaller build, include only the message headers you need.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MESSAGES_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MESSAGES_HPP

// ACK (0x05)
#include "castle_ext/protocols/ubx/messages/ack.hpp"

// CFG (0x06)
#include "castle_ext/protocols/ubx/messages/cfg_rst.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_spt.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valdel.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"

// ESF (0x10)
#include "castle_ext/protocols/ubx/messages/esf_alg.hpp"
#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"
#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"
#include "castle_ext/protocols/ubx/messages/esf_raw.hpp"
#include "castle_ext/protocols/ubx/messages/esf_status.hpp"

// INF (0x04)
#include "castle_ext/protocols/ubx/messages/inf.hpp"

// MGA (0x13)
#include "castle_ext/protocols/ubx/messages/mga_ack.hpp"
#include "castle_ext/protocols/ubx/messages/mga_bds.hpp"
#include "castle_ext/protocols/ubx/messages/mga_dbd.hpp"
#include "castle_ext/protocols/ubx/messages/mga_gal.hpp"
#include "castle_ext/protocols/ubx/messages/mga_glo.hpp"
#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"
#include "castle_ext/protocols/ubx/messages/mga_ini.hpp"
#include "castle_ext/protocols/ubx/messages/mga_qzss.hpp"

// MON (0x0A)
#include "castle_ext/protocols/ubx/messages/mon_comms.hpp"
#include "castle_ext/protocols/ubx/messages/mon_gnss.hpp"
#include "castle_ext/protocols/ubx/messages/mon_hw.hpp"
#include "castle_ext/protocols/ubx/messages/mon_hw2.hpp"
#include "castle_ext/protocols/ubx/messages/mon_hw3.hpp"
#include "castle_ext/protocols/ubx/messages/mon_io.hpp"
#include "castle_ext/protocols/ubx/messages/mon_msgpp.hpp"
#include "castle_ext/protocols/ubx/messages/mon_patch.hpp"
#include "castle_ext/protocols/ubx/messages/mon_rf.hpp"
#include "castle_ext/protocols/ubx/messages/mon_rxbuf.hpp"
#include "castle_ext/protocols/ubx/messages/mon_rxr.hpp"
#include "castle_ext/protocols/ubx/messages/mon_span.hpp"
#include "castle_ext/protocols/ubx/messages/mon_spt.hpp"
#include "castle_ext/protocols/ubx/messages/mon_txbuf.hpp"
#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

// NAV (0x01)
#include "castle_ext/protocols/ubx/messages/nav_att.hpp"
#include "castle_ext/protocols/ubx/messages/nav_clock.hpp"
#include "castle_ext/protocols/ubx/messages/nav_cov.hpp"
#include "castle_ext/protocols/ubx/messages/nav_dop.hpp"
#include "castle_ext/protocols/ubx/messages/nav_eell.hpp"
#include "castle_ext/protocols/ubx/messages/nav_eoe.hpp"
#include "castle_ext/protocols/ubx/messages/nav_geofence.hpp"
#include "castle_ext/protocols/ubx/messages/nav_hpposecef.hpp"
#include "castle_ext/protocols/ubx/messages/nav_hpposllh.hpp"
#include "castle_ext/protocols/ubx/messages/nav_odo.hpp"
#include "castle_ext/protocols/ubx/messages/nav_orb.hpp"
#include "castle_ext/protocols/ubx/messages/nav_posecef.hpp"
#include "castle_ext/protocols/ubx/messages/nav_posllh.hpp"
#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"
#include "castle_ext/protocols/ubx/messages/nav_relposned.hpp"
#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"
#include "castle_ext/protocols/ubx/messages/nav_sbas.hpp"
#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"
#include "castle_ext/protocols/ubx/messages/nav_slas.hpp"
#include "castle_ext/protocols/ubx/messages/nav_status.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timebds.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timegal.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeglo.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timegps.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timels.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeqzss.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"
#include "castle_ext/protocols/ubx/messages/nav_velecef.hpp"
#include "castle_ext/protocols/ubx/messages/nav_velned.hpp"

// NAV2 (0x29)
#include "castle_ext/protocols/ubx/messages/nav2_dop.hpp"
#include "castle_ext/protocols/ubx/messages/nav2_eell.hpp"
#include "castle_ext/protocols/ubx/messages/nav2_pvt.hpp"
#include "castle_ext/protocols/ubx/messages/nav2_timegps.hpp"

// RXM (0x02)
#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"
#include "castle_ext/protocols/ubx/messages/rxm_pmreq.hpp"
#include "castle_ext/protocols/ubx/messages/rxm_rawx.hpp"
#include "castle_ext/protocols/ubx/messages/rxm_rlm.hpp"
#include "castle_ext/protocols/ubx/messages/rxm_rtcm.hpp"

// SEC (0x27)
#include "castle_ext/protocols/ubx/messages/sec_crc.hpp"
#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"
#include "castle_ext/protocols/ubx/messages/sec_uniqid.hpp"

// TIM (0x0D)
#include "castle_ext/protocols/ubx/messages/tim_tm2.hpp"
#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"
#include "castle_ext/protocols/ubx/messages/tim_vrfy.hpp"

// UPD (0x09)
#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MESSAGES_HPP
