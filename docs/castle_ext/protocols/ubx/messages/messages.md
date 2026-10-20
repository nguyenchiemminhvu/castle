# UBX Message Umbrella

## Overview
`messages.hpp` includes every UBX message model shipped with Castle: 80 headers covering the ACK, CFG, ESF, INF, MGA, MON, NAV, NAV2, RXM, SEC, TIM and UPD classes of the u-blox ZED-F9R interface description (UBX-19056845, protocol 33.20). Include it for convenience, or include single message headers for a smaller build.

Every model follows the same shape:

- `static bool matches(const message_view&)` recognises the frame (class/ID, plus the exact length and type or version byte where one class/ID carries several layouts);
- `static bool decode(const message_view&, Model&)` validates the payload length exactly and fills a value-owning model; on failure the output is value-initialised;
- `void encode(payload_writer&) const` is used by `encode_message()` for host-to-receiver messages and for loop-back tests;
- the wire layout is written **once** (`layout()`), so decoding and encoding cannot drift. See [`detail/fixed_message`](detail/fixed_message.md), [`detail/typed_message`](detail/typed_message.md) and [`detail/repeated_group`](detail/repeated_group.md).

Poll requests with an empty payload need no model: `encode_poll<Model>(buffer, capacity, written)`.

## Header
`#include "castle_ext/protocols/ubx/messages/messages.hpp"`

## Dependencies
- [`castle_ext/protocols/ubx/messages/ack.hpp`](ack.md)
- [`castle_ext/protocols/ubx/messages/cfg_rst.hpp`](cfg_rst.md)
- [`castle_ext/protocols/ubx/messages/cfg_spt.hpp`](cfg_spt.md)
- [`castle_ext/protocols/ubx/messages/cfg_valdel.hpp`](cfg_valdel.md)
- [`castle_ext/protocols/ubx/messages/cfg_valget.hpp`](cfg_valget.md)
- [`castle_ext/protocols/ubx/messages/cfg_valset.hpp`](cfg_valset.md)
- [`castle_ext/protocols/ubx/messages/esf_alg.hpp`](esf_alg.md)
- [`castle_ext/protocols/ubx/messages/esf_ins.hpp`](esf_ins.md)
- [`castle_ext/protocols/ubx/messages/esf_meas.hpp`](esf_meas.md)
- [`castle_ext/protocols/ubx/messages/esf_raw.hpp`](esf_raw.md)
- [`castle_ext/protocols/ubx/messages/esf_status.hpp`](esf_status.md)
- [`castle_ext/protocols/ubx/messages/inf.hpp`](inf.md)
- [`castle_ext/protocols/ubx/messages/mga_ack.hpp`](mga_ack.md)
- [`castle_ext/protocols/ubx/messages/mga_bds.hpp`](mga_bds.md)
- [`castle_ext/protocols/ubx/messages/mga_dbd.hpp`](mga_dbd.md)
- [`castle_ext/protocols/ubx/messages/mga_gal.hpp`](mga_gal.md)
- [`castle_ext/protocols/ubx/messages/mga_glo.hpp`](mga_glo.md)
- [`castle_ext/protocols/ubx/messages/mga_gps.hpp`](mga_gps.md)
- [`castle_ext/protocols/ubx/messages/mga_ini.hpp`](mga_ini.md)
- [`castle_ext/protocols/ubx/messages/mga_qzss.hpp`](mga_qzss.md)
- [`castle_ext/protocols/ubx/messages/mon_comms.hpp`](mon_comms.md)
- [`castle_ext/protocols/ubx/messages/mon_gnss.hpp`](mon_gnss.md)
- [`castle_ext/protocols/ubx/messages/mon_hw.hpp`](mon_hw.md)
- [`castle_ext/protocols/ubx/messages/mon_hw2.hpp`](mon_hw2.md)
- [`castle_ext/protocols/ubx/messages/mon_hw3.hpp`](mon_hw3.md)
- [`castle_ext/protocols/ubx/messages/mon_io.hpp`](mon_io.md)
- [`castle_ext/protocols/ubx/messages/mon_msgpp.hpp`](mon_msgpp.md)
- [`castle_ext/protocols/ubx/messages/mon_patch.hpp`](mon_patch.md)
- [`castle_ext/protocols/ubx/messages/mon_rf.hpp`](mon_rf.md)
- [`castle_ext/protocols/ubx/messages/mon_rxbuf.hpp`](mon_rxbuf.md)
- [`castle_ext/protocols/ubx/messages/mon_rxr.hpp`](mon_rxr.md)
- [`castle_ext/protocols/ubx/messages/mon_span.hpp`](mon_span.md)
- [`castle_ext/protocols/ubx/messages/mon_spt.hpp`](mon_spt.md)
- [`castle_ext/protocols/ubx/messages/mon_txbuf.hpp`](mon_txbuf.md)
- [`castle_ext/protocols/ubx/messages/mon_ver.hpp`](mon_ver.md)
- [`castle_ext/protocols/ubx/messages/nav2_dop.hpp`](nav2_dop.md)
- [`castle_ext/protocols/ubx/messages/nav2_eell.hpp`](nav2_eell.md)
- [`castle_ext/protocols/ubx/messages/nav2_pvt.hpp`](nav2_pvt.md)
- [`castle_ext/protocols/ubx/messages/nav2_timegps.hpp`](nav2_timegps.md)
- [`castle_ext/protocols/ubx/messages/nav_att.hpp`](nav_att.md)
- [`castle_ext/protocols/ubx/messages/nav_clock.hpp`](nav_clock.md)
- [`castle_ext/protocols/ubx/messages/nav_cov.hpp`](nav_cov.md)
- [`castle_ext/protocols/ubx/messages/nav_dop.hpp`](nav_dop.md)
- [`castle_ext/protocols/ubx/messages/nav_eell.hpp`](nav_eell.md)
- [`castle_ext/protocols/ubx/messages/nav_eoe.hpp`](nav_eoe.md)
- [`castle_ext/protocols/ubx/messages/nav_geofence.hpp`](nav_geofence.md)
- [`castle_ext/protocols/ubx/messages/nav_hpposecef.hpp`](nav_hpposecef.md)
- [`castle_ext/protocols/ubx/messages/nav_hpposllh.hpp`](nav_hpposllh.md)
- [`castle_ext/protocols/ubx/messages/nav_odo.hpp`](nav_odo.md)
- [`castle_ext/protocols/ubx/messages/nav_orb.hpp`](nav_orb.md)
- [`castle_ext/protocols/ubx/messages/nav_posecef.hpp`](nav_posecef.md)
- [`castle_ext/protocols/ubx/messages/nav_posllh.hpp`](nav_posllh.md)
- [`castle_ext/protocols/ubx/messages/nav_pvt.hpp`](nav_pvt.md)
- [`castle_ext/protocols/ubx/messages/nav_relposned.hpp`](nav_relposned.md)
- [`castle_ext/protocols/ubx/messages/nav_sat.hpp`](nav_sat.md)
- [`castle_ext/protocols/ubx/messages/nav_sbas.hpp`](nav_sbas.md)
- [`castle_ext/protocols/ubx/messages/nav_sig.hpp`](nav_sig.md)
- [`castle_ext/protocols/ubx/messages/nav_slas.hpp`](nav_slas.md)
- [`castle_ext/protocols/ubx/messages/nav_status.hpp`](nav_status.md)
- [`castle_ext/protocols/ubx/messages/nav_timebds.hpp`](nav_timebds.md)
- [`castle_ext/protocols/ubx/messages/nav_timegal.hpp`](nav_timegal.md)
- [`castle_ext/protocols/ubx/messages/nav_timeglo.hpp`](nav_timeglo.md)
- [`castle_ext/protocols/ubx/messages/nav_timegps.hpp`](nav_timegps.md)
- [`castle_ext/protocols/ubx/messages/nav_timels.hpp`](nav_timels.md)
- [`castle_ext/protocols/ubx/messages/nav_timeqzss.hpp`](nav_timeqzss.md)
- [`castle_ext/protocols/ubx/messages/nav_timeutc.hpp`](nav_timeutc.md)
- [`castle_ext/protocols/ubx/messages/nav_velecef.hpp`](nav_velecef.md)
- [`castle_ext/protocols/ubx/messages/nav_velned.hpp`](nav_velned.md)
- [`castle_ext/protocols/ubx/messages/rxm_measx.hpp`](rxm_measx.md)
- [`castle_ext/protocols/ubx/messages/rxm_pmreq.hpp`](rxm_pmreq.md)
- [`castle_ext/protocols/ubx/messages/rxm_rawx.hpp`](rxm_rawx.md)
- [`castle_ext/protocols/ubx/messages/rxm_rlm.hpp`](rxm_rlm.md)
- [`castle_ext/protocols/ubx/messages/rxm_rtcm.hpp`](rxm_rtcm.md)
- [`castle_ext/protocols/ubx/messages/sec_crc.hpp`](sec_crc.md)
- [`castle_ext/protocols/ubx/messages/sec_sig.hpp`](sec_sig.md)
- [`castle_ext/protocols/ubx/messages/sec_uniqid.hpp`](sec_uniqid.md)
- [`castle_ext/protocols/ubx/messages/tim_tm2.hpp`](tim_tm2.md)
- [`castle_ext/protocols/ubx/messages/tim_tp.hpp`](tim_tp.md)
- [`castle_ext/protocols/ubx/messages/tim_vrfy.hpp`](tim_vrfy.md)
- [`castle_ext/protocols/ubx/messages/upd_sos.hpp`](upd_sos.md)

## Public API

This header declares nothing itself. It exposes the following models, all in `castle::protocols::ubx::messages`.

### ACK (0x05)

| Header | Message |
| --- | --- |
| [`ack.hpp`](ack.md) | UBX-ACK-ACK (0x05 0x01) / UBX-ACK-NAK (0x05 0x00): input message acknowledgement. |

### CFG (0x06)

| Header | Message |
| --- | --- |
| [`cfg_rst.hpp`](cfg_rst.md) | UBX-CFG-RST (0x06 0x04): reset receiver / clear backup data structures. |
| [`cfg_spt.hpp`](cfg_spt.md) | UBX-CFG-SPT (0x06 0x64): configure and start a sensor production test. |
| [`cfg_valdel.hpp`](cfg_valdel.md) | UBX-CFG-VALDEL (0x06 0x8c): delete configuration item values. |
| [`cfg_valget.hpp`](cfg_valget.md) | UBX-CFG-VALGET (0x06 0x8b): get configuration items. |
| [`cfg_valset.hpp`](cfg_valset.md) | UBX-CFG-VALSET (0x06 0x8a): set configuration item values. |

### ESF (0x10)

| Header | Message |
| --- | --- |
| [`esf_alg.hpp`](esf_alg.md) | UBX-ESF-ALG (0x10 0x14): IMU-mount alignment information. |
| [`esf_ins.hpp`](esf_ins.md) | UBX-ESF-INS (0x10 0x15): vehicle dynamics information. |
| [`esf_meas.hpp`](esf_meas.md) | UBX-ESF-MEAS (0x10 0x02): external sensor fusion measurements. |
| [`esf_raw.hpp`](esf_raw.md) | UBX-ESF-RAW (0x10 0x03): raw sensor measurements. |
| [`esf_status.hpp`](esf_status.md) | UBX-ESF-STATUS (0x10 0x10): external sensor fusion status. |

### INF (0x04)

| Header | Message |
| --- | --- |
| [`inf.hpp`](inf.md) | UBX-INF-ERROR/WARNING/NOTICE/TEST/DEBUG (0x04 0x00..0x04): firmware text output. |

### MGA (0x13)

| Header | Message |
| --- | --- |
| [`mga_ack.hpp`](mga_ack.md) | UBX-MGA-ACK (0x13 0x60): acknowledgement of one assistance message. |
| [`mga_bds.hpp`](mga_bds.md) | UBX-MGA-BDS (0x13 0x03): BeiDou assistance data (ephemeris, almanac, health, UTC, ionosphere). |
| [`mga_dbd.hpp`](mga_dbd.md) | UBX-MGA-DBD (0x13 0x80): navigation database dump entry. |
| [`mga_gal.hpp`](mga_gal.md) | UBX-MGA-GAL (0x13 0x02): Galileo assistance data (ephemeris, almanac, GPS time offset, UTC). |
| [`mga_glo.hpp`](mga_glo.md) | UBX-MGA-GLO (0x13 0x06): GLONASS assistance data (ephemeris, almanac, auxiliary time offsets). |
| [`mga_gps.hpp`](mga_gps.md) | UBX-MGA-GPS (0x13 0x00): GPS assistance data (ephemeris, almanac, health, UTC, ionosphere). |
| [`mga_ini.hpp`](mga_ini.md) | UBX-MGA-INI (0x13 0x40): initial position, time, clock drift and frequency assistance. |
| [`mga_qzss.hpp`](mga_qzss.md) | UBX-MGA-QZSS (0x13 0x05): QZSS assistance data (ephemeris, almanac, health). |

### MON (0x0A)

| Header | Message |
| --- | --- |
| [`mon_comms.hpp`](mon_comms.md) | UBX-MON-COMMS (0x0a 0x36): communication port information. |
| [`mon_gnss.hpp`](mon_gnss.md) | UBX-MON-GNSS (0x0a 0x28): major GNSS selection. |
| [`mon_hw.hpp`](mon_hw.md) | UBX-MON-HW (0x0a 0x09): hardware status. |
| [`mon_hw2.hpp`](mon_hw2.md) | UBX-MON-HW2 (0x0a 0x0b): extended hardware status. |
| [`mon_hw3.hpp`](mon_hw3.md) | UBX-MON-HW3 (0x0a 0x37): I/O pin status. |
| [`mon_io.hpp`](mon_io.md) | UBX-MON-IO (0x0a 0x02): I/O system status. |
| [`mon_msgpp.hpp`](mon_msgpp.md) | UBX-MON-MSGPP (0x0a 0x06): message parse and process status. |
| [`mon_patch.hpp`](mon_patch.md) | UBX-MON-PATCH (0x0a 0x27): installed patches. |
| [`mon_rf.hpp`](mon_rf.md) | UBX-MON-RF (0x0a 0x38): RF information per RF block. |
| [`mon_rxbuf.hpp`](mon_rxbuf.md) | UBX-MON-RXBUF (0x0a 0x07): receiver buffer status. |
| [`mon_rxr.hpp`](mon_rxr.md) | UBX-MON-RXR (0x0a 0x21): receiver status information. |
| [`mon_span.hpp`](mon_span.md) | UBX-MON-SPAN (0x0a 0x31): signal characteristics (basic spectrum analyzer). |
| [`mon_spt.hpp`](mon_spt.md) | UBX-MON-SPT (0x0a 0x2f): sensor production test. |
| [`mon_txbuf.hpp`](mon_txbuf.md) | UBX-MON-TXBUF (0x0a 0x08): transmitter buffer status. |
| [`mon_ver.hpp`](mon_ver.md) | UBX-MON-VER (0x0a 0x04): receiver and software version. |

### NAV (0x01)

| Header | Message |
| --- | --- |
| [`nav_att.hpp`](nav_att.md) | UBX-NAV-ATT (0x01 0x05): vehicle attitude solution. |
| [`nav_clock.hpp`](nav_clock.md) | UBX-NAV-CLOCK (0x01 0x22): receiver clock solution. |
| [`nav_cov.hpp`](nav_cov.md) | UBX-NAV-COV (0x01 0x36): position and velocity covariance matrices. |
| [`nav_dop.hpp`](nav_dop.md) | UBX-NAV-DOP (0x01 0x04): dilution of precision. |
| [`nav_eell.hpp`](nav_eell.md) | UBX-NAV-EELL (0x01 0x3D): position error ellipse parameters. |
| [`nav_eoe.hpp`](nav_eoe.md) | UBX-NAV-EOE (0x01 0x61): end-of-epoch marker. |
| [`nav_geofence.hpp`](nav_geofence.md) | UBX-NAV-GEOFENCE (0x01 0x39): geofencing status. |
| [`nav_hpposecef.hpp`](nav_hpposecef.md) | UBX-NAV-HPPOSECEF (0x01 0x13): high-precision position solution in ECEF. |
| [`nav_hpposllh.hpp`](nav_hpposllh.md) | UBX-NAV-HPPOSLLH (0x01 0x14): high-precision geodetic position solution. |
| [`nav_odo.hpp`](nav_odo.md) | UBX-NAV-ODO (0x01 0x09): odometer solution. |
| [`nav_orb.hpp`](nav_orb.md) | UBX-NAV-ORB (0x01 0x34): GNSS orbit database info. |
| [`nav_posecef.hpp`](nav_posecef.md) | UBX-NAV-POSECEF (0x01 0x01): position solution in ECEF. |
| [`nav_posllh.hpp`](nav_posllh.md) | UBX-NAV-POSLLH (0x01 0x02): geodetic position solution. |
| [`nav_pvt.hpp`](nav_pvt.md) | UBX-NAV-PVT (0x01 0x07): navigation position velocity time solution. |
| [`nav_relposned.hpp`](nav_relposned.md) | UBX-NAV-RELPOSNED (0x01 0x3C): relative position from the reference station in NED. |
| [`nav_sat.hpp`](nav_sat.md) | UBX-NAV-SAT (0x01 0x35): satellite information. |
| [`nav_sbas.hpp`](nav_sbas.md) | UBX-NAV-SBAS (0x01 0x32): SBAS status data. |
| [`nav_sig.hpp`](nav_sig.md) | UBX-NAV-SIG (0x01 0x43): signal information. |
| [`nav_slas.hpp`](nav_slas.md) | UBX-NAV-SLAS (0x01 0x42): QZSS L1S SLAS status data. |
| [`nav_status.hpp`](nav_status.md) | UBX-NAV-STATUS (0x01 0x03): receiver navigation status. |
| [`nav_timebds.hpp`](nav_timebds.md) | UBX-NAV-TIMEBDS (0x01 0x24): BeiDou time solution. |
| [`nav_timegal.hpp`](nav_timegal.md) | UBX-NAV-TIMEGAL (0x01 0x25): Galileo time solution. |
| [`nav_timeglo.hpp`](nav_timeglo.md) | UBX-NAV-TIMEGLO (0x01 0x23): GLONASS time solution. |
| [`nav_timegps.hpp`](nav_timegps.md) | UBX-NAV-TIMEGPS (0x01 0x20): GPS time solution. |
| [`nav_timels.hpp`](nav_timels.md) | UBX-NAV-TIMELS (0x01 0x26): leap second event information. |
| [`nav_timeqzss.hpp`](nav_timeqzss.md) | UBX-NAV-TIMEQZSS (0x01 0x27): QZSS time solution. |
| [`nav_timeutc.hpp`](nav_timeutc.md) | UBX-NAV-TIMEUTC (0x01 0x21): UTC time solution. |
| [`nav_velecef.hpp`](nav_velecef.md) | UBX-NAV-VELECEF (0x01 0x11): velocity solution in ECEF. |
| [`nav_velned.hpp`](nav_velned.md) | UBX-NAV-VELNED (0x01 0x12): velocity solution in the NED frame. |

### NAV2 (0x29)

| Header | Message |
| --- | --- |
| [`nav2_dop.hpp`](nav2_dop.md) | UBX-NAV2-DOP (0x29 0x04): secondary-output dilution of precision. |
| [`nav2_eell.hpp`](nav2_eell.md) | UBX-NAV2-EELL (0x29 0x3D): secondary-output position error ellipse parameters. |
| [`nav2_pvt.hpp`](nav2_pvt.md) | UBX-NAV2-PVT (0x29 0x07): secondary-output navigation position velocity time solution. |
| [`nav2_timegps.hpp`](nav2_timegps.md) | UBX-NAV2-TIMEGPS (0x29 0x20): secondary-output GPS time solution. |

### RXM (0x02)

| Header | Message |
| --- | --- |
| [`rxm_measx.hpp`](rxm_measx.md) | UBX-RXM-MEASX (0x02 0x14): satellite measurements for RRLP. |
| [`rxm_pmreq.hpp`](rxm_pmreq.md) | UBX-RXM-PMREQ (0x02 0x41): power management request. |
| [`rxm_rawx.hpp`](rxm_rawx.md) | UBX-RXM-RAWX (0x02 0x15): multi-GNSS raw measurements. |
| [`rxm_rlm.hpp`](rxm_rlm.md) | UBX-RXM-RLM (0x02 0x59): Galileo SAR Return Link Message reports. |
| [`rxm_rtcm.hpp`](rxm_rtcm.md) | UBX-RXM-RTCM (0x02 0x32): RTCM input status. |

### SEC (0x27)

| Header | Message |
| --- | --- |
| [`sec_crc.hpp`](sec_crc.md) | UBX-SEC-CRC (0x27 0x01): CRC of a configuration or firmware region. |
| [`sec_sig.hpp`](sec_sig.md) | UBX-SEC-SIG (0x27 0x09): signal security (jamming / spoofing) indicator. |
| [`sec_uniqid.hpp`](sec_uniqid.md) | UBX-SEC-UNIQID (0x27 0x03): unique chip ID. |

### TIM (0x0D)

| Header | Message |
| --- | --- |
| [`tim_tm2.hpp`](tim_tm2.md) | UBX-TIM-TM2 (0x0D 0x03): time mark data. |
| [`tim_tp.hpp`](tim_tp.md) | UBX-TIM-TP (0x0D 0x01): time pulse time data. |
| [`tim_vrfy.hpp`](tim_vrfy.md) | UBX-TIM-VRFY (0x0D 0x06): sourced time verification. |

### UPD (0x09)

| Header | Message |
| --- | --- |
| [`upd_sos.hpp`](upd_sos.md) | UBX-UPD-SOS (0x09 0x14): save-on-shutdown backup in flash. |

### Shared helpers (`detail/`)

| Header | Purpose |
| --- | --- |
| [`detail/fixed_message.hpp`](detail/fixed_message.md) | CRTP base for fixed-length messages: `matches`/`decode`/`encode` from one `layout()`. |
| [`detail/typed_message.hpp`](detail/typed_message.md) | Fixed layout selected by payload byte 0 (MGA, UPD-SOS, RXM-RLM). |
| [`detail/repeated_group.hpp`](detail/repeated_group.md) | Length validation and block decode/encode into `castle::container::vector`. |
| [`detail/cfg_data.hpp`](detail/cfg_data.md) | Versioned CFG base plus key-list and key/value `cfgData` codecs. |
| [`detail/esf_data.hpp`](detail/esf_data.md) | Signed 24-bit ESF data-word codec shared by ESF-MEAS and ESF-RAW. |
| [`detail/nav_gnss_time.hpp`](detail/nav_gnss_time.md) | Shared layout of NAV-TIMEBDS / TIMEGAL / TIMEQZSS. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/messages.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

// Receive
void on_message(const message_view& raw)
{
    nav_pvt pvt{};
    if (nav_pvt::decode(raw, pvt) && pvt.fix_ok())
    {
        // pvt.latitude_deg(), pvt.longitude_deg() ...
    }
}

// Send
uint8_t frame[64U];
castle::size_type written = 0U;
castle::status result = encode_message(cfg_rst::cold_start(), frame, sizeof(frame), written);
result = encode_poll<mon_ver<>>(frame, sizeof(frame), written);
```

See [`samples/castle_ext/protocols/ubx/messages/messages.cpp`](../../../../../samples/castle_ext/protocols/ubx/messages/messages.cpp). For subscription-based dispatch of every receiver output, use `castle::parsers::ubx_parser::default_decoder_registry<>` ([decoder registry](../../../parsers/ubx_parser/decoder_registry.md)).

## Constraints & Notes
- Header-only. No heap allocation, exceptions, RTTI, virtual dispatch or STL. Variable-length messages store their blocks in a fixed-capacity `castle::container::vector` whose capacity is a template parameter (`nav_sat<16>` ...). The spec-named count field holds the wire count; `.size()` is what was stored.
- Decoded models own their data and never alias the parser's payload buffer.
- Models are plain value types and are not thread-safe.
- Messages that share one class/ID (MGA-*, UPD-SOS, RXM-RLM, RXM-PMREQ, CFG v0/v1) are distinct models; at most one of them matches a given frame.
- UBX-SEC-CRC and UBX-SEC-SIG are not part of UBX-19056845. They follow the generic u-blox layout and accept longer payloads from newer firmware.
