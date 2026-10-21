# RTCM 3.3 correction input to a u-blox receiver

## Overview

UBX-19056845 (ZED-F9R interface description), chapter 4, lists the RTCM 3.3 messages the receiver accepts as correction input. Castle builds every one of them with the encode-only models in [`castle_ext/protocols/rtcm/messages/`](../../rtcm/messages/messages.md). This page maps each u-blox entry to its Castle model, explains what the u-blox "Class/ID" means, and links one sample per message group.

**Short version:** encode the message with `castle::protocols::rtcm::encode_message()` and write the resulting bytes to the receiver port unchanged. RTCM frames are **not** wrapped in a UBX frame.

## Reading a u-blox `RTCM-3X-TYPEnnnn` entry

Every entry in section 4.4 looks like this:

```
Message      RTCM-3X-TYPE1002   Extended L1-only GPS RTK observables
Type         Input
Information  Class/ID: 0xf5 0x02, Message Type: 1002 (0x3ea), Message Size: 6 + numData
Payload description:
  0            U1     preamble   Preamble (0xd3)
  1            X2     bitfield0  bits 9..0 numData, bits 15..10 res1 (reserved, zero)
  3 + n        U1     data       Message payload data       (numData times)
  3 + numData  U1[3]  crc        Checksum
```

Three facts in the document show that the "payload description" is the complete wire frame and that the Class/ID is never sent:

1. **The table starts with `0xd3` and ends with a 3-byte CRC.** That is the RTCM 3 transport frame `[0xd3][6 reserved bits | 10-bit length][payload][CRC-24Q]`, which is exactly what `rtcm::encode_message()` produces. A UBX frame would start with `0xb5 0x62`, use a 16-bit little-endian length and end with a 2-byte Fletcher checksum.
2. **"Message Size: 6 + numData"** is the RTCM overhead (3 header bytes + 3 CRC bytes). A UBX frame around it would have 8 more bytes of overhead.
3. **The entries have no "Message structure" row.** In chapter 3, a real UBX message such as UBX-RXM-RTCM shows `Message structure: 0xb5 0x62 | 0x02 | 0x32 | 8 | payload | CK_A CK_B`. The RTCM entries leave that row out, and the 0xf5 Class/ID appears only on the "Information" line.

The Class/ID is u-blox's internal catalogue key for every message the firmware knows, whatever its protocol. The same document gives NMEA sentences one too, for example `NMEA-Standard-GGA` has `Class/ID: 0xf0 0x00`, yet GGA is the ASCII line `$GPGGA,...*5B\r\n`. Class `0xf0`/`0xf1` stands for NMEA/PUBX, and `0xf5` stands for RTCM 3. The receiver's port parser identifies each protocol by its first byte (`0xb5` UBX, `$` NMEA, `0xd3` RTCM 3), so UBX, NMEA and RTCM frames can share one UART.

```
host --UART1--> [b5 62 06 8a ... CK_A CK_B]   UBX-CFG-VALSET    (configuration)
                [d3 00 15 3e e4 d2 ... crc ]  RTCM 1006         (correction input, as is)
                [d3 00 60 43 54 d2 ... crc ]  RTCM 1077
host <--UART1-- [b5 62 02 32 08 00 ... ]      UBX-RXM-RTCM      (one per parsed RTCM frame)
```

The Class/ID only identifies the message within u-blox's catalogue; it never appears on the wire. The value that identifies the message in the frame is the 12-bit message number DF002 at the start of the RTCM payload. UBX-RXM-RTCM reports that number back in its `msgType` field.

## Mapping table

All models are in `castle::protocols::rtcm::messages`; the message number constants (`RTCM_TYPE_*`) are in `castle::protocols::rtcm`. Every message number is its own model type (the number is fixed at compile time, as for the MSM aliases), so the model column is the whole selection. The frame size column is the size of the frame each sample sends.

| u-blox message | Class/ID | Description | Castle model | Sample frame size |
| --- | --- | --- | --- | --- |
| RTCM-3X-TYPE1001 | 0xf5 0x01 | L1-only GPS RTK observables | `gps_rtk_l1_obs` | 36 B (3 sats) |
| RTCM-3X-TYPE1002 | 0xf5 0x02 | Extended L1-only GPS RTK observables | `gps_rtk_l1_obs_ext` | 42 B |
| RTCM-3X-TYPE1003 | 0xf5 0x03 | L1/L2 GPS RTK observables | `gps_rtk_l1_l2_obs` | 52 B |
| RTCM-3X-TYPE1004 | 0xf5 0x04 | Extended L1/L2 GPS RTK observables | `gps_rtk_l1_l2_obs_ext` | 61 B |
| RTCM-3X-TYPE1005 | 0xf5 0x05 | Stationary RTK reference station ARP | `station_arp` | 25 B |
| RTCM-3X-TYPE1006 | 0xf5 0x06 | Station ARP with antenna height | `station_arp_height` | 27 B |
| RTCM-3X-TYPE1007 | 0xf5 0x07 | Antenna descriptor | `antenna_descriptor` | 31 B |
| RTCM-3X-TYPE1009 | 0xf5 0x09 | L1-only GLONASS RTK observables | `glonass_rtk_l1_obs` | 38 B (3 sats) |
| RTCM-3X-TYPE1010 | 0xf5 0x0a | Extended L1-only GLONASS RTK observables | `glonass_rtk_l1_obs_ext` | 44 B |
| RTCM-3X-TYPE1011 | 0xf5 0xa1 | L1&L2 GLONASS RTK observables | `glonass_rtk_l1_l2_obs` | 54 B |
| RTCM-3X-TYPE1012 | 0xf5 0xa2 | Extended L1&L2 GLONASS RTK observables | `glonass_rtk_l1_l2_obs_ext` | 63 B |
| RTCM-3X-TYPE1033 | 0xf5 0x21 | Receiver and antenna descriptors | `receiver_antenna_descriptor` | 67 B |
| RTCM-3X-TYPE1074 | 0xf5 0x4a | GPS MSM4 | `gps_msm4` | 71 B (3 sats x 2 sigs) |
| RTCM-3X-TYPE1075 | 0xf5 0x4b | GPS MSM5 | `gps_msm5` | 89 B |
| RTCM-3X-TYPE1077 | 0xf5 0x4d | GPS MSM7 | `gps_msm7` | 102 B |
| RTCM-3X-TYPE1084 | 0xf5 0x54 | GLONASS MSM4 | `glonass_msm4` | 71 B |
| RTCM-3X-TYPE1085 | 0xf5 0x55 | GLONASS MSM5 | `glonass_msm5` | 89 B |
| RTCM-3X-TYPE1087 | 0xf5 0x57 | GLONASS MSM7 | `glonass_msm7` | 102 B |
| RTCM-3X-TYPE1094 | 0xf5 0x5e | Galileo MSM4 | `galileo_msm4` | 71 B |
| RTCM-3X-TYPE1095 | 0xf5 0x5f | Galileo MSM5 | `galileo_msm5` | 89 B |
| RTCM-3X-TYPE1097 | 0xf5 0x61 | Galileo MSM7 | `galileo_msm7` | 102 B |
| RTCM-3X-TYPE1124 | 0xf5 0x7c | BeiDou MSM4 | `beidou_msm4` | 71 B |
| RTCM-3X-TYPE1125 | 0xf5 0x7d | BeiDou MSM5 | `beidou_msm5` | 89 B |
| RTCM-3X-TYPE1127 | 0xf5 0x7f | BeiDou MSM7 | `beidou_msm7` | 102 B |
| RTCM-3X-TYPE1230 | 0xf5 0xe6 | GLONASS L1 and L2 code-phase biases | `glonass_code_phase_biases` | 14 B (2 biases) |

The legacy observables aliases (1001-1004, 1009-1012) have the full capacity of 31 satellites. For a smaller satellite capacity name the message number and the capacity, for example `gps_rtk_observables<RTCM_TYPE_GPS_L1_OBS, N>` or `glonass_rtk_observables<RTCM_TYPE_GLO_L1_OBS, N>`.

For most IDs the low byte is `type - 1000`. The exceptions (`0xa1`, `0xa2`, `0x21`, `0xe6` for 1011, 1012, 1033, 1230) show that the ID is a u-blox key, not something derived from the frame. Castle has no constants for these IDs, because no encoder ever writes them.

Messages outside this list (for example MSM1-3/MSM6, SBAS/QZSS MSM, 1008, 1019/1020 ephemerides, SSR) are not supported as input. UBX-RXM-RTCM is still output for them, because it is emitted "upon successful parsing of an RTCM input message, irrespective of whether the RTCM message is supported or not by the receiver". The model for SBAS/QZSS MSM (`msm_message<msm_system::qzss, 7U>`) exists but is not in the u-blox input list.

## Sending corrections step by step

1. **Configure the port (UBX, once).** Send UBX-CFG-VALSET with the relevant keys:

   | Key | ID | Type | Default | Purpose |
   | --- | --- | --- | --- | --- |
   | `CFG-UART1INPROT-RTCM3X` | `0x10730004` | L | 1 | Accept RTCM 3 input on UART1. Siblings: UART2 `0x10750004`, USB `0x10770004`, I2C `0x10710004`, SPI `0x10790004` (all default 1). |
   | `CFG-RTCM-DF003_IN` | `0x30090008` | U2 | 0 | Reference station ID used by the input filter. |
   | `CFG-RTCM-DF003_IN_FILTER` | `0x20090009` | E1 | 0 | 0 DISABLED, 1 RELAXED (messages with a different DF003 are dropped; messages without DF003 pass), 2 STRICT (messages must carry a matching DF003). |
   | `CFG-MSGOUT-UBX_RXM_RTCM_UART1` | `0x20910269` | U1 | 0 | Rate of UBX-RXM-RTCM on UART1 (I2C `0x20910268`, SPI `0x2091026c`, UART2 `0x2091026a`, USB `0x2091026b`). |

   Build the frame with `ubx::messages::cfg_valset<>` and `ubx::encode_message()` (see [`cfg_valset`](../messages/cfg_valset.md)). The receiver answers with UBX-ACK-ACK or UBX-ACK-NAK.

2. **Stream corrections (RTCM, every epoch).** Send the reference station position first (1005 or 1006, usually every 5-10 s), then each epoch's observations. MSM is the usual choice for a multi-band, multi-GNSS rover: send one MSM per constellation, and set `multiple_message = true` (DF393) on every MSM of the epoch except the last. Legacy observables 1001-1004 and 1009-1012 are for base stations that do not support MSM; in that case `synchronous` (DF005) plays the same role. Add 1230 for GLONASS when the base station receiver is not a u-blox receiver. 1007 and 1033 are informational.

   ```cpp
   namespace rtcm = castle::protocols::rtcm;

   rtcm::messages::gps_msm7 epoch{};
   epoch.station_id = 1234U;
   epoch.epoch_time = gps_tow_ms;
   // ... masks, satellites and cells (see msm.md)

   uint8_t tx[rtcm::RTCM_MAX_FRAME_SIZE];
   castle::size_type written = 0U;
   if (castle::succeeded(rtcm::encode_message(epoch, tx, sizeof(tx), written)))
   {
       uart_write(tx, written); // the bytes go to the receiver unchanged
   }
   ```

3. **Check the feedback (UBX).** For every RTCM frame it parses, the receiver outputs UBX-RXM-RTCM. Decode it with [`rxm_rtcm`](../messages/rxm_rtcm.md):
   - `crc_failed()` means the frame was damaged on the way, so the link is bad;
   - `msg_used()` is `used` (used successfully), `not_used` or `unknown`;
   - `msg_type` and `ref_station` echo the DF002 and DF003 of the frame you sent.

   Whether the corrections reached the solution shows in UBX-NAV-PVT `carr_soln()` (float/fixed RTK), UBX-NAV-SAT `rtcm_corr_used()` and UBX-NAV-SIG `corr_source` (`rtcm3_osr`).

## Samples

All samples live in `samples/castle_ext/protocols/ubx/rtcm/` and write to a simulated UART1 transmit buffer. After each frame they check that it matches the u-blox payload table: preamble `0xd3`, zero reserved bits, `size == 6 + numData`, DF002, DF003, and a CRC-24Q residue of 0. Every frame they produce was also decoded with the independent Python decoder `pyrtcm`, and the decoded values matched the input measurements.

| Sample | u-blox messages |
| --- | --- |
| [`rtcm_input.cpp`](../../../../../samples/castle_ext/protocols/ubx/rtcm/rtcm_input.cpp) | End-to-end flow: CFG-VALSET configuration, then 1006 and 1077 on the same port, then decoding the UBX-RXM-RTCM reply. Also shows that `ubx::decode_frame()` rejects an RTCM frame. |
| [`rtcm_3x_gps_observables.cpp`](../../../../../samples/castle_ext/protocols/ubx/rtcm/rtcm_3x_gps_observables.cpp) | 1001, 1002, 1003, 1004 from physical pseudoranges, carrier phases and CNR. |
| [`rtcm_3x_glonass_observables.cpp`](../../../../../samples/castle_ext/protocols/ubx/rtcm/rtcm_3x_glonass_observables.cpp) | 1009, 1010, 1011, 1012 including the frequency channel (DF040 = k + 7). |
| [`rtcm_3x_station.cpp`](../../../../../samples/castle_ext/protocols/ubx/rtcm/rtcm_3x_station.cpp) | 1005, 1006, 1007, 1033, 1230. |
| [`rtcm_3x_msm.cpp`](../../../../../samples/castle_ext/protocols/ubx/rtcm/rtcm_3x_msm.cpp) | MSM4, MSM5 and MSM7 for GPS, GLONASS, Galileo and BeiDou (all twelve numbers), two signals per satellite, one full multi-GNSS epoch per level. |

The shared sample plumbing (`rtcm_input_support.hpp`) holds the configuration key IDs, one epoch of measurements per constellation, and the conversions from metres and dB-Hz to the raw data fields. These conversions are:

- legacy observables: the pseudorange is split into ambiguity (DF014 / DF044) and remainder (DF011 / DF041, 0.02 m), and the phase is sent relative to the rebuilt pseudorange (DF012 / DF042, 0.0005 m);
- MSM: `range_m / 299792.458` gives light-milliseconds, which are fed to `set_rough_range()`, `set_pseudorange()`, `set_phase_range()`, `set_phase_range_rate()` and `set_cnr()`.

MSM signal IDs (DF395) used by the samples, from RTCM 10403.3:

| System | Signal 1 | Signal 2 |
| --- | --- | --- |
| GPS | 2 = L1 C/A (1C) | 16 = L2C L (2L) |
| GLONASS | 2 = G1 C/A (1C) | 8 = G2 C/A (2C) |
| Galileo | 2 = E1 C (1C) | 14 = E5b I (7I) |
| BeiDou | 2 = B1I (2I) | 14 = B2I (7I) |

## Constraints & Notes

- There is no UBX wrapper and no 0xf5 class on the wire. Wrapping an RTCM frame in `b5 62 f5 xx ...` makes it unreadable to the receiver: its UBX parser has no handler for class 0xf5, and its RTCM parser never sees the `0xd3` sync byte.
- RTCM frames and UBX frames may be interleaved on one port, but never split a frame: write each `encode_message()` result as one contiguous block.
- `encode_message()` returns `written == 0` on any failure, and the buffer contents must then not be sent. The sample `receiver_port` advances by `written`, so a failed encode queues nothing.
- Everything is encoded in place into the caller's buffer, with no heap, exceptions, RTTI or STL.
- RTCM input is enabled by default on every port of a ZED-F9R (UART2 included). The DF003 filter is off by default, so a receiver connected to a network stream with several base stations should enable it.
