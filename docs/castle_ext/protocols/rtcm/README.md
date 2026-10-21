# RTCM 3.x Message and Frame Specification

## Contents

* [Status and sources](#status-and-sources)
* [1. Overview](#1-overview)
* [2. Notation and data types](#2-notation-and-data-types)
* [3. Transport frame](#3-transport-frame)
* [4. Message catalogue](#4-message-catalogue)
* [5. Station and service messages](#5-station-and-service-messages)
* [6. Legacy RTK observation messages (1001-1004, 1009-1012)](#6-legacy-rtk-observation-messages-1001-1004-1009-1012)
* [7. Multiple Signal Messages (MSM)](#7-multiple-signal-messages-msm)
* [8. Broadcast ephemeris messages (1019, 1020, 1042, 1044, 1045, 1046)](#8-broadcast-ephemeris-messages-1019-1020-1042-1044-1045-1046)
* [9. State space representation messages (1057-1068, 1240-1263)](#9-state-space-representation-messages-1057-1068-1240-1263)
* [10. Encoding rules (Castle)](#10-encoding-rules-castle)
* [11. Decoding rules (gnss++, Kalmix)](#11-decoding-rules-gnss-kalmix)
* [12. Interoperability profiles](#12-interoperability-profiles)
* [13. Source discrepancies](#13-source-discrepancies)
* [14. Verification record](#14-verification-record)
* [15. Castle implementation map](#15-castle-implementation-map)
* [Appendix A. Worked examples](#appendix-a-worked-examples)

## Status and sources

This document specifies the RTCM 3.x transport frame, the data-field encoding rules and the message layouts that Castle emits (RTCM 3.3, as accepted by a u-blox ZED-F9R), plus the neighbouring messages that the reference decoders handle.

| Tag | Source | Role |
| --- | --- | --- |
| **[C]** | Castle `include/castle_ext/protocols/rtcm/` (headers, tests, samples) | Primary source. Every Castle-implemented message in this document was also built and decoded (see §14). |
| **[G]** | gnss++ (`rsasaki0109/gnssplusplus-library`, branch `develop`): `src/io/rtcm*.cpp`, `rtcm_internal.hpp` | Decoder and encoder. Source for MSM decode rules, signal maps, broadcast ephemerides, SSR, time conversions and stream handling. |
| **[K]** | Kalmix AN-002 *RTCM SC-104 Frame Structure & Message Type Reference* and *RTCM Unpacked* (GNSS Handbook) | Field-level prose, MSM tables, interoperability guidance. |
| **[P]** | `pyrtcm` 1.2.1 field tables (transcribed from RTCM 10403.3) | Not a requested source. Used only as an independent check of DF numbers and field widths. |
| **[R]** | RTCM 10403.x / RTKLIB conventions recalled by the author | **Unverified.** Not found in [C], [G], [K] or [P]. Treat as a hint and confirm against the standard before relying on it. |

An untagged statement comes from [C], or is confirmed by at least two sources. Where sources disagree, the document follows [C] and records the disagreement in §13.

### Scope

* Castle is **encode-only** and builds the RTCM correction input of a u-blox receiver: the frame, MSM4/5/7 for GPS, GLONASS, Galileo and BeiDou (SBAS and QZSS are available through the template), legacy GPS and GLONASS observables, station messages 1005, 1006, 1007, 1033 and 1230.
* The ephemeris (§8) and SSR (§9) messages are documented from [G] and [P] only. Castle has no models for them.
* MSM1-3 and MSM6 are documented for completeness. Castle has no models for them.
* Everything is bit-exact: a "start bit" is counted from the first bit of the payload, bit 0 being the most significant bit of payload byte 0.

## 1. Overview

RTCM 10403 (RTCM SC-104 *Differential GNSS Services, Version 3*) defines a binary stream for sending GNSS correction data from a reference station, or a correction service, to a rover. Every message travels in the same transport frame (§3) and carries a 12-bit message number (DF002) as its first field.

Two correction models share the frame:

* **OSR** (observation space representation): the reference station's raw observations and position. The rover differences them against its own observations. Messages 1001-1012 (legacy) and 1071-1137 (MSM), together with station messages 1005-1008, 1033 and 1230.
* **SSR** (state space representation): modelled orbit, clock, bias and atmosphere corrections for PPP and PPP-RTK. Messages 1057-1068 and 1240-1263. [K] Standard OSR RTK services do not carry SSR.

| Year | Version | Relevance [K] |
| --- | --- | --- |
| 2004 | RTCM 3.0 | Variable-length frame with CRC-24Q. |
| 2006 | RTCM 3.1 (10403.1) | Network RTK; broadcast ephemerides 1019 and 1020. |
| 2013 | RTCM 3.2 (10403.2) | Multiple Signal Messages (MSM). |
| 2016 | RTCM 3.3 (10403.3) | Accumulated amendments; BeiDou ephemeris 1042 and Galileo I/NAV ephemeris 1046 belong to this version. |
| 2023 | RTCM 3.4 (10403.4) | Still version 3 (first published 2023, Amendment 1 in 2024). |

MSM introduced a signal-aware layout that scales to every constellation. Legacy observables exist only for GPS and GLONASS on L1/L2, and there is no legacy message for Galileo or BeiDou. [K]

## 2. Notation and data types

### 2.1 Bit stream

* A payload is a bit stream. Fields are packed **most significant bit first** with no alignment between them. A field may start and end in the middle of a byte.
* Bit 0 of the payload is the MSB of payload byte 0. "Start bit" columns in this document count from there.
* After the last field the payload is padded with zero bits to a whole byte. The frame length field counts payload **bytes**, so the receiver cannot tell padding from data except through the message layout. Castle's `bit_writer` clears every new byte before writing into it, so padding is always zero. [C]
* Reserved bits are written as zero. [C]

### 2.2 Field types

| Type | Meaning | Range |
| --- | --- | --- |
| `bit(n)` | Raw bit string with no numeric meaning (masks, flags, indicators). | n bits |
| `uint(n)` | Unsigned integer. | 0 .. 2^n − 1 |
| `int(n)` | Two's complement signed integer. | −2^(n−1) .. 2^(n−1) − 1 |
| `intS(n)` | Sign-magnitude: the MSB is the sign, the other n − 1 bits the magnitude. Used by the GLONASS ephemeris. [G] [P] | −(2^(n−1) − 1) .. 2^(n−1) − 1 |
| `char8(k)` | k 8-bit characters. In Castle always preceded by an 8-bit count, so a string is `uint(8)` + `char8(count)`. [C] | count 0..255; Castle limits descriptors to 31 characters |

Widths up to 64 bits are used (the satellite mask DF394 is `bit(64)`, the ECEF coordinates `int(38)`). A value that does not fit its field is an encoding error, never silently truncated. [C]

### 2.3 Resolution and invalid markers

A scaled field carries `physical value = raw integer × resolution`. Resolutions of the form 2^−n are exact in binary.

Many signed fields reserve their most negative value as "invalid" (the observation is missing). The markers used by MSM are listed in §7.7, those of the legacy messages in §6.4. A decoder must test for the marker before scaling.

### 2.4 Constants used in the formulas

| Symbol | Value | Meaning |
| --- | --- | --- |
| c | 299 792 458 m/s | Speed of light. |
| c_ms | 299 792.458 m | One **light-millisecond**: the distance light travels in 1 ms. Ranges in MSM are expressed in light-milliseconds. [C] [G] |
| semicircle | π rad | Unit of ephemeris angles. [G] |
| DF001 | — | Label used for reserved bits. [P] |


## 3. Transport frame

### 3.1 Layout

```
byte 0     bytes 1..2                 bytes 3 .. 3+L-1     bytes 3+L .. 5+L
+--------+-------------------------+--------------------+--------------------+
|  0xD3  | 000000 | length (10 bit)|  payload (L bytes) |  CRC-24Q (3 bytes) |
+--------+-------------------------+--------------------+--------------------+
```

| Field | Bits | Value |
| --- | --- | --- |
| Preamble | 8 | `0xD3` (`11010011`). |
| Reserved | 6 | `000000`. A receiver treats a non-zero value as a false preamble. [K] |
| Length L | 10 | Payload size in bytes, 0..1023. The frame carries `L + 6` bytes. |
| Payload | 8·L | Message. The first 12 bits are DF002, the message number. |
| CRC-24Q | 24 | Over bytes 0 .. 2+L (preamble, length and payload), transmitted most significant byte first. |

| Constant | Value | Castle name |
| --- | --- | --- |
| Header size | 3 | `RTCM_FRAME_INDEX_PAYLOAD` |
| CRC size | 3 | `RTCM_CRC_SIZE` |
| Overhead | 6 | `RTCM_FRAME_OVERHEAD` |
| Largest payload | 1023 | `RTCM_MAX_PAYLOAD_LEN` |
| Largest frame | 1029 | `RTCM_MAX_FRAME_SIZE` |

The length is written as a 16-bit big-endian integer; the six reserved bits stay zero because the value never exceeds 1023. [C]

### 3.2 CRC-24Q

| Parameter | Value |
| --- | --- |
| Width | 24 bits |
| Polynomial | `0x864CFB` (`0x1864CFB` when the x^24 term is written out [K]) |
| Initial value | 0 |
| Input / output reflection | none |
| Final XOR | 0 |
| Check value | `CRC("123456789") = 0xCDE703` |

Properties:

* The CRC of the complete frame, trailer included, is **0**. Both Castle's tests and the gnss++ decoder rely on this (Castle `crc24q::calculate(frame, size) == 0`). [C]
* The CRC covers the header, so a corrupted length is detected as well.

Bitwise reference [K]:

```c
unsigned int crc24q(const unsigned char *buf, int len)
{
    unsigned int crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= (unsigned int)buf[i] << 16;
        for (int j = 0; j < 8; j++) {
            crc <<= 1;
            if (crc & 0x1000000) crc ^= 0x1864CFB;
        }
    }
    return crc & 0xFFFFFF;
}
```

gnss++ uses the table-driven form `crc = (crc << 8) ^ table[((crc >> 16) ^ byte) & 0xFF]` with `table[1] = 0x864CFB`, `table[2] = 0x8AD50D`, `table[3] = 0x0C99F6`. [G] Castle's implementation (`castle_ext/checksums/crc24q.hpp`) is table-free and produces the same values.

### 3.3 Building a frame (Castle)

1. Write the payload bytes to `frame[3 ..]`.
2. `frame[0] = 0xD3`; write the payload length as big-endian 16-bit at `frame[1..2]`.
3. Compute the CRC-24Q over `frame[0 .. 2+L]` and store it big-endian at `frame[3+L .. 5+L]`.

`encode_frame()` frames an existing payload. `encode_message()` lets a message model write its bits straight into `frame[3 ..]` and then completes the frame, so no scratch buffer is needed. An empty payload yields the valid 6-byte frame `D3 00 00 47 EA 4B`. [C] (gnss++ does not accept payloads shorter than 2 bytes because it must read DF002. [G])

### 3.4 Receiving a stream

RTCM frames have no delimiter. A serial port or a TCP socket delivers arbitrary chunks (half a frame, several frames), so the receiver buffers bytes and synchronises itself. [K] [G]

```
SYNC     scan for 0xD3
HEADER   need 3 bytes; if (byte1 & 0xFC) != 0 -> false preamble, drop 1 byte, SYNC
         L = ((byte1 & 0x03) << 8) | byte2
PAYLOAD  buffer L + 3 bytes (payload and CRC)
CRC      compute CRC-24Q over 3 + L bytes and compare;
         on mismatch drop ONE byte after the preamble position and SYNC
DISPATCH DF002 = first 12 bits of the payload; route to the message decoder; consume 6 + L bytes
```

* After a CRC failure the parser resumes at `preamble position + 1`, never at `preamble position + 6 + L`, because the `0xD3` may have been a data byte and a real frame can start inside the bytes already consumed. gnss++ does exactly this (`++i`). [G] [K]
* gnss++ validates only `length >= 2` and does not test the six reserved bits (it masks them with `0x03`). [G] A conforming receiver should test them, as [K] prescribes.
* A decoder must never assume one `recv()` equals one frame.
* MSM-style parsers must still range-check every length derived from the payload (§11.2) because a valid CRC proves transport integrity, not layout correctness.

### 3.5 Maximum sizes

| Case | Payload | Frame |
| --- | --- | --- |
| Frame limit | 1023 bytes | 1029 bytes |
| Largest MSM7 (64 satellite-signal cells, 64 satellites × 1 signal) | 7657 bits (958 bytes) | 964 bytes |
| Largest legacy 1004 (31 satellites) | 493 bytes | 499 bytes |

Every legal MSM fits one frame, because N_sat × N_sig ≤ 64 bounds the size (§7.12).


## 4. Message catalogue

DF002 is a 12-bit field, so message numbers run 0..4095. The table lists the numbers that appear in the sources. "Castle" names the model that builds the message; "gnss++" says what the reference library does with it.

| Number | Content | Castle | gnss++ | § |
| --- | --- | --- | --- | --- |
| 1001 | GPS L1-only RTK observables | `gps_rtk_l1_obs` | name only | 6 |
| 1002 | GPS extended L1-only RTK observables | `gps_rtk_l1_obs_ext` | name only | 6 |
| 1003 | GPS L1/L2 RTK observables | `gps_rtk_l1_l2_obs` | decode, encode (layout deviates, §13) | 6 |
| 1004 | GPS extended L1/L2 RTK observables | `gps_rtk_l1_l2_obs_ext` | decode, encode | 6 |
| 1005 | Stationary reference station ARP | `station_arp` | decode (position only) | 5.1 |
| 1006 | ARP with antenna height | `station_arp_height` | decode (position only) | 5.1 |
| 1007 | Antenna descriptor | `antenna_descriptor` | name only | 5.2 |
| 1008 | Antenna descriptor and serial number | none | name only | 5.2 |
| 1009 | GLONASS L1-only RTK observables | `glonass_rtk_l1_obs` | name only | 6 |
| 1010 | GLONASS extended L1-only RTK observables | `glonass_rtk_l1_obs_ext` | name only | 6 |
| 1011 | GLONASS L1/L2 RTK observables | `glonass_rtk_l1_l2_obs` | name only | 6 |
| 1012 | GLONASS extended L1/L2 RTK observables | `glonass_rtk_l1_l2_obs_ext` | name only | 6 |
| 1019 | GPS broadcast ephemeris | none | decode, encode | 8 |
| 1020 | GLONASS broadcast ephemeris | none | decode, encode | 8 |
| 1033 | Receiver and antenna descriptors | `receiver_antenna_descriptor` | name only | 5.3 |
| 1042 | BeiDou broadcast ephemeris | none | decode, encode | 8 |
| 1044 | QZSS broadcast ephemeris | none | decode, encode | 8 |
| 1045 | Galileo F/NAV broadcast ephemeris | none | decode, encode | 8 |
| 1046 | Galileo I/NAV broadcast ephemeris | none | decode, encode | 8 |
| 1057-1062 | GPS SSR: orbit, clock, code bias, combined orbit+clock, URA, high-rate clock | none | decode | 9 |
| 1063-1068 | GLONASS SSR (same six contents) | none | decode | 9 |
| 1071-1077 | GPS MSM1-MSM7 | `gps_msm4`, `gps_msm5`, `gps_msm7` (MSM4/5/7) | MSM4-7 decode, encode | 7 |
| 1081-1087 | GLONASS MSM1-MSM7 | `glonass_msm4/5/7` | MSM4-7 decode, encode | 7 |
| 1091-1097 | Galileo MSM1-MSM7 | `galileo_msm4/5/7` | MSM4-7 decode, encode | 7 |
| 1101-1107 | SBAS MSM1-MSM7 | `msm_message<msm_system::sbas, L>` | not decoded | 7 |
| 1111-1117 | QZSS MSM1-MSM7 | `msm_message<msm_system::qzss, L>` | MSM4-7 decode, encode | 7 |
| 1121-1127 | BeiDou MSM1-MSM7 | `beidou_msm4/5/7` | MSM4-7 decode, encode | 7 |
| 1131-1137 | NavIC (IRNSS) MSM1-MSM7 [K] | none | MSM4-7 (1134-1137) decode, encode | 7 |
| 1230 | GLONASS L1 and L2 code-phase biases | `glonass_code_phase_biases` | none | 5.4 |
| 1240-1245 | Galileo SSR (six contents) | none | decode | 9 |
| 1246-1251 | QZSS SSR | none | decode | 9 |
| 1252-1257 | SBAS SSR | none | name-to-system mapping only | 9 |
| 1258-1263 | BeiDou SSR | none | decode | 9 |
| 1264 | VTEC ionosphere model [K] | none | none | — |
| 4072 | u-blox proprietary; appears only in UBX-RXM-RTCM output, never as input | none | none | — |

Other numbers exist in RTCM 10403.x (for example 1013-1017, 1021-1027, 1029-1032, 1034-1039, 1041 and 1300-1305 in the `pyrtcm` tables [P]). They are outside this document.

### 4.1 MSM numbering

`message number = base + level`, with `base` 1070 (GPS), 1080 (GLONASS), 1090 (Galileo), 1100 (SBAS), 1110 (QZSS), 1120 (BeiDou), 1130 (NavIC), and `level` 1..7. Castle's `msm_system` enum holds the base value, so `MESSAGE_TYPE = base + Level`. [C] [K]

### 4.2 Messages a u-blox ZED-F9R accepts as input

25 messages: 1001-1007, 1009-1012, 1033, 1230, and MSM4, MSM5, MSM7 of GPS, GLONASS, Galileo and BeiDou. Each frame is sent to the port as is. The full mapping, with the configuration keys that enable RTCM 3 input, is in [rtcm_input.md](../ubx/rtcm/rtcm_input.md). [C]


## 5. Station and service messages

All four messages start with DF002 and DF003. Offsets below are in bits from the start of the payload.

### 5.1 Messages 1005 and 1006: reference station antenna reference point

Message 1005 is 152 bits (19 bytes). Message 1006 appends DF028 and is 168 bits (21 bytes). Castle: `station_arp` (1005), `station_arp_height` (1006), both instances of `station_arp_message<Type>`.

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF021 | ITRF realization year | uint | 6 |  | 0 = unspecified |
| 30 | DF022 | GPS indicator | bit | 1 |  | 1 = GPS service supported |
| 31 | DF023 | GLONASS indicator | bit | 1 |  | 1 = GLONASS service supported |
| 32 | DF024 | Galileo indicator | bit | 1 |  | 1 = Galileo service supported |
| 33 | DF141 | Reference-station indicator | bit | 1 |  | 0 = physical station, 1 = non-physical (network) |
| 34 | DF025 | Antenna reference point ECEF-X | int | 38 | 0.0001 m |  |
| 72 | DF142 | Single receiver oscillator indicator | bit | 1 |  | 1 = all observations use one oscillator |
| 73 | DF001 | Reserved | bit | 1 |  | zero |
| 74 | DF026 | Antenna reference point ECEF-Y | int | 38 | 0.0001 m |  |
| 112 | DF364 | Quarter-cycle indicator | bit | 2 |  | 0 = unspecified, 1 = all phases corrected, 2 = corrected for L1 C/A only, 3 = reserved |
| 114 | DF027 | Antenna reference point ECEF-Z | int | 38 | 0.0001 m |  |
| 152 | DF028 | Antenna height | uint | 16 | 0.0001 m | 0..6.5535 m; 1006 only |

Rules:

* The coordinates are ECEF, in metres, of the **antenna reference point** (ARP), not the phase centre. The `int(38)` range is ±2^37 × 0.0001 m = ±13 743 895.347 m (about ±13 744 km [K]), enough for any terrestrial antenna.
* DF021 = 0 leaves the reference frame unspecified. [K]
* DF022-DF024 say which constellations the station serves. A rover cannot compute a baseline without the station position, so 1005 or 1006 is **required** for single-base RTK. [K]
* DF028 is `0..6.5535 m`. Castle's `set_antenna_height_m()` clamps to that range.
* DF141 = 1 marks a non-physical (network-generated) station.
* DF364 = 0 unspecified, 1 phases corrected for all signals, 2 corrected for L1 C/A only. 3 is reserved.
* Castle stores the raw `int64` values (`ecef_x`, `ecef_y`, `ecef_z`) and converts with `set_position_m()`: divide by 0.0001, round to nearest (half away from zero), NaN becomes 0, and a result far outside the field range is saturated so the writer reports `out_of_range`.
* gnss++ reads only the three coordinates, at start bits 34 (X), 74 (Y) and 114 (Z), and requires at least 152 bits (1005) or 168 bits (1006). [G]

Worked frame: Appendix A.1.

### 5.2 Messages 1007 and 1008: antenna descriptor

Message 1007:

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF029 | Descriptor counter N | uint | 8 |  | 0..31 (Castle limit) |
| 32 | DF030 | Antenna descriptor | char8 | 8 × N |  | IGS antenna name |
| 32 + 8·N | DF031 | Antenna setup ID | uint | 8 |  | 0 = default |

Message 1008 adds the antenna serial number after DF031:

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF029 | Descriptor counter N | uint | 8 |  | 0..31 (Castle limit) |
| 32 | DF030 | Antenna descriptor | char8 | 8 × N |  | IGS antenna name |
| 32 + 8·N | DF031 | Antenna setup ID | uint | 8 |  | 0 = default |
| 40 + 8·N | DF032 | Serial number counter M | uint | 8 |  |  |
| 48 + 8·N | DF033 | Antenna serial number | char8 | 8 × M |  |  |

* Strings are `uint(8)` count + `char8(count)`. Castle accepts at most 31 characters (`RTCM_MAX_DESCRIPTOR_LEN`): `descriptor_string` is `castle::container::string<31>`, so a longer string cannot be stored.
* The descriptor follows the IGS antenna naming convention (for example `LEIAR25.R4    LEIT`). [K]
* Payload size: `40 + 8·N` bits for 1007 (N = DF029), so 1007 with a 20-character descriptor is 25 bytes and a 31-byte frame. [C]
* Castle has no 1008 model; the layout is the 1007 layout plus the two serial-number fields, and was checked against the field table of [P].

### 5.3 Message 1033: receiver and antenna descriptors

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF029 | Antenna descriptor counter | uint | 8 |  | N1 = 0..31 |
| 32 | DF030 | Antenna descriptor | char8 | 8 × N1 |  |  |
| 32 + 8·N1 | DF031 | Antenna setup ID | uint | 8 |  |  |
| 40 + 8·N1 | DF032 | Antenna serial number counter | uint | 8 |  | N2 |
| 48 + 8·N1 | DF033 | Antenna serial number | char8 | 8 × N2 |  |  |
| 48 + 8·(N1 + N2) | DF227 | Receiver type descriptor counter | uint | 8 |  | N3 |
| 56 + 8·(N1 + N2) | DF228 | Receiver type descriptor | char8 | 8 × N3 |  |  |
| 56 + 8·(N1 + N2 + N3) | DF229 | Receiver firmware version counter | uint | 8 |  | N4 |
| 64 + 8·(N1 + N2 + N3) | DF230 | Receiver firmware version | char8 | 8 × N4 |  |  |
| 64 + 8·(N1 + N2 + N3 + N4) | DF231 | Receiver serial number counter | uint | 8 |  | N5 |
| 72 + 8·(N1 + N2 + N3 + N4) | DF232 | Receiver serial number | char8 | 8 × N5 |  |  |

* Five counted strings. The payload size is `24 + 8 + 8·5 + 8·(N1+N2+N3+N4+N5)` bits, that is `72 + 8·ΣN`. Strings of 11, 4, 7, 8 and 4 characters give a 43-byte payload and a 49-byte frame.
* The antenna string is what a rover would use to look up phase-centre offsets. A rover without an antenna calibration database receives the string but cannot act on it. [K]
* Castle: `receiver_antenna_descriptor` with fields `antenna`, `antenna_setup_id`, `antenna_serial`, `receiver_type`, `firmware`, `receiver_serial`.

### 5.4 Message 1230: GLONASS code-phase biases

GLONASS is FDMA, so receivers from different manufacturers have different inter-frequency code-phase biases. 1230 transmits the biases of the reference station so the rover can compensate. [K]

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF421 | GLONASS code-phase bias indicator | bit | 1 |  | see note below |
| 25 | DF001 | Reserved | bit | 3 |  | zero |
| 28 | DF422 | GLONASS FDMA signals mask | bit | 4 |  | MSB first: L1 C/A, L1 P, L2 C/A, L2 P |
| 32 | DF423 | GLONASS L1 C/A code-phase bias | int | 16 | 0.02 m | present if mask bit 0x08 |
| 48 | DF424 | GLONASS L1 P code-phase bias | int | 16 | 0.02 m | present if mask bit 0x04 |
| 64 | DF425 | GLONASS L2 C/A code-phase bias | int | 16 | 0.02 m | present if mask bit 0x02 |
| 80 | DF426 | GLONASS L2 P code-phase bias | int | 16 | 0.02 m | present if mask bit 0x01 |

* The mask bits select which biases follow. They appear in **transmission order** L1 C/A, L1 P, L2 C/A, L2 P, so the first value on the wire belongs to the most significant set mask bit. Castle's constants are `MASK_L1_CA = 0x08`, `MASK_L1_P = 0x04`, `MASK_L2_CA = 0x02`, `MASK_L2_P = 0x01`. [C]
* Payload size: `32 + 16·popcount(DF422)` bits. Four biases give 96 bits (12 bytes, 18-byte frame). [C]
* Resolution 0.02 m, range ±655.36 m.
* DF421 (indicator). Castle documents `true` as "the station's observations are aligned to GLONASS time". Kalmix reads the bit as "0 = default biases, 1 = explicit bias values follow". The sources disagree; see §13. Castle sets the bit from `code_phase_bias_indicator` and does not interpret it.
* Importance: a rover from another vendor may track GLONASS but exclude it from the RTK solution when 1230 is missing, without any parser error. Treat 1230 as required whenever GLONASS MSM is sent from a non-u-blox base. [K] [C]
* Kalmix numbers the fields DF423 (mask) and DF424-DF427 (values). The numbers of Castle and `pyrtcm` are DF422 (mask) and DF423-DF426, and are used here.


## 6. Legacy RTK observation messages (1001-1004, 1009-1012)

Each legacy message is one header followed by `n` satellite blocks (n = DF006 for GPS, DF035 for GLONASS). The four message numbers of a constellation share one structure and differ in which fields the satellite block carries:

| Offset from the L1-only number | Content | Ambiguity and CNR (extended) | L2 fields |
| --- | --- | --- | --- |
| +0 (1001, 1009) | L1 only | no | no |
| +1 (1002, 1010) | L1 only, extended | yes | no |
| +2 (1003, 1011) | L1 and L2 | no | yes |
| +3 (1004, 1012) | L1 and L2, extended | yes | yes |

Castle expresses this as `detail::rtk_observables_family<First, Type>` (`IS_EXTENDED = ((Type − First) & 1) != 0`, `HAS_L2 = (Type − First) >= 2`). Each message number is its own type; the aliases are `gps_rtk_l1_obs` (1001) ... `gps_rtk_l1_l2_obs_ext` (1004) and `glonass_rtk_l1_obs` (1009) ... `glonass_rtk_l1_l2_obs_ext` (1012).

### 6.1 GPS messages 1001-1004

Header (64 bits, identical for the four messages):

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF004 | GPS epoch time (TOW) | uint | 30 | 1 ms | 0..604 799 999 |
| 54 | DF005 | Synchronous GNSS message flag | bit | 1 |  | 1 = more messages follow for the same epoch (Castle `synchronous`) |
| 55 | DF006 | No. of GPS satellite signals processed | uint | 5 |  | 0..31, number of satellite blocks |
| 60 | DF007 | GPS divergence-free smoothing indicator | bit | 1 |  | 1 = divergence-free smoothing |
| 61 | DF008 | GPS smoothing interval | bit | 3 |  | see smoothing-interval table |

Satellite block, repeated DF006 times. The "Present in" column selects the fields; a field a message does not carry is not on the wire at all.

| DF | Field | Type | Bits | Resolution | Present in | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| DF009 | GPS satellite ID | uint | 6 |  | all | PRN 1..32; 40..58 denote SBAS in 10403.x [R] |
| DF010 | GPS L1 code indicator | bit | 1 |  | all | 0 = C/A, 1 = P(Y) direct |
| DF011 | GPS L1 pseudorange | uint | 24 | 0.02 m | all | modulo 299 792.458 m (1 light-ms) |
| DF012 | GPS L1 phaserange − L1 pseudorange | int | 20 | 0.0005 m | all | 0x80000 = invalid [R] |
| DF013 | GPS L1 lock time indicator | uint | 7 |  | all | 7-bit lock-time table |
| DF014 | GPS integer L1 pseudorange modulus ambiguity | uint | 8 | 299 792.458 m | 1002, 1004 |  |
| DF015 | GPS L1 CNR | uint | 8 | 0.25 dB-Hz | 1002, 1004 | 0 = not computed [R] |
| DF016 | GPS L2 code indicator | bit | 2 |  | 1003, 1004 | 0 = C/A, 1 = P(Y) direct, 2 = P(Y) cross-correlated, 3 = Z-tracking (Castle) |
| DF017 | GPS L2 − L1 pseudorange difference | int | 14 | 0.02 m | 1003, 1004 | 0x2000 = invalid [R] |
| DF018 | GPS L2 phaserange − L1 pseudorange | int | 20 | 0.0005 m | 1003, 1004 | 0x80000 = invalid [R] |
| DF019 | GPS L2 lock time indicator | uint | 7 |  | 1003, 1004 | 7-bit lock-time table |
| DF020 | GPS L2 CNR | uint | 8 | 0.25 dB-Hz | 1004 |  |

| Message | Header bits | Bits per satellite | Payload bits, n satellites | Largest (n = 31) |
| --- | --- | --- | --- | --- |
| 1001 | 64 | 58 | 64 + 58·n | 1862 bits = 233 bytes |
| 1002 | 64 | 74 | 64 + 74·n | 2358 bits = 295 bytes |
| 1003 | 64 | 101 | 64 + 101·n | 3195 bits = 400 bytes |
| 1004 | 64 | 125 | 64 + 125·n | 3939 bits = 493 bytes |

### 6.2 GLONASS messages 1009-1012

Header (61 bits):

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF034 | GLONASS epoch time (tk) | uint | 27 | 1 ms | ms of GLONASS day (UTC+3 h), 0..86 399 999 |
| 51 | DF005 | Synchronous GNSS message flag | bit | 1 |  | 1 = more messages follow for the same epoch |
| 52 | DF035 | No. of GLONASS satellite signals processed | uint | 5 |  | 0..31, number of satellite blocks |
| 57 | DF036 | GLONASS divergence-free smoothing indicator | bit | 1 |  | 1 = divergence-free smoothing |
| 58 | DF037 | GLONASS smoothing interval | bit | 3 |  | see smoothing-interval table |

Satellite block, repeated DF035 times:

| DF | Field | Type | Bits | Resolution | Present in | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| DF038 | GLONASS satellite ID (slot number) | uint | 6 |  | all | 1..24; 40..63 reserved for non-GLONASS |
| DF039 | GLONASS L1 code indicator | bit | 1 |  | all | 0 = C/A, 1 = P |
| DF040 | GLONASS satellite frequency channel number | uint | 5 |  | all | k + 7, valid 0..20 |
| DF041 | GLONASS L1 pseudorange | uint | 25 | 0.02 m | all | modulo 599 584.916 m (2 light-ms) |
| DF042 | GLONASS L1 phaserange − L1 pseudorange | int | 20 | 0.0005 m | all | 0x80000 = invalid [R] |
| DF043 | GLONASS L1 lock time indicator | uint | 7 |  | all | 7-bit lock-time table |
| DF044 | GLONASS integer L1 pseudorange modulus ambiguity | uint | 7 | 599 584.916 m | 1010, 1012 |  |
| DF045 | GLONASS L1 CNR | uint | 8 | 0.25 dB-Hz | 1010, 1012 |  |
| DF046 | GLONASS L2 code indicator | bit | 2 |  | 1011, 1012 | 0 = C/A, 1 = P (Castle) |
| DF047 | GLONASS L2 − L1 pseudorange difference | int | 14 | 0.02 m | 1011, 1012 | 0x2000 = invalid [R] |
| DF048 | GLONASS L2 phaserange − L1 pseudorange | int | 20 | 0.0005 m | 1011, 1012 | 0x80000 = invalid [R] |
| DF049 | GLONASS L2 lock time indicator | uint | 7 |  | 1011, 1012 | 7-bit lock-time table |
| DF050 | GLONASS L2 CNR | uint | 8 | 0.25 dB-Hz | 1012 |  |

| Message | Header bits | Bits per satellite | Payload bits, n satellites | Largest (n = 31) |
| --- | --- | --- | --- | --- |
| 1009 | 61 | 64 | 61 + 64·n | 2045 bits = 256 bytes |
| 1010 | 61 | 79 | 61 + 79·n | 2510 bits = 314 bytes |
| 1011 | 61 | 107 | 61 + 107·n | 3378 bits = 423 bytes |
| 1012 | 61 | 130 | 61 + 130·n | 4091 bits = 512 bytes |

### 6.3 Interpreting the fields

Pseudoranges are sent **modulo** one ambiguity unit so that they fit 24 or 25 bits. With `A = DF014 × 299 792.458 m` (GPS) or `A = DF044 × 599 584.916 m` (GLONASS), as gnss++ decodes GPS: [G]

```
P1       = A + DF011 × 0.02                  L1 pseudorange (m)
P2       = P1 + DF017 × 0.02                 L2 pseudorange (m)
L1_range = P1 + DF012 × 0.0005               L1 phase range in metres; cycles = L1_range / lambda_L1
L2_range = P1 + DF018 × 0.0005               L2 phase range in metres; cycles = L2_range / lambda_L2
CNR      = DF015 × 0.25 dB-Hz
```

* The phase range is always sent relative to the **L1 pseudorange**, never to its own pseudorange.
* Messages without the ambiguity field (1001, 1003, 1009, 1011) leave the integer ambiguity to the receiver, which resolves it from an approximate position. This is why 1002 and 1004 are called extended. [R]
* GLONASS wavelengths depend on the frequency channel, `k = DF040 − 7`: L1 `f = 1602 + 0.5625·k` MHz, L2 `f = 1246 + 0.4375·k` MHz. [K]
* The ambiguity unit of GLONASS is two light-milliseconds (599 584.916 m), which is why DF044 has 7 bits and DF041 has 25 bits. [C]
* The raw fields have symmetric ranges: DF012 and DF018 reach ±(2^19 − 1) × 0.0005 m = ±262.1 m, DF017 reaches ±(2^13 − 1) × 0.02 m = ±163.8 m.

Reference encoding of one GPS satellite (gnss++ `encodeObservations`, messages 1003/1004): [G]

```
A    = floor(P1 / 299792.458)                 must be 0..255
DF011 = round((P1 - A * 299792.458) / 0.02)   must be 0..0xFFFFFF
DF012 = round((L1_range - P1) / 0.0005)       must fit int(20)
DF017 = round((P2 - P1) / 0.02)               must fit int(14)
DF018 = round((L2_range - P1) / 0.0005)       must fit int(20)
```

A satellite that violates a range is **omitted** from the message, never clipped. gnss++ writes the lock indicator as 0 (loss of lock) or 127 (locked), and the CNR as `round(CNR_dBHz / 0.25)` clamped to 0..255.

### 6.4 Invalid markers [R]

| Field | Marker |
| --- | --- |
| DF012, DF042 | `0x80000` (−524 288) |
| DF018, DF048 | `0x80000` |
| DF017, DF047 | `0x2000` (−8192): L2 pseudorange not available |
| DF015, DF020, DF045, DF050 | 0: CNR not computed |

Castle does not write these markers on its own; the model holds raw integers and the caller sets them.

### 6.5 Lock time indicator (DF013, DF019, DF043, DF049) [R]

The 7-bit indicator maps to the **minimum** continuous lock time in seconds. The ranges join continuously (each range starts at the value the previous range ends on).

| Indicator i | Minimum lock time (s) | Step (s) |
| --- | --- | --- |
| 0 .. 23 | i | 1 |
| 24 .. 47 | 2·i − 24 | 2 |
| 48 .. 71 | 4·i − 120 | 4 |
| 72 .. 95 | 8·i − 408 | 8 |
| 96 .. 119 | 16·i − 1176 | 16 |
| 120 .. 126 | 32·i − 3096 | 32 |
| 127 | 937 (minimum) | — |

### 6.6 Smoothing interval (DF008, DF037, DF418) [R]

| Value | Meaning |
| --- | --- |
| 0 | no smoothing |
| 1 | < 30 s |
| 2 | 30 s to 1 min |
| 3 | 1 to 2 min |
| 4 | 2 to 4 min |
| 5 | 4 to 8 min |
| 6 | > 8 min |
| 7 | unlimited |

[K] confirms 0 = no smoothing, 1 = under 30 s and 7 = unlimited. The intermediate steps are recalled.

### 6.7 Limits and epoch handling

* The satellite count is a 5-bit field, so one message carries at most **31** satellites. Castle enforces this with `static_assert(MaxSatellites <= 31)`. A GPS constellation of 32 satellites therefore needs two messages for the same epoch.
* DF005 ("synchronous GNSS message flag") is 1 when another message with the **same epoch time** follows (for example the GLONASS message after the GPS one). It plays the same role as the MSM multiple-message bit DF393. [C]
* The GPS epoch is the GPS time of week in milliseconds (DF004). The GLONASS epoch is the time of day tk in milliseconds, UTC + 3 hours (DF034).
* Castle models keep all values raw (`l1_pseudorange`, `l1_phase_minus_pseudorange`, ...) and expose the resolutions as `PSEUDORANGE_RESOLUTION_M`, `PHASE_RESOLUTION_M`, `AMBIGUITY_M` and `CNR_RESOLUTION_DBHZ`.


## 7. Multiple Signal Messages (MSM)

An MSM carries the observations of **one constellation and one epoch**: every satellite, and every signal of each satellite, in a compact masked layout. The message number identifies the constellation (§4.1) and the **level** 1..7, which selects the observation content.

### 7.1 Levels

| Level | Pseudorange | Phase range | Lock time | Half-cycle | CNR | Phase-range rate | Resolution class |
| --- | --- | --- | --- | --- | --- | --- | --- |
| MSM1 | yes | no | no | no | no | no | standard |
| MSM2 | no | yes | yes | yes | no | no | standard |
| MSM3 | yes | yes | yes | yes | no | no | standard |
| MSM4 | yes | yes | yes | yes | yes | no | standard |
| MSM5 | yes | yes | yes | yes | yes | yes | standard |
| MSM6 | yes | yes | yes | yes | yes | no | high |
| MSM7 | yes | yes | yes | yes | yes | yes | high |

Level content follows [K]. Castle implements **MSM4, MSM5 and MSM7**, the levels a u-blox receiver accepts. The levels differ in which data fields exist ([K] Table 12, [P] payload tables):

| Field | DF | MSM1 | MSM2 | MSM3 | MSM4 | MSM5 | MSM6 | MSM7 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Satellite data** | | | | | | | | |
| Rough range, integer ms (`uint 8`) | DF397 | | | | x | x | x | x |
| Extended satellite information (`uint 4`) | DF419 | | | | | x | | x |
| Rough range modulo 1 ms (`uint 10`) | DF398 | x | x | x | x | x | x | x |
| Rough phase-range rate (`int 14`) | DF399 | | | | | x | | x |
| **Signal data (per cell)** | | | | | | | | |
| Fine pseudorange (`int 15`) | DF400 | x | | x | x | x | | |
| Fine pseudorange, high resolution (`int 20`) | DF405 | | | | | | x | x |
| Fine phase range (`int 22`) | DF401 | | x | x | x | x | | |
| Fine phase range, high resolution (`int 24`) | DF406 | | | | | | x | x |
| Lock time indicator (`uint 4`) | DF402 | | x | x | x | x | | |
| Lock time indicator, extended (`uint 10`) | DF407 | | | | | | x | x |
| Half-cycle ambiguity (`bit 1`) | DF420 | | x | x | x | x | x | x |
| CNR (`uint 6`) | DF403 | | | | x | x | | |
| CNR, high resolution (`uint 10`) | DF408 | | | | | | x | x |
| Fine phase-range rate (`int 15`) | DF404 | | | | | x | | x |

MSM1-3 send only the sub-millisecond part of the rough range ([K], [P]); the receiver supplies the integer milliseconds from an approximate position. [R]

### 7.2 Payload structure

```
+-----------------------------+----------------------------------+------------------------------------+
| header                      | satellite data (N_sat entries)   | signal data (N_cell entries)       |
| 169 bits + N_sat·N_sig bits | field-major arrays               | field-major arrays                 |
+-----------------------------+----------------------------------+------------------------------------+
```

The lengths of the two data segments follow from the three masks of the header. A decoder cannot allocate or index before it has read them.

### 7.3 Header

| Start bit | DF | Field | Type | Bits | Resolution | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | uint | 12 |  | message type, see catalogue |
| 12 | DF003 | Reference station ID | uint | 12 |  | 0..4095 |
| 24 | DF004 / DF248 / DF427 / DF428 | GNSS epoch time | uint | 30 | 1 ms | TOW: GPS and SBAS DF004, Galileo DF248, BeiDou DF427, QZSS DF428. GLONASS replaces the field with DF416 (3 bit) + DF034 (27 bit) |
| 54 | DF393 | Multiple message bit | bit | 1 |  | 1 = more MSM messages follow for this epoch |
| 55 | DF409 | Issue of data station (IODS) | uint | 3 |  |  |
| 58 | DF001 | Reserved | bit | 7 |  | zero |
| 65 | DF411 | Clock steering indicator | uint | 2 |  | 0 unknown, 1 not applied, 2 applied (Castle, Kalmix) |
| 67 | DF412 | External clock indicator | uint | 2 |  | 0 internal, 1 external locked, 2 external unlocked, 3 unknown (Kalmix) |
| 69 | DF417 | GNSS smoothing type indicator | bit | 1 |  | 1 = divergence-free smoothing |
| 70 | DF418 | GNSS smoothing interval | bit | 3 |  | see smoothing-interval table |
| 73 | DF394 | GNSS satellite mask | bit | 64 |  | bit for satellite ID n is the n-th bit, MSB first |
| 137 | DF395 | GNSS signal mask | bit | 32 |  | bit for signal ID n is the n-th bit, MSB first |
| 169 | DF396 | GNSS cell mask | bit | N_sat × N_sig |  | one bit per (satellite, signal), satellite-major, MSB first |

* The fixed part (DF002 to DF418) is 73 bits. With the 64-bit satellite mask and the 32-bit signal mask the cell mask starts at bit **169**. [K] [C]
* DF411 values: 0 unknown, 1 clock steering not applied, 2 applied (Castle documents 0-2; Kalmix lists 3 as a second "unknown").
* DF412: 0 internal clock, 1 external clock locked, 2 external clock not locked, 3 unknown. [K]
* DF417 = 1 means divergence-free smoothing; DF418 is the interval of §6.6. DF418 = 0 means no smoothing.
* DF409 (IODS) is incremented when the station parameters change.
* Castle's `msm_message` exposes these as `multiple_message`, `iods`, `clock_steering`, `external_clock`, `divergence_free` and `smoothing_interval`, and writes the 7 reserved bits as zero. gnss++ labels the same 7 bits "session transmit time" and skips them. [G]

### 7.4 Masks and cells

* **Satellite mask (DF394)**: 64 bits, MSB first. The `n`-th bit (n = 1..64) is 1 when satellite ID `n` is present. Castle: `satellite_bit(n) = 1 << (64 - n)`.
* **Signal mask (DF395)**: 32 bits, MSB first, signal ID 1..32. Castle: `signal_bit(n) = 1 << (32 - n)`.
* `N_sat = popcount(DF394)`, `N_sig = popcount(DF395)`.
* **Cell mask (DF396)**: `N_sat × N_sig` bits, **satellite-major**: the first `N_sig` bits belong to the first satellite, in signal order, then the next satellite. A set bit means that this satellite has an observation for that signal. `N_cell = popcount(DF396)`.
* `N_sat × N_sig ≤ 64` is a hard limit; a frame that violates it is invalid and a parser must reject it before allocating anything. [K] [G] [C]
* Castle: `cell_bit(satellite_index, signal_index, satellite_count, signal_count) = 1 << (N - 1 - (satellite_index × N_sig + signal_index))`, with `N = satellite_count × signal_count`. The indices count set mask bits, not IDs. It returns 0 for out-of-range indices.

Worked example (satellites 5, 12, 25; signals 2 and 16; satellite 12 has no signal 16):

```
DF394 = 0x0810008000000000     bits 5, 12 and 25 set       N_sat = 3
DF395 = 0x40010000             bits 2 and 16 set           N_sig = 2
cells = (5,2) (5,16) | (12,2) (12,16) | (25,2) (25,16)
DF396 =   1     1    |   1       0     |   1       1     =  111011   (N_sat × N_sig = 6 bits, N_cell = 5)
```

### 7.5 Satellite data

One entry per set bit of DF394, in ascending satellite ID order. The data are **field-major**: first the value of the first field for every satellite, then the second field for every satellite, and so on.

| Order | DF | Field | Type | Resolution | Present in | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | DF397 | Rough range, integer milliseconds | `uint(8)` | 1 ms (one light-ms) | MSM4-7 | 255 = invalid, the satellite is excluded. Largest valid value 254 ms, about 76 000 km. |
| 2 | DF419 | Extended satellite information | `uint(4)` | — | MSM5, MSM7 | GLONASS: frequency channel k + 7 (0..13), 15 = unknown. Other systems: reserved, zero. |
| 3 | DF398 | Rough range modulo 1 ms | `uint(10)` | 2^−10 ms | all | |
| 4 | DF399 | Rough phase-range rate | `int(14)` | 1 m/s | MSM5, MSM7 | −8192 (`0x2000`) = invalid. |

Bits per satellite: **10** (MSM1-3), **18** (MSM4, MSM6), **36** (MSM5, MSM7).

Castle's `msm_satellite` holds `rough_range_ms` (DF397), `extended_info`, `rough_range_mod` (DF398) and `rough_phase_range_rate` (DF399). Fields that the chosen level does not carry are ignored by `encode()`.

### 7.6 Signal data

One entry per set bit of DF396, in cell-mask order (satellite-major, ascending ID, signals in ascending ID within each satellite). Again **field-major**.

| Order | DF | Field | Type | Resolution | Present in |
| --- | --- | --- | --- | --- | --- |
| 1 | DF400 | Fine pseudorange | `int(15)` | 2^−24 ms | MSM1, 3, 4, 5 |
| 1 | DF405 | Fine pseudorange, high resolution | `int(20)` | 2^−29 ms | MSM6, 7 |
| 2 | DF401 | Fine phase range | `int(22)` | 2^−29 ms | MSM2, 3, 4, 5 |
| 2 | DF406 | Fine phase range, high resolution | `int(24)` | 2^−31 ms | MSM6, 7 |
| 3 | DF402 | Phase-range lock time indicator | `uint(4)` | table | MSM2-5 |
| 3 | DF407 | Lock time indicator, extended | `uint(10)` | table | MSM6, 7 |
| 4 | DF420 | Half-cycle ambiguity indicator | `bit(1)` | — | MSM2-7 |
| 5 | DF403 | CNR | `uint(6)` | 1 dB-Hz | MSM4, 5 |
| 5 | DF408 | CNR, high resolution | `uint(10)` | 2^−4 dB-Hz (0.0625) | MSM6, 7 |
| 6 | DF404 | Fine phase-range rate | `int(15)` | 0.0001 m/s | MSM5, 7 |

Bits per cell: **15** (MSM1), **27** (MSM2), **42** (MSM3), **48** (MSM4), **63** (MSM5), **65** (MSM6), **80** (MSM7).

Castle's `msm_signal` holds `fine_pseudorange`, `fine_phase_range`, `lock_time`, `half_cycle`, `cnr` and `fine_phase_range_rate`; the width of each is chosen by the level (`PSEUDORANGE_BITS`, `PHASE_RANGE_BITS`, `LOCK_TIME_BITS`, `CNR_BITS`).

### 7.7 Payload size

| Level | Satellite bits (S) | Cell bits (C) | Payload bits |
| --- | --- | --- | --- |
| MSM1 | 10 | 15 | 169 + N_sat·N_sig + 10·N_sat + 15·N_cell |
| MSM2 | 10 | 27 | 169 + N_sat·N_sig + 10·N_sat + 27·N_cell |
| MSM3 | 10 | 42 | 169 + N_sat·N_sig + 10·N_sat + 42·N_cell |
| MSM4 | 18 | 48 | 169 + N_sat·N_sig + 18·N_sat + 48·N_cell |
| MSM5 | 36 | 63 | 169 + N_sat·N_sig + 36·N_sat + 63·N_cell |
| MSM6 | 18 | 65 | 169 + N_sat·N_sig + 18·N_sat + 65·N_cell |
| MSM7 | 36 | 80 | 169 + N_sat·N_sig + 36·N_sat + 80·N_cell |

For the Appendix A.2 example (3 satellites, 2 signals, 5 cells): MSM4 is 469 bits, MSM5 598 bits and MSM7 683 bits. The payload is padded with zeros to 59, 75 and 86 bytes.

### 7.8 Scaling, invalid markers and reconstruction

With `c_ms = 299 792.458 m` (one light-millisecond):

```
rough_ms     = DF397 + DF398 × 2^-10                                   [ms]   (MSM1-3: DF398 only)
pseudorange  = (rough_ms + DF400 × 2^-24) × c_ms        MSM1,3,4,5     [m]
             = (rough_ms + DF405 × 2^-29) × c_ms        MSM6,7
phase_range  = (rough_ms + DF401 × 2^-29) × c_ms        MSM2-5         [m]
             = (rough_ms + DF406 × 2^-31) × c_ms        MSM6,7
carrier_phase = phase_range / wavelength                               [cycles]
range_rate   = DF399 + DF404 × 0.0001                   MSM5,7         [m/s]
doppler      = - range_rate / wavelength                               [Hz]
cnr          = DF403 [dB-Hz]   or   DF408 × 0.0625 [dB-Hz]
```

The fine fields are measured against the **quantised** rough range (`DF397 + DF398/1024`), not against the true range. [C] The Doppler sign follows from a range rate that is positive when the range grows. [G]

| Field | Bits | Invalid marker |
| --- | --- | --- |
| DF397 | 8 | 255 |
| DF399 | 14 | −8192 (`0x2000`) |
| DF400 | 15 | −16384 (`0x4000`) |
| DF401 | 22 | −2 097 152 (`0x200000`) |
| DF404 | 15 | −16384 (`0x4000`) |
| DF405 | 20 | −524 288 (`0x80000`) |
| DF406 | 24 | −8 388 608 (`0x800000`) |

The marker is the most negative value of the field, so a valid residual is limited to `−(2^(n−1) − 1) .. 2^(n−1) − 1`.

| Quantity | MSM4, MSM5 | MSM7 |
| --- | --- | --- |
| Pseudorange step | 2^−24 ms = 17.87 mm | 2^−29 ms = 0.558 mm |
| Pseudorange residual range | ±292.7 m | ±292.7 m |
| Phase-range step | 2^−29 ms = 0.558 mm | 2^−31 ms = 0.140 mm |
| Phase-range residual range | ±1170.9 m | ±1170.9 m |
| CNR | 0..63 dB-Hz, step 1 | 0..63.9375 dB-Hz, step 0.0625 |
| Fine range-rate step | 0.0001 m/s, ±1.6383 m/s | same |
| Rough range step | 2^−10 ms = 292.8 m | same |

An encoder must store the invalid marker when a residual does not fit its field. Castle's `quantise_residual()` returns the invalid marker when the scaled value reaches or exceeds the field limit, and gnss++ leaves the cell at its invalid default for `|residual| > 292.7 m` (pseudorange) or `> 1171 m` (phase). [C] [G]

### 7.9 Epoch time

The 30-bit epoch field (27 bits plus a 3-bit day for GLONASS) carries milliseconds:

| System | Field | Time scale |
| --- | --- | --- |
| GPS | DF004, 30 bits | GPS time of week, 0..604 799 999 ms |
| GLONASS | DF416 (3 bits, day of week 0 = Sunday .. 6, 7 = unknown) and DF034 (27 bits, ms of day) | GLONASS time = UTC + 3 h (UTC(SU)) |
| Galileo | DF248, 30 bits | Galileo System Time of week; GST week + 1024 equals the GPS week [G] |
| SBAS | DF004, 30 bits | GPS time of week [K] |
| QZSS | DF428, 30 bits | GPS time of week [K] |
| BeiDou | DF427, 30 bits | BeiDou Time of week; **BDT = GPST − 14 s** [G]; BDT week 0 begins in GPS week 1356 [G] |
| NavIC | 30-bit epoch field (DF number not confirmed by the sources) | not confirmed; gnss++ decodes it as GPS time of week [K] [G] |

Conversions used by gnss++, which Castle leaves to the caller (`set_glonass_epoch(day, ms)` packs `day << 27 | ms`): [G]

```
GLONASS:  utc = gpst - leap_seconds;  glot = utc + 3 h;
          day = floor(glot_tow / 86400) mod 7;  ms_of_day = round((glot_tow - day × 86400) × 1000)
          epoch = (day << 27) | (ms_of_day & 0x7FFFFFF)
BeiDou:   epoch = (gpst_tow - 14 s) in ms, wrapped to one week
```

`leap_seconds` is the GPST − UTC offset (18 s since 2017-01-01 in the gnss++ table). To turn a decoded epoch into an absolute time the receiver picks the week for which the time lies within half a week (302 400 s) of a reference time. [G]

### 7.10 Satellite IDs

The satellite mask bit `n` selects the satellite below. [P] [G]

| System | ID range | Satellite |
| --- | --- | --- |
| GPS | 1..63 (1..32 used) | PRN `n` |
| GLONASS | 1..24 | slot number `n` |
| Galileo | 1..50 (+ 51, 52 for GIOVE-A/B) | satellite `n` |
| SBAS | 1..39 | PRN `n + 119` |
| QZSS | 1..10 | PRN `n + 192` |
| BeiDou | 1..63 | PRN `n` |
| NavIC | 1..14 | PRN `n` |

### 7.11 Signal IDs

The signal mask bit `n` selects the signal below; the RINEX code is the 2-character observation attribute (`1C` = band 1, tracking code C). The tables are the RTCM 10403.3 signal maps as transcribed by [P] and cross-checked against the tables of gnss++ [G]. IDs that do not appear are reserved.

**GPS**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 2 | 1C | L1 |  |
| 3 | 1P | L1 | L1 P |
| 4 | 1W | L1 | L1 P(Y) semi-codeless |
| 8 | 2C | L2 | L2 C/A |
| 9 | 2P | L2 | L2 P |
| 10 | 2W | L2 | L2 P(Y) semi-codeless |
| 15 | 2S | L2 | L2C(M) |
| 16 | 2L | L2 | L2C(L) |
| 17 | 2X | L2 | L2C(M+L) |
| 22 | 5I | L5 | L5 I |
| 23 | 5Q | L5 | L5 Q |
| 24 | 5X | L5 | L5 I+Q |
| 30 | 1S | L1 | L1C data |
| 31 | 1L | L1 | L1C pilot |
| 32 | 1X | L1 | L1C data+pilot |

**GLONASS**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 2 | 1C | G1 |  |
| 3 | 1P | G1 |  |
| 8 | 2C | G2 |  |
| 9 | 2P | G2 |  |

**Galileo**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 2 | 1C | E1 |  |
| 3 | 1A | E1 |  |
| 4 | 1B | E1 |  |
| 5 | 1X | E1 |  |
| 6 | 1Z | E1 |  |
| 8 | 6C | E6 |  |
| 9 | 6A | E6 |  |
| 10 | 6B | E6 |  |
| 11 | 6X | E6 |  |
| 12 | 6Z | E6 |  |
| 14 | 7I | E5B |  |
| 15 | 7Q | E5B |  |
| 16 | 7X | E5B |  |
| 18 | 8I | E5AB | E5 AltBOC |
| 19 | 8Q | E5AB | E5 AltBOC |
| 20 | 8X | E5AB | E5 AltBOC |
| 22 | 5I | E5A |  |
| 23 | 5Q | E5A |  |
| 24 | 5X | E5A |  |

**SBAS**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 2 | 1C | L1 |  |
| 22 | 5I | L5 |  |
| 23 | 5Q | L5 |  |
| 24 | 5X | L5 |  |

**QZSS**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 2 | 1C | L1 |  |
| 9 | 6S | LEX | L6 |
| 10 | 6L | LEX | L6 |
| 11 | 6X | LEX | L6 |
| 15 | 2S | L2 |  |
| 16 | 2L | L2 |  |
| 17 | 2X | L2 |  |
| 22 | 5I | L5 |  |
| 23 | 5Q | L5 |  |
| 24 | 5X | L5 |  |
| 30 | 1S | L1 |  |
| 31 | 1L | L1 |  |
| 32 | 1X | L1 |  |

**BeiDou**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 2 | 2I | B1 |  |
| 3 | 2Q | B1 |  |
| 4 | 2X | B1 |  |
| 8 | 6I | B3 |  |
| 9 | 6Q | B3 |  |
| 10 | 6X | B3 |  |
| 14 | 7I | B2 |  |
| 15 | 7Q | B2 |  |
| 16 | 7X | B2 |  |
| 22 | 5D | B2A |  |
| 23 | 5P | B2A |  |
| 24 | 5X | B2A |  |
| 25 | 7D | B2B | B2b |
| 30 | 1D | B1C | B1C data |
| 31 | 1P | B1C | B1C pilot |
| 32 | 1X | B1C | B1C data+pilot |

**NavIC**

| Signal ID | RINEX code | Band | Notes |
| --- | --- | --- | --- |
| 22 | 5A | L5 |  |

The Castle samples use GPS IDs 2 (L1 C/A) and 16 (L2C L), GLONASS 2 and 8, Galileo 2 (E1 C) and 14 (E5b I), BeiDou 2 (B1I) and 14 (B2I). [C]

Notes:

* The Kalmix tables list only a subset: GPS without 2C/2P/2W (IDs 8-10) and BeiDou without B1C, B2a and B2b. [K] The complete maps above are the ones used by both decoders.
* Several IDs of one satellite can map to the same physical band (for example GPS 2C, 2W, 2S, 2L and 2X are all L2). A decoder that offers one observation per band ranks them; gnss++ uses the RINEX tracking-attribute priority and keeps the lowest rank, the lower signal ID winning a tie. [G]
* GLONASS wavelengths depend on the satellite's frequency channel (§6.3), so a GLONASS cell can be converted to cycles only when DF419 or an earlier message gave the channel. [G]

### 7.12 Lock time, half-cycle and CNR

**DF402 (4 bits)** gives a minimum lock time of `2^(i + 4)` ms for `i = 1..15` and "below 32 ms" for `i = 0`: [R]

| Indicator i | Minimum lock time (ms) |
| --- | --- |
| 0 | 0 (below 32 ms) |
| 1 | 32 |
| 2 | 64 |
| 3 | 128 |
| 4 | 256 |
| 5 | 512 |
| 6 | 1024 |
| 7 | 2048 |
| 8 | 4096 |
| 9 | 8192 |
| 10 | 16384 |
| 11 | 32768 |
| 12 | 65536 |
| 13 | 131072 |
| 14 | 262144 |
| 15 | 524288 |

**DF407 (10 bits)** is piecewise linear and continuous: a minimum lock time of `i` ms up to `i = 63`, then `2^j × (i − 32 j)` ms in blocks of 32 values with `j = floor(i / 32) − 1`, saturating at `i = 704` and above with 2^26 ms (about 18.6 h): [R]

| Indicator i | Minimum lock time (ms) | Step (ms) |
| --- | --- | --- |
| 0 .. 63 | i | 1 |
| 64 .. 95 | 2·i − 64 | 2 |
| 96 .. 127 | 4·i − 256 | 4 |
| 128 .. 159 | 8·i − 768 | 8 |
| 160 .. 191 | 16·i − 2048 | 16 |
| 192 .. 223 | 32·i − 5120 | 32 |
| 224 .. 255 | 64·i − 12288 | 64 |
| 256 .. 287 | 128·i − 28672 | 128 |
| 288 .. 319 | 256·i − 65536 | 256 |
| 320 .. 351 | 512·i − 147456 | 512 |
| 352 .. 383 | 1024·i − 327680 | 1024 |
| 384 .. 415 | 2048·i − 720896 | 2048 |
| 416 .. 447 | 4096·i − 1572864 | 4096 |
| 448 .. 479 | 8192·i − 3407872 | 8192 |
| 480 .. 511 | 16384·i − 7340032 | 16384 |
| 512 .. 543 | 32768·i − 15728640 | 32768 |
| 544 .. 575 | 65536·i − 33554432 | 65536 |
| 576 .. 607 | 131072·i − 71303168 | 131072 |
| 608 .. 639 | 262144·i − 150994944 | 262144 |
| 640 .. 671 | 524288·i − 318767104 | 524288 |
| 672 .. 703 | 1048576·i − 671088640 | 1048576 |
| 704 .. 1023 | 67 108 864 (minimum) | — |

* Kalmix describes DF402 as "0 = < 24 ms to 15 = ≥ 524 s"; the lower bound of 24 ms disagrees with the 32 ms boundary above, the upper bound (524 288 ms) agrees. [K]
* **DF420** = 1 means a half-cycle ambiguity is present (unresolved), 0 means resolved. An unresolved half-cycle degrades RTK fixing. [K]
* **CNR** is 0 when not computed. [R] Castle's `set_cnr()` clamps to the field range and maps NaN to 0.
* gnss++ reduces the lock time to "lost" (`0`) versus "locked" (4-bit 15, 10-bit 1023) when it encodes, and treats lock 0 as loss of lock when it decodes. [G]

### 7.13 Limits and worst case

* `N_sat ≤ 64`, `N_sig ≤ 32`, `N_sat × N_sig ≤ 64`, therefore `N_cell ≤ 64`.
* The largest legal MSM7 payload is 7657 bits (958 bytes), so an MSM always fits the 1023-byte frame limit.
* A larger epoch must be split into several messages of the same constellation (different signal subsets), all but the last with DF393 = 1.

### 7.14 Multiple messages per epoch

* One message per constellation per epoch. DF393 is 1 on every MSM of the epoch **except the last**, so that a receiver can wait for the complete epoch before it differences. [C] [K]
* The legacy equivalent is DF005 (§6.7).
* DF409 (IODS) lets a receiver detect that the station configuration changed between messages.

### 7.15 Castle models

`msm_message<System, Level, MaxSatellites = 64, MaxCells = 64>` with `Level` 4, 5 or 7; `MESSAGE_TYPE = System + Level`. Aliases: `gps_msm4/5/7`, `glonass_msm4/5/7`, `galileo_msm4/5/7`, `beidou_msm4/5/7`. `encode()` latches `invalid_argument` when `N_sat × N_sig > 64`, when `satellites.size() != popcount(DF394)`, or when `signals.size() != popcount(DF396)`. A cell mask with bits above `N_sat × N_sig` is rejected by the bit writer as `out_of_range`. See [`msm.md`](messages/msm.md).


## 8. Broadcast ephemeris messages (1019, 1020, 1042, 1044, 1045, 1046)

These messages relay the navigation message a reference receiver decoded from a satellite. A rover normally obtains the same data from the sky, so the messages mainly shorten the time to first fix and make raw logs self-contained for post-processing. [K] Castle has no model for them; the layouts below are taken from the gnss++ decoder and encoder and checked against the field tables of [P]. The total lengths agree: 488, 360, 511, 485, 496 and 504 bits.

Conventions common to all six:

* Angles and rates of angles are in **semicircles** (1 semicircle = π rad) and semicircles/s. Multiply by π for radians. [G]
* `toc` and `toe` are times of week in seconds, `raw × step` (step in the "Resolution" column).
* Orbit and clock terms keep the usual names: `Crs`, `Crc` (m), `Cuc`, `Cus`, `Cic`, `Cis` (rad), `Δn` (semicircle/s), `M0`, `Ω0`, `i0`, `ω` (semicircle), `ΩDOT`, `IDOT` (semicircle/s), `A½` (m^½), `e` (dimensionless).
* The decoder rejects a payload shorter than the total below, and a satellite ID of zero or above the limit stated per system. [G]

### 8.1 Message 1019: GPS

| Start bit | DF | Field | Type | Bits | Resolution / unit |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message Number | uint | 12 |  |
| 12 | DF009 | GPS Satellite ID | uint | 6 |  |
| 18 | DF076 | GPS Week number | uint | 10 |  |
| 28 | DF077 | GPS SV Acc. (URA) | bit | 4 |  |
| 32 | DF078 | GPS CODE ON L2 | bit | 2 |  |
| 34 | DF079 | GPS IDOT (Issue of Data, Time) | int | 14 | 2^-43 semicircle/s |
| 48 | DF071 | GPS IODE | uint | 8 |  |
| 56 | DF081 | GPS toc (Reference Time, Clock) | uint | 16 | 16 s |
| 72 | DF082 | GPS af2 (Clock correction drift rate) | int | 8 | 2^-55 s/s² |
| 80 | DF083 | GPS af1 (Clock correction drift) | int | 16 | 2^-43 s/s |
| 96 | DF084 | GPS af0 (Clock correction bias) | int | 22 | 2^-31 s |
| 118 | DF085 | GPS IODC (Issue of Data, Clock) | uint | 10 |  |
| 128 | DF086 | GPS Crs (Amplitude of sine harmonic correction term to the orbit, radius) | int | 16 | 2^-5 m |
| 144 | DF087 | GPS ∆n (Mean motion difference from computed value) | int | 16 | 2^-43 semicircle/s |
| 160 | DF088 | GPS M0 (Mean Anomaly) | int | 32 | 2^-31 semicircle |
| 192 | DF089 | GPS Cuc (Amplitude of cosine harmonic correction term to argument of latitude) | int | 16 | 2^-29 rad |
| 208 | DF090 | GPS e (Eccentricity) | uint | 32 | 2^-33 |
| 240 | DF091 | GPS Cus (Amplitude of sine harmonic correction term to argument of latitude) | int | 16 | 2^-29 rad |
| 256 | DF092 | GPS A½ (Square root of Semi-major Axis) | uint | 32 | 2^-19 m^½ |
| 288 | DF093 | GPS toe (Reference Time, Ephemeris) | uint | 16 | 16 s |
| 304 | DF094 | GPS Cic (Amplitude of cosine harmonic correction term to angle of inclination) | int | 16 | 2^-29 rad |
| 320 | DF095 | GPS Ω0 (Longitude of Ascending Node) | int | 32 | 2^-31 semicircle |
| 352 | DF096 | GPS Cis (Amplitude of sine harmonic correction term to angle of inclination) | int | 16 | 2^-29 rad |
| 368 | DF097 | GPS i0 (Inclination) | int | 32 | 2^-31 semicircle |
| 400 | DF098 | GPS Crc (Amplitude of cosine harmonic correction term to orbit,  radius) | int | 16 | 2^-5 m |
| 416 | DF099 | GPS ω (Argument of Perigee) | int | 32 | 2^-31 semicircle |
| 448 | DF100 | GPS ΩDOT (Rate of Change of Right Ascension) | int | 24 | 2^-43 semicircle/s |
| 472 | DF101 | GPS tGD | int | 8 | 2^-31 s |
| 480 | DF102 | GPS SV HEALTH | uint | 6 |  |
| 486 | DF103 | GPS L2 P data flag | bit | 1 |  |
| 487 | DF137 | GPS Fit Interval | bit | 1 |  |
|  |  | **Total** |  | 488 | 61 bytes with zero padding |

* Payload 488 bits = 61 bytes. [G]
* DF076 is the week modulo 1024. gnss++ expands it with `week = w + 1024 × floor((ref_week − w + 512) / 1024)` using a reference week. [G]
* DF077 is the URA index. gnss++ maps it to metres with the table below; index 15 means "no accuracy prediction available". [G]

| URA index | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| URA (m) | 2.4 | 3.4 | 4.85 | 6.85 | 9.65 | 13.65 | 24 | 48 | 96 | 192 | 384 | 768 | 1536 | 3072 | 6144 |

* gnss++ accepts satellite IDs 1..32 and ignores DF078 (code on L2), DF103 and DF137 on decode; it writes them as zero on encode.
* DF081 (`toc`) and DF093 (`toe`) have 16 bits and 16 s steps. gnss++ refuses to encode a `toe` or `toc` above `0xFFFF` steps.

### 8.2 Message 1020: GLONASS

| Start bit | DF | Field | Type | Bits | Resolution / unit |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message Number | uint | 12 |  |
| 12 | DF038 | GLONASS Satellite ID (Satellite Slot Number) | uint | 6 |  |
| 18 | DF040 | GLONASS Satellite Frequency Channel Number | uint | 5 |  |
| 23 | DF104 | GLONASS almanac health | bit | 1 |  |
| 24 | DF105 | GLONASS almanac health availability indicator | bit | 1 |  |
| 25 | DF106 | GLONASS P1 | bit | 2 |  |
| 27 | DF107 | GLONASS tk | bit | 12 |  |
| 39 | DF108 | GLONASS MSB of Bn word | bit | 1 |  |
| 40 | DF109 | GLONASS P2 | bit | 1 |  |
| 41 | DF110 | GLONASS tb | uint | 7 | 900 s |
| 48 | DF111 | GLONASS xn(tb), first derivative | intS | 24 | 2^-20 km/s |
| 72 | DF112 | GLONASS xn(tb) | intS | 27 | 2^-11 km |
| 99 | DF113 | GLONASS xn(tb), second derivative | intS | 5 | 2^-30 km/s² |
| 104 | DF114 | GLONASS yn(tb), first derivative | intS | 24 | 2^-20 km/s |
| 128 | DF115 | GLONASS yn(tb) | intS | 27 | 2^-11 km |
| 155 | DF116 | GLONASS yn(tb), second derivative | intS | 5 | 2^-30 km/s² |
| 160 | DF117 | GLONASS zn(tb), first derivative | intS | 24 | 2^-20 km/s |
| 184 | DF118 | GLONASS zn(tb) | intS | 27 | 2^-11 km |
| 211 | DF119 | GLONASS zn(tb), second derivative | intS | 5 | 2^-30 km/s² |
| 216 | DF120 | GLONASS P3 | bit | 1 |  |
| 217 | DF121 | GLONASS γn(tb) (Relative deviation) | intS | 11 | 2^-40 |
| 228 | DF122 | GLONASS-M P | bit | 2 |  |
| 230 | DF123 | GLONASS-M ln (third string) | bit | 1 |  |
| 231 | DF124 | GLONASS τn (tb) | intS | 22 | 2^-30 s |
| 253 | DF125 | GLONASS-M Δτn | intS | 5 |  |
| 258 | DF126 | GLONASS En | uint | 5 |  |
| 263 | DF127 | GLONASS-M P4 | bit | 1 |  |
| 264 | DF128 | GLONASS-M FT | uint | 4 |  |
| 268 | DF129 | GLONASS-M NT | uint | 11 |  |
| 279 | DF130 | GLONASS-M M | bit | 2 |  |
| 281 | DF131 | GLONASS The Availability of Additional Data | bit | 1 |  |
| 282 | DF132 | GLONASS NA | uint | 11 |  |
| 293 | DF133 | GLONASS τc | intS | 32 |  |
| 325 | DF134 | GLONASS-M N4 | uint | 5 |  |
| 330 | DF135 | GLONASS-M τGPS | intS | 22 |  |
| 352 | DF136 | GLONASS-M ln (fifth string) | bit | 1 |  |
| 353 | — | Reserved | bit | 7 |  |
|  |  | **Total** |  | 360 | 45 bytes with zero padding |

* Payload 360 bits = 45 bytes. [G]
* DF040 is the frequency channel number plus 7: `k = DF040 − 7`.
* DF107 `tk` is the frame start time: 5 bits hours, 6 bits minutes, 1 bit for 30 s (`seconds = 30 × bit`), in Moscow time (UTC + 3 h). DF110 `tb` is the reference time in steps of 15 minutes (900 s). Both are converted to GPS time with a leap-second table and aligned to the nearest day of the receiver's reference time. [G]
* Fields typed `intS` are **sign-magnitude**. gnss++ scales them as: position `× 2^−11` km, velocity `× 2^−20` km/s, acceleration `× 2^−30` km/s², `γn × 2^−40`, `τn × 2^−30` s. [G]
* DF108 `Bn` is used as the health flag. The remaining GLONASS-M fields (DF122 onward) are skipped by gnss++ on decode and written as zero on encode, except `En` (DF126, age in days).
* gnss++ accepts satellite slots 1..32 on decode.

### 8.3 Message 1042: BeiDou

| Start bit | DF | Field | Type | Bits | Resolution / unit |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message Number | uint | 12 |  |
| 12 | DF488 | BDS Satellite ID | uint | 6 |  |
| 18 | DF489 | BDS Week Number | uint | 13 |  |
| 31 | DF490 | BDS URAI | bit | 4 |  |
| 35 | DF491 | BDS IDOT | int | 14 | 2^-43 semicircle/s |
| 49 | DF492 | BDS AODE | uint | 5 |  |
| 54 | DF493 | BDS Toc | uint | 17 | 8 s |
| 71 | DF494 | BDS a2 | int | 11 | 2^-66 s/s² |
| 82 | DF495 | BDS a1 | int | 22 | 2^-50 s/s |
| 104 | DF496 | BDS a0 | int | 24 | 2^-33 s |
| 128 | DF497 | BDS AODC | uint | 5 |  |
| 133 | DF498 | BDS Crs | int | 18 | 2^-6 m |
| 151 | DF499 | BDS ∆n | int | 16 | 2^-43 semicircle/s |
| 167 | DF500 | BDS M0 | int | 32 | 2^-31 semicircle |
| 199 | DF501 | BDS Cuc | int | 18 | 2^-31 rad |
| 217 | DF502 | BDS e (Eccentricity) | uint | 32 | 2^-33 |
| 249 | DF503 | BDS Cus | int | 18 | 2^-31 rad |
| 267 | DF504 | BDS A½ | uint | 32 | 2^-19 m^½ |
| 299 | DF505 | BDS Toe | uint | 17 | 8 s |
| 316 | DF506 | BDS Cic | int | 18 | 2^-31 rad |
| 334 | DF507 | BDS Ω0 | int | 32 | 2^-31 semicircle |
| 366 | DF508 | BDS Cis | int | 18 | 2^-31 rad |
| 384 | DF509 | BDS i0 | int | 32 | 2^-31 semicircle |
| 416 | DF510 | BDS Crc | int | 18 | 2^-6 m |
| 434 | DF511 | BDS ω (Argument of Perigee) | int | 32 | 2^-31 semicircle |
| 466 | DF512 | BDS ΩDOT (Rate of Right Ascension) | int | 24 | 2^-43 semicircle/s |
| 490 | DF513 | BDS TGD1 | int | 10 | 0.1 ns |
| 500 | DF514 | BDS TGD2 | int | 10 | 0.1 ns |
| 510 | DF515 | BDS SV health | bit | 1 |  |
|  |  | **Total** |  | 511 | 64 bytes with zero padding |

* Payload 511 bits = 64 bytes (the last byte has one padding bit). [G]
* `toc` and `toe` use 8 s steps. The week (DF489) is the **BDT week**; gnss++ converts to the GPS week as `bdt_week + 1356`, then adds 14 s (`BDT = GPST − 14 s`). [G]
* TGD1 and TGD2 are in steps of 0.1 ns. [P] gnss++ scales them by 1e−10 s.
* Satellite IDs 1..63 are accepted. [G]

### 8.4 Message 1044: QZSS

| Start bit | DF | Field | Type | Bits | Resolution / unit |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message Number | uint | 12 |  |
| 12 | DF429 | QZSS Satellite ID | uint | 4 |  |
| 16 | DF430 | QZSS toc | uint | 16 | 16 s |
| 32 | DF431 | QZSS af2 | int | 8 | 2^-55 s/s² |
| 40 | DF432 | QZSS af1 | int | 16 | 2^-43 s/s |
| 56 | DF433 | QZSS af0 | int | 22 | 2^-31 s |
| 78 | DF434 | QZSS IODE | uint | 8 |  |
| 86 | DF435 | QZSS Crs | int | 16 | 2^-5 m |
| 102 | DF436 | QZSS ∆n | int | 16 | 2^-43 semicircle/s |
| 118 | DF437 | QZSS M0 | int | 32 | 2^-31 semicircle |
| 150 | DF438 | QZSS Cuc | int | 16 | 2^-29 rad |
| 166 | DF439 | QZSS e | uint | 32 | 2^-33 |
| 198 | DF440 | QZSS Cus | int | 16 | 2^-29 rad |
| 214 | DF441 | QZSS A½ | uint | 32 | 2^-19 m^½ |
| 246 | DF442 | QZSS toe | uint | 16 | 16 s |
| 262 | DF443 | QZSS Cic | int | 16 | 2^-29 rad |
| 278 | DF444 | QZSS Ω0 | int | 32 | 2^-31 semicircle |
| 310 | DF445 | QZSS Cis | int | 16 | 2^-29 rad |
| 326 | DF446 | QZSS i0 | int | 32 | 2^-31 semicircle |
| 358 | DF447 | QZSS Crc | int | 16 | 2^-5 m |
| 374 | DF448 | QZSS ω (Argument of Perigee) | int | 32 | 2^-31 semicircle |
| 406 | DF449 | QZSS ΩDOT (Rate of Right Ascension) | int | 24 | 2^-43 semicircle/s |
| 430 | DF450 | QZSS i0-DOT | int | 14 | 2^-43 semicircle/s |
| 444 | DF451 | QZSS Codes on L2 Channel | bit | 2 |  |
| 446 | DF452 | QZSS Week Number | uint | 10 |  |
| 456 | DF453 | QZSS URA | uint | 4 |  |
| 460 | DF454 | QZSS SV health | uint | 6 |  |
| 466 | DF455 | QZSS TGD | int | 8 | 2^-31 s |
| 474 | DF456 | QZSS IODC | uint | 10 |  |
| 484 | DF457 | QZSS Fit Interval | bit | 1 |  |
|  |  | **Total** |  | 485 | 61 bytes with zero padding |

* Payload 485 bits = 61 bytes. [G]
* The 4-bit satellite ID counts from the first QZSS PRN: ID 1 is PRN 193 ("J01"); gnss++ stores 1..10 and accepts 1..10. [G]
* DF452 is the week modulo 1024 (expanded as for GPS). DF457 (fit interval) is skipped.

### 8.5 Messages 1045 (Galileo F/NAV) and 1046 (Galileo I/NAV)

Message 1045:

| Start bit | DF | Field | Type | Bits | Resolution / unit |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message Number | uint | 12 |  |
| 12 | DF252 | Galileo Satellite ID | uint | 6 |  |
| 18 | DF289 | Galileo Week Number | uint | 12 |  |
| 30 | DF290 | Galileo IODnav | uint | 10 |  |
| 40 | DF291 | Galileo SV SISA | bit | 8 |  |
| 48 | DF292 | Galileo Rate of Inclination (IDOT) | int | 14 | 2^-43 semicircle/s |
| 62 | DF293 | Galileo toc | uint | 14 | 60 s |
| 76 | DF294 | Galileo af2 | int | 6 | 2^-59 s/s² |
| 82 | DF295 | Galileo af1 | int | 21 | 2^-46 s/s |
| 103 | DF296 | Galileo af0 | int | 31 | 2^-34 s |
| 134 | DF297 | Galileo Crs | int | 16 | 2^-5 m |
| 150 | DF298 | Galileo ∆n | int | 16 | 2^-43 semicircle/s |
| 166 | DF299 | Galileo M0 | int | 32 | 2^-31 semicircle |
| 198 | DF300 | Galileo Cuc | int | 16 | 2^-29 rad |
| 214 | DF301 | Galileo Eccentricity (e) | uint | 32 | 2^-33 |
| 246 | DF302 | Galileo Cus | int | 16 | 2^-29 rad |
| 262 | DF303 | Galileo A½ | uint | 32 | 2^-19 m^½ |
| 294 | DF304 | Galileo toe | uint | 14 | 60 s |
| 308 | DF305 | Galileo Cic | int | 16 | 2^-29 rad |
| 324 | DF306 | Galileo Ω0 | int | 32 | 2^-31 semicircle |
| 356 | DF307 | Galileo Cis | int | 16 | 2^-29 rad |
| 372 | DF308 | Galileo i0 | int | 32 | 2^-31 semicircle |
| 404 | DF309 | Galileo Crc | int | 16 | 2^-5 m |
| 420 | DF310 | Galileo ω (Argument of Perigee) | int | 32 | 2^-31 semicircle |
| 452 | DF311 | Galileo ΩDOT | int | 24 | 2^-43 semicircle/s |
| 476 | DF312 | Galileo BGD (E1/E5a) | int | 10 | 2^-32 s |
| 486 | DF314 | Galileo E5a Signal Health Status | bit | 2 |  |
| 488 | DF315 | Galileo E5a Data Validity Status | bit | 1 |  |
| 489 | — | Reserved | bit | 7 |  |
|  |  | **Total** |  | 496 | 62 bytes with zero padding |

Message 1046:

| Start bit | DF | Field | Type | Bits | Resolution / unit |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message Number | uint | 12 |  |
| 12 | DF252 | Galileo Satellite ID | uint | 6 |  |
| 18 | DF289 | Galileo Week Number | uint | 12 |  |
| 30 | DF290 | Galileo IODnav | uint | 10 |  |
| 40 | DF286 | Galileo SISA (E1, E5b) | bit | 8 |  |
| 48 | DF292 | Galileo Rate of Inclination (IDOT) | int | 14 | 2^-43 semicircle/s |
| 62 | DF293 | Galileo toc | uint | 14 | 60 s |
| 76 | DF294 | Galileo af2 | int | 6 | 2^-59 s/s² |
| 82 | DF295 | Galileo af1 | int | 21 | 2^-46 s/s |
| 103 | DF296 | Galileo af0 | int | 31 | 2^-34 s |
| 134 | DF297 | Galileo Crs | int | 16 | 2^-5 m |
| 150 | DF298 | Galileo ∆n | int | 16 | 2^-43 semicircle/s |
| 166 | DF299 | Galileo M0 | int | 32 | 2^-31 semicircle |
| 198 | DF300 | Galileo Cuc | int | 16 | 2^-29 rad |
| 214 | DF301 | Galileo Eccentricity (e) | uint | 32 | 2^-33 |
| 246 | DF302 | Galileo Cus | int | 16 | 2^-29 rad |
| 262 | DF303 | Galileo A½ | uint | 32 | 2^-19 m^½ |
| 294 | DF304 | Galileo toe | uint | 14 | 60 s |
| 308 | DF305 | Galileo Cic | int | 16 | 2^-29 rad |
| 324 | DF306 | Galileo Ω0 | int | 32 | 2^-31 semicircle |
| 356 | DF307 | Galileo Cis | int | 16 | 2^-29 rad |
| 372 | DF308 | Galileo i0 | int | 32 | 2^-31 semicircle |
| 404 | DF309 | Galileo Crc | int | 16 | 2^-5 m |
| 420 | DF310 | Galileo ω (Argument of Perigee) | int | 32 | 2^-31 semicircle |
| 452 | DF311 | Galileo ΩDOT | int | 24 | 2^-43 semicircle/s |
| 476 | DF312 | Galileo BGD (E1/E5a) | int | 10 | 2^-32 s |
| 486 | DF313 | Galileo BGD E5b/E1 | int | 10 | 2^-32 s |
| 496 | DF316 | Galileo E5b signal health status (E5b HS) | bit | 2 |  |
| 498 | DF317 | Galileo E5b data validity status (E5b DVS) | bit | 1 |  |
| 499 | DF287 | Galileo E1-B Signal Health Status | bit | 2 |  |
| 501 | DF288 | Galileo E1-B Data Validity Status | bit | 1 |  |
| 502 | — | Reserved | bit | 2 |  |
|  |  | **Total** |  | 504 | 63 bytes with zero padding |

* Payload 496 bits = 62 bytes (1045), 504 bits = 63 bytes (1046). [G]
* The week (DF289) is the **GST week**; the GPS week is `GST week + 1024`. [G]
* `toc` and `toe` use 60 s steps, 14 bits.
* DF291 and DF286 are the SISA index. 255 means "no accuracy prediction" (NAPA). [G]
* Health words: gnss++ folds the status bits into one number. F/NAV: `(E5a HS << 4) | (E5a DVS << 3)`. I/NAV: `(E5b HS << 7) | (E5b DVS << 6) | (E1-B HS << 1) | E1-B DVS`. The result needs more than 8 bits, so it must be stored in 16 bits. [G]
* Satellite IDs 1..36 are accepted.


## 9. State space representation messages (1057-1068, 1240-1263)

SSR messages broadcast corrections to the satellite orbit, clock and code biases instead of reference-station observations. Castle has no model for them; the layouts below come from the gnss++ decoder (`decodeSSRCorrections`) and were checked against [P]. They are not used in conventional RTK. [K]

Each constellation has six message contents. The message number is `first + k`:

| k | Content | Satellite block |
| --- | --- | --- |
| 0 | Orbit correction | ID, IOD, radial/along/cross offsets and rates |
| 1 | Clock correction | ID, C0, C1, C2 |
| 2 | Code bias | ID, N, N × (signal, bias) |
| 3 | Combined orbit and clock correction | ID, IOD, orbit part, clock part |
| 4 | URA | ID, URA index |
| 5 | High-rate clock correction | ID, clock |

| Constellation | Messages | Epoch field | Epoch bits | Satellite ID bits | IOD |
| --- | --- | --- | --- | --- | --- |
| GPS | 1057-1062 | DF385 | 20 | 6 (DF068) | IODE, 8 bits (DF071) |
| GLONASS | 1063-1068 | DF386 | 17 | 5 (DF384) | IOD, 8 bits (DF392) |
| Galileo | 1240-1245 | DF458 | 20 | 6 (DF252) | IODnav, 10 bits (DF459) |
| QZSS | 1246-1251 | DF460 | 20 | 4 (DF429) | IODE, 8 bits (DF434) |
| BeiDou | 1258-1263 | DF465 | 20 | 6 (DF488) | see §9.3 |

The satellite ID of BeiDou starts at 0 on the wire: gnss++ computes `PRN = raw + 1`. [G] SBAS SSR (1252-1257) exists in the numbering but gnss++ only maps the number to the system. [G]

### 9.1 Header

| Order | DF | Field | Type | Resolution | Present in |
| --- | --- | --- | --- | --- | --- |
| 1 | DF002 | Message number | `uint(12)` | | all |
| 2 | DF385 / DF386 / DF458 / DF460 / DF465 | Epoch time | `uint(20)`, GLONASS `uint(17)` | 1 s | all |
| 3 | DF391 | SSR update interval | `bit(4)` | table below | all |
| 4 | DF388 | Multiple message indicator | `bit(1)` | | all |
| 5 | DF375 | Satellite reference datum | `bit(1)` | | orbit (k = 0) and combined (k = 3) only |
| 6 | DF413 | IOD SSR | `uint(4)` | | all |
| 7 | DF414 | SSR provider ID | `uint(16)` | | all |
| 8 | DF415 | SSR solution ID | `uint(4)` | | all |
| 9 | DF387 | No. of satellites | `uint(6)` | | all |

The header is 68 bits (GPS orbit and combined), 67 bits (the other GPS contents), and 3 bits shorter for GLONASS. [G] [P] gnss++ reads a 4-bit satellite count for QZSS, where [P] shows 6 bits (§13).

The GPS epoch is the GPS time of week in seconds. The GLONASS epoch is the time of day in seconds, UTC + 3 h; gnss++ aligns it to the receiver's reference day and converts to GPS time. The update interval DF391 indexes this table (seconds): [G]

| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 2 | 5 | 10 | 15 | 30 | 60 | 120 | 240 | 300 | 600 | 900 | 1800 | 3600 | 7200 | 10800 |

### 9.2 Satellite blocks

**Orbit (k = 0)**: satellite ID, IOD, then

| DF | Field | Type | Resolution |
| --- | --- | --- | --- |
| DF365 | Delta radial | `int(22)` | 0.1 mm |
| DF366 | Delta along-track | `int(20)` | 0.4 mm |
| DF367 | Delta cross-track | `int(20)` | 0.4 mm |
| DF368 | Dot delta radial | `int(21)` | 0.001 mm/s |
| DF369 | Dot delta along-track | `int(19)` | 0.004 mm/s |
| DF370 | Dot delta cross-track | `int(19)` | 0.004 mm/s |

The orbit part is 121 bits. A satellite block is `id_bits + iod_bits + 121` bits (GPS: 135).

**Clock (k = 1)**: satellite ID, then

| DF | Field | Type | Resolution |
| --- | --- | --- | --- |
| DF376 | Delta clock C0 | `int(22)` | 0.1 mm |
| DF377 | Delta clock C1 | `int(21)` | 0.001 mm/s |
| DF378 | Delta clock C2 | `int(27)` | 0.00002 mm/s² |

The clock part is 70 bits. A clock-only block is `id_bits + 70` bits (GPS: 76).

**Combined (k = 3)**: ID, IOD, the orbit part, then the clock part (no second ID). Block size `id_bits + iod_bits + 191` (GPS: 205).

**Code bias (k = 2)**: satellite ID, `DF379` (`uint(5)`, number of biases N), then N repetitions of

| DF | Field | Type | Resolution |
| --- | --- | --- | --- |
| DF380 (BeiDou DF467) | Signal and tracking mode identifier | `uint(5)` | |
| DF383 | Code bias | `int(14)` | 0.01 m |

**URA (k = 4)**: satellite ID, DF389 SSR URA (`bit(6)`). Block size `id_bits + 6`. gnss++ derives the one-sigma value in metres as `(3^class × (1 + subclass/4) − 1) × 1e-3`, with `class = (index >> 3) & 7` and `subclass = index & 7`, and special-cases index 0 and indices of 63 and above. [G] The special cases follow gnss++ and were not checked against the standard.

**High-rate clock (k = 5)**: satellite ID, DF390 (`int(22)`, 0.1 mm). Block size `id_bits + 22`.

Orbit corrections are in the radial, along-track and cross-track frame of the satellite. gnss++ converts the raw values to metres: orbit offsets × 1e−4 (radial) and × 4e−4 (along, cross), orbit rates × 1e−6 (radial) and × 4e−6 (along, cross) m/s, clock C0 × 1e−4 m, C1 × 1e−6 m/s, C2 × 2e−8 m/s², high-rate clock × 1e−4 m, code bias × 0.01 m. [G]

### 9.3 BeiDou orbit blocks

For BeiDou orbit messages [P] shows `DF470` (BDS toe modulo, `bit(10)`, 8 s) followed by `DF471` (BDS IOD, `bit(8)`), 18 bits in total; gnss++ reads a 10-bit IODE followed by a 24-bit IODCRC, 34 bits in total. The two disagree; see §13.


## 10. Encoding rules (Castle)

### 10.1 Payload assembly

`bit_writer` appends fields to a caller-provided payload view, MSB first:

* `write_bits(value, width)`: `width` in 0..64; `value >> width` must be 0, otherwise `out_of_range`. A write that does not fit the remaining capacity latches `full`.
* `field(unsigned, bits)` writes `uintN`; `field(signed, bits)` writes two's complement `intN` and latches `out_of_range` when the value lies outside `−2^(bits−1) .. 2^(bits−1) − 1` (or when `bits` is 0 or above 64); `field(bool)` writes one bit; `reserved(bits)` writes zeros; `text(string<N>)` writes the 8-bit count and the characters.
* **Sticky error**: the first failure is latched in `status()`; later writes are no-ops. `encode_message()` checks the status once, after the message wrote everything.
* A byte is zeroed when the first bit lands in it, so the final padding bits are zero.
* `size()` is `(bit_size + 7) / 8`.

`encode_message(model, output, capacity, written)`:

| Condition | Result |
| --- | --- |
| `output == nullptr` | `invalid_argument` |
| `capacity < 6` | `full` |
| payload does not fit `min(capacity − 6, 1023)` | `full` |
| a field is out of range, or the model's own consistency check failed | first latched status (`out_of_range`, `invalid_argument`, ...) |
| success | `ok`, `written = payload + 6` |

On any failure `written == 0`, and the bytes already stored in `output` must not be sent.

### 10.2 Rounding and saturation

All quantisers round **half away from zero** (`castle::math::round_half_away`). Out-of-range inputs never wrap:

| Value | Rule |
| --- | --- |
| ECEF coordinate | `round(m / 0.0001)`; NaN → 0; far out of range → saturated so the writer reports `out_of_range` |
| Antenna height | `round(m / 0.0001)` clamped to `0..65535` |
| MSM rough range | `round(range_ms × 1024)`; outside `0 <= range_ms < 255` → DF397 = 255, DF398 = 0; a result whose integer part is 255 also becomes invalid |
| MSM rough rate | `round(rate)`; outside `(−8191.5, 8191.5)` → −8192 |
| MSM fine pseudorange, phase range, phase-range rate | residual against the **quantised** rough value, scaled; if it reaches the field limit the invalid marker is stored |
| MSM CNR | `round(dbhz × steps)`, clamped to `0..2^bits − 1`; NaN → 0 |

### 10.3 Building an MSM from measurements

The steps that both Castle and gnss++ follow:

1. Group the observations by satellite and signal. Keep one observation per (satellite, signal); prefer the one that has both pseudorange and phase.
2. The satellite mask holds the satellites that have at least one usable cell, the signal mask the union of their signals. The mask product `N_sat × N_sig` must not exceed 64, otherwise split the epoch (§7.13).
3. Build the cell mask satellite-major; mark a cell present only when the satellite has that signal.
4. Choose the **rough range** of each satellite from its first valid pseudorange (or, failing that, phase × wavelength): `round(range_m / c_ms × 1024)`. The integer millisecond must be below 255. A satellite without a valid rough range is dropped. Use the quantised value for every residual.
5. Choose the rough phase-range rate (MSM5/7) from a Doppler or phase-rate measurement; invalid when |rate| exceeds 8191 m/s.
6. For each cell compute the fine pseudorange `(P − rough) / c_ms × 2^24` (MSM4/5) or `× 2^29` (MSM7), the fine phase range `(Φ − rough) / c_ms × 2^29` or `× 2^31`, and the fine rate `rate − rough_rate` in steps of 0.0001 m/s.
7. Write the header, masks, satellite arrays and cell arrays (field-major, §7.5-7.6). Set DF393 = 1 unless this is the last MSM of the epoch.

For GLONASS the satellite entry also needs the frequency channel (DF419 = k + 7, 15 when unknown); without it gnss++ drops the satellite because it cannot convert phase to range. [G]

Castle's `msm_message` provides the building blocks (`satellite_bit`, `signal_bit`, `cell_bit`, `set_rough_range`, `set_pseudorange`, `set_phase_range`, `set_phase_range_rate`, `set_cnr`, `set_glonass_epoch`); the caller supplies the masks and the order of the entries. See [`msm.md`](messages/msm.md).

### 10.4 Sequencing a correction stream

* Send the station position (1005 or 1006) first and repeat it periodically; a rover cannot form a baseline without it. [K] [C]
* Then the epoch observations: MSM (one message per constellation, DF393 chain) or, for legacy rovers, 1002/1004 and 1010/1012 with the DF005 chain.
* Add 1230 whenever GLONASS is sent and the base is not a u-blox receiver. [K] [C]
* 1007/1033 are informational. [C]
* Write every frame as one contiguous block; frames of different protocols can share a port but a frame must never be split. [C]


## 11. Decoding rules (gnss++, Kalmix)

Castle does not decode RTCM. This section records how the reference decoder behaves, so that a conforming decoder, or a test that cross-checks Castle's output, can be written from it.

### 11.1 Stream handling

See §3.4. gnss++ additionally:

* Accepts a frame only if `buffer[i] == 0xD3`, `length >= 2` and the CRC matches; counts valid messages, CRC errors and decode errors.
* Reads a stream through a persistent buffer, tries to decode, and on failure advances by one byte; it never skips the declared length of a frame whose CRC failed.
* Updates the reference station position from 1005-1008 and 1033 messages whenever they arrive (`isStationMessage`); only 1005 and 1006 are actually decoded for the position. [G]

### 11.2 Validating an MSM before using it

1. Payload at least 187 bits (gnss++ minimum for the header plus one satellite entry). DF002 must equal the message number of the frame.
2. Read DF394 and DF395. `N_sat = 0`, `N_sig = 0` or `N_sat × N_sig > 64` → reject.
3. Read the cell mask and count `N_cell`.
4. Compute `required = 169 + N_sat·N_sig + N_sat·S + N_cell·C` (§7.7). A payload shorter than `required` → reject.
5. Read the satellite arrays, then the cell arrays, in field-major order.
6. For each satellite: DF397 = 255 → skip all its cells (but advance the cell index). Otherwise `rough = DF397 + DF398/1024`.
7. For each cell: look up the signal (§7.11). Skip IDs without an entry. Test each fine field against its invalid marker. Compute `pseudorange`, `phase_range`, `range_rate` (§7.8). Convert phase range to cycles with the signal wavelength; for GLONASS use the frequency channel from DF419 or the one cached from an earlier message (1020 or an MSM5/7). Without the channel no phase or Doppler is produced. [G]
8. Resolve the epoch against a reference time (§7.9).

### 11.3 Legacy GPS (1003, 1004) in gnss++

gnss++ decodes only 1003 and 1004. It reads 125 bits per satellite for 1004 and **109** bits per satellite for 1003, and reads the 8-bit ambiguity field in both. The 1003 layout of §6.1 has 101 bits per satellite and no ambiguity (§13). Satellites with ID 0 or above 32 are skipped, the L2 pseudorange is `P1 + DF017·0.02`, and no invalid marker is tested. [G]

### 11.4 Time handling

* GPS week from the 10-bit field: expand with a reference week (§8.1).
* GLONASS and SSR times of day: choose the day that places the time within half a day of the reference. For epochs: within half a week (302 400 s). [G]
* BeiDou: add 14 s (§7.9), Galileo: week + 1024. [G]
* The stream decoder resolves "time-of-week only" values against a reference time with `week = round((ref_total − tow) / 604800)`. [G]

### 11.5 Pitfalls

From [K]:

| Symptom | Likely cause | Check |
| --- | --- | --- |
| CRC fails repeatedly after one bad frame | The parser skipped the corrupted length | Resume at `preamble + 1`. |
| MSM payload runs past the frame | `N_sat × N_sig > 64`, or the cell mask is read in the wrong order | Reject; decode satellite-major. |
| MSM1 cell offsets drift by 4 bits | Reading DF402 in MSM1 | DF402 is MSM2-5 only; DF407 is MSM6-7 only. |
| MSM5/7 Doppler is impossible | DF404 treated as a satellite field, or DF399 skipped | Satellite: DF399; cell: DF404. |
| QZSS epoch mapping inconsistent | QZSS epoch labelled DF004 | The field is DF428. |
| GLONASS visible but not used | 1230 missing | Check for 1230 whenever GLONASS MSM is sent. |

RTCM payloads cannot be parsed with C structs: fields are packed by bit offset and cross byte boundaries. Use explicit bit extraction (`getbitu(buf, pos, len)`, `getbits(buf, pos, len)` with sign extension). [K]


## 12. Interoperability profiles

### 12.1 Choosing an MSM level [K]

| Scenario | Starting point | Reason |
| --- | --- | --- |
| Constrained radio link (for example 9600 bps) | MSM4 | Full carrier phase and CNR without Doppler; validate the real stream rate. |
| 4G / Wi-Fi correction link | MSM5 | Adds phase-range rate when the receiver and link can use it. |
| High-detail logging on a capable link | MSM7 | High-resolution observations; confirm link capacity first. |
| Sub-metre or extremely constrained links | MSM1 | Pseudorange only; not suited to centimetre RTK. |

The last digit is the observation-content level, not a quality score. MSM7 is not automatically better.

### 12.2 Message sets [K]

| Scenario | Required | Optional |
| --- | --- | --- |
| GPS-only RTK | 1074 or 1075, 1005 | 1019, 1033 |
| GPS + GLONASS RTK | add 1084/1085 and **1230** | 1020 |
| Four-constellation RTK | add 1094/1095, 1124/1125, 1005, 1230 | 1042, 1045/1046, 1033 |
| PPK logging | MSM7 for all constellations, 1005, ephemerides 1019/1020/1042/1045 | 1230, 1033 |
| Sub-metre DGNSS | 1071/1081 (MSM1), 1005 | |

Whenever GLONASS MSM is present, 1230 is required in mixed-vendor setups; without it the rover usually tracks GLONASS but drops it from the RTK solution without an error. [K]

### 12.3 Bandwidth [K]

Approximate rates at 1 Hz:

| Configuration | Rate | Fits 9600 bps (960 B/s) |
| --- | --- | --- |
| GPS MSM4 (about 12 satellites) | about 250 B/s | yes |
| GPS + GLONASS MSM4 | about 450 B/s | yes |
| Four constellations MSM4 | about 900 B/s | marginal |
| Four constellations MSM7 | about 1300 B/s | no, needs at least 19200 bps |
| Ephemeris, 1005 and 1230 overhead | +50 to +100 B/s | |

### 12.4 u-blox ZED-F9R input [C]

* The receiver takes RTCM 3 on every port, parses the frame from its `0xD3` byte and reports each parsed frame with UBX-RXM-RTCM (message number, station ID, CRC and use status).
* Accepted inputs are listed in §4.2. Anything else (for example 1008, MSM1-3, MSM6, 1019/1020, SSR) is reported by UBX-RXM-RTCM but not used.
* Enable the port with `CFG-<port>INPROT-RTCM3X` and optionally filter by station (`CFG-RTCM-DF003_IN`, `CFG-RTCM-DF003_IN_FILTER`). These are UBX configuration items; Castle's RTCM module does not build them. See [rtcm_input.md](../ubx/rtcm/rtcm_input.md).


## 13. Source discrepancies

Where the sources disagree this document takes the position stated in the last column. "[P]" is used as a tie-breaker only.

| # | Topic | What the sources say | Position |
| --- | --- | --- | --- |
| 1 | 1003 layout | Castle: 101 bits per satellite, no ambiguity field. [P] agrees. gnss++ decodes and encodes 109 bits per satellite and includes the 8-bit ambiguity. | Castle/[P]. A 1003 written by gnss++ is not standard and other decoders will misread it. |
| 2 | BeiDou SSR orbit IOD | [P]: DF470 (10 bits) + DF471 (8 bits). gnss++: 10-bit IODE + 24-bit IODCRC. | Undecided. The tables of §9 show both. gnss++ would misparse a message laid out as [P] shows. |
| 3 | QZSS SSR satellite count | [P]: DF387 is 6 bits for all constellations. gnss++ reads 4 bits for QZSS. | [P]. |
| 4 | 1230 field numbering | Castle and [P]: DF421 indicator, DF422 mask, DF423-DF426 biases. Kalmix: DF423 mask, DF424-DF427 biases. | Castle/[P]. |
| 5 | 1230 DF421 meaning | Castle: 1 = observations aligned to GLONASS time. Kalmix: 0 = default biases, 1 = explicit values follow. | Not resolved. Castle only stores the bit. Check the standard before interpreting it. |
| 6 | MSM reserved bits | Castle and [P]: 7 reserved bits (DF001). gnss++ calls them "session transmit time". | Reserved, written as zero. |
| 7 | DF411 values | Castle and Kalmix list 0 = unknown, 1 = not applied, 2 = applied. Kalmix also lists 3 = unknown. | 0-2 as in Castle; 3 treated as unknown. |
| 8 | DF402 lowest step | Kalmix: "0 = < 24 ms". The 4-bit table of §7.12 [R] starts at 32 ms. | Table of §7.12, marked [R]. |
| 9 | Signal-ID tables | Kalmix lists a subset (no GPS 2C/2P/2W, no BeiDou B1C/B2a/B2b). gnss++ and [P] agree with each other on the full maps. | Full maps (§7.11). |
| 10 | CRC polynomial notation | Kalmix `0x1864CFB`, Castle and gnss++ `0x864CFB`. | Same polynomial; the leading x^24 term is implicit in the 24-bit form. |
| 11 | Reserved frame bits | Kalmix: a non-zero value after `0xD3` means a false preamble. gnss++ masks them off (`& 0x03`) and does not test them. | Test them (§3.4). |
| 12 | Frame without message number | Castle `encode_frame()` accepts any payload, including an empty one. gnss++ rejects `length < 2`. | Framing layer vs message layer; see §3.3. |
| 13 | DF009 range | Castle's comment says 1..32. SBAS satellites (40..58) share the field in RTCM 10403.x. [R] | Castle's models do not check the range. |
| 14 | MSM lock-time reporting | gnss++ encodes only 0 or the maximum and decodes only "lock 0 = loss of lock". The standard defines a graded table (§7.12). | Graded table. |
| 15 | GPS L2 code indicator | Castle: 0 = C/A, 1 = P direct, 2 = P cross-correlated, 3 = Z-tracking. gnss++ maps only 0 = L2C and everything else = L2P. | Castle. |


## 14. Verification record

The statements tagged [C] were checked against running code, not only against the headers.

| Check | Method | Result |
| --- | --- | --- |
| Field widths of every Castle table in §5-§7 | Compared each DF width with the `pyrtcm` data dictionary programmatically | all equal |
| Bit layout of 17 encoded frames (1001-1007, 1009-1012, 1033, 1230, 1074, 1075, 1077, 1085) | Built with the Castle encoders from known values, decoded with `pyrtcm` 1.2.1 | all 17 decoded; DF numbers and scaled values (checked numerically for every field written) equal the inputs |
| MSM payload length formula, field-major order, mask order, zero padding | Wrote an independent bit parser from the rules of §7 and parsed the MSM4, MSM5, MSM7 and GLONASS MSM5 frames | `169 + N_sat·N_sig + N_sat·S + N_cell·C` bits matched all four; padding bits are zero |
| Legacy sizes | Payload lengths of one-satellite 1001-1004 and 1009-1012 frames, and the 3-satellite frame sizes of `rtcm_input.md` (36/42/52/61 and 38/44/54/63 bytes) | match `64 + 58/74/101/125·n` and `61 + 64/79/107/130·n` bits |
| CRC-24Q | `CRC("123456789")`, and the CRC of the golden 1005 frame in `test/castle_ext/protocols/rtcm/rtcm.cpp` | `0xCDE703`; golden frame residual 0 |
| Signal-ID tables | gnss++ `msm_tables` parsed from source and compared with the `pyrtcm` maps | GPS, GLONASS, Galileo, QZSS, BeiDou, NavIC identical |
| Ephemeris lengths | Sum of the field widths in §8 against the minimum bit counts in gnss++ | 488, 360, 511, 485, 496 (1046: 504 with 2 reserved bits; gnss++ requires 502) |
| Lock-time tables (§6.5, §7.12) | Checked that each table is monotonic and continuous at the block boundaries | pass (the values themselves are [R]) |
| Worst-case MSM7 size | Exhaustive search over `N_sat × N_sig ≤ 64` | 7657 bits, 958 bytes |

What was **not** verified:

* The ephemeris (§8) and SSR (§9) layouts were not exercised against real frames; they come from reading gnss++ and the `pyrtcm` tables.
* No source was compared with the text of RTCM 10403.x. Items marked [R] rest on memory of that standard and RTKLIB behaviour.
* Message 1008, MSM1-3 and MSM6 have no Castle encoder and were not produced or parsed.


## 15. Castle implementation map

Namespace `castle::protocols::rtcm`, headers under `include/castle_ext/protocols/rtcm/`. Everything is header-only, encode-only, and uses caller-provided buffers (no heap, exceptions, RTTI or STL).

| Message | Header | Model | Sample frame |
| --- | --- | --- | --- |
| framing | `rtcm.hpp` | `encode_frame()`, `frame_size()`, `RTCM_*` constants | [rtcm.md](rtcm.md) |
| bit stream | `bit_writer.hpp` | `bit_writer`, `encode_message()` | [bit_writer.md](bit_writer.md) |
| 1001-1004 | `messages/gps_rtk_observables.hpp` | `gps_rtk_observables<Type, MaxSatellites>` | [gps_rtk_observables.md](messages/gps_rtk_observables.md) |
| 1009-1012 | `messages/glonass_rtk_observables.hpp` | `glonass_rtk_observables<Type, MaxSatellites>` | [glonass_rtk_observables.md](messages/glonass_rtk_observables.md) |
| 1005, 1006 | `messages/station_arp.hpp` | `station_arp_message<Type>` | [station_arp.md](messages/station_arp.md) |
| 1007 | `messages/antenna_descriptor.hpp` | `antenna_descriptor` | [antenna_descriptor.md](messages/antenna_descriptor.md) |
| 1033 | `messages/receiver_antenna_descriptor.hpp` | `receiver_antenna_descriptor` | [receiver_antenna_descriptor.md](messages/receiver_antenna_descriptor.md) |
| 1230 | `messages/glonass_code_phase_biases.hpp` | `glonass_code_phase_biases` | [glonass_code_phase_biases.md](messages/glonass_code_phase_biases.md) |
| MSM4/5/7 | `messages/msm.hpp` | `msm_message<System, Level, MaxSatellites, MaxCells>` | [msm.md](messages/msm.md) |
| all | `messages/messages.hpp` | umbrella | [messages.md](messages/messages.md) |

The message numbers are compile-time constants (`RTCM_TYPE_GPS_L1_OBS` = 1001 ... `RTCM_TYPE_GLO_BIASES` = 1230). Each model has `MESSAGE_TYPE`, `accepts(type)` and `encode(bit_writer&)`; `encode_message()` turns any such model into a frame.

Status codes (`castle::status`): `ok`; `invalid_argument` (null output, inconsistent mask or count); `out_of_range` (a value does not fit its field, payload over 1023 bytes); `full` (buffer too small).

Not implemented, by design: any decoder, parser or round trip; ephemerides; SSR; MSM1-3 and MSM6; 1008; the u-blox proprietary 4072; the UBX configuration items that enable RTCM input (those belong to the UBX module).

## Appendix A. Worked examples

All frames were produced by the Castle encoders (A.1 is the golden frame of the unit test) and decoded with `pyrtcm`. Offsets are start bits within the payload.

### A.1 Message 1005, station 1234

Frame (25 bytes): `D3 00 13 3E D4 D2 3B 09 60 CF D9 9C 80 B7 03 70 C0 8B 74 3C 4C AC 19 D1 F1`

* `D3` preamble; `00 13` reserved bits and length 19; 19 payload bytes; `19 D1 F1` CRC-24Q.
* The CRC of the whole frame is 0.

| Start bit | DF | Field | Bits | Raw value | Physical |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | 12 | 1005 |  |
| 12 | DF003 | Reference station ID | 12 | 1234 |  |
| 24 | DF021 | ITRF year | 6 | 14 |  |
| 30 | DF022 | GPS | 1 | 1 |  |
| 31 | DF023 | GLONASS | 1 | 1 |  |
| 32 | DF024 | Galileo | 1 | 0 |  |
| 33 | DF141 | Reference-station indicator | 1 | 0 |  |
| 34 | DF025 | ECEF-X | 38 | 40278940060 | 4027894.0060 m |
| 72 | DF142 | Single receiver oscillator | 1 | 1 |  |
| 73 | DF001 | Reserved | 1 | 0 |  |
| 74 | DF026 | ECEF-Y | 38 | 3070456000 | 307045.6000 m |
| 112 | DF364 | Quarter-cycle indicator | 2 | 2 |  |
| 114 | DF027 | ECEF-Z | 38 | 49194749100 | 4919474.9100 m |

### A.2 Message 1004, one GPS satellite

Frame (30 bytes): `D3 00 18 3E C4 D2 52 65 C0 00 10 14 1E 84 81 FF 65 D9 11 AD 1F B5 00 24 A6 D5 00 E0 10 71`

Payload 189 bits, padded to 24 bytes (3 zero bits).

| Start bit | DF | Field | Bits | Raw value | Physical |
| --- | --- | --- | --- | --- | --- |
| 0 | DF002 | Message number | 12 | 1004 |  |
| 12 | DF003 | Station ID | 12 | 1234 |  |
| 24 | DF004 | GPS TOW | 30 | 345600000 | 345600 s |
| 54 | DF005 | Synchronous flag | 1 | 0 |  |
| 55 | DF006 | No. of satellites | 5 | 1 |  |
| 60 | DF007 | Divergence-free | 1 | 0 |  |
| 61 | DF008 | Smoothing interval | 3 | 0 |  |
| 64 | DF009 | Satellite ID | 6 | 5 |  |
| 70 | DF010 | L1 code | 1 | 0 |  |
| 71 | DF011 | L1 pseudorange (mod) | 24 | 1000000 | 20000 m |
| 95 | DF012 | L1 phase - pseudorange | 20 | -1234 | -0.617 m |
| 115 | DF013 | L1 lock time | 7 | 100 |  |
| 122 | DF014 | L1 ambiguity | 8 | 70 | 20985472.06 m |
| 130 | DF015 | L1 CNR | 8 | 180 | 45 dB-Hz |
| 138 | DF016 | L2 code | 2 | 1 |  |
| 140 | DF017 | L2 - L1 pseudorange | 14 | -300 | -6 m |
| 154 | DF018 | L2 phase - L1 pseudorange | 20 | 2345 | 1.1725 m |
| 174 | DF019 | L2 lock time | 7 | 90 |  |
| 181 | DF020 | L2 CNR | 8 | 160 | 40 dB-Hz |

The L1 pseudorange is `20 000.000 + 70 × 299 792.458 = 21 005 472.06 m`.

### A.3 Message 1074 (GPS MSM4), three satellites, two signals

Satellites 5, 12, 25, signals 1C (ID 2) and 2L (ID 16), five of six cells present. Frame (65 bytes): `D3 00 3B 43 24 D2 52 65 C0 02 80 25 84 08 00 40 00 00 00 00 20 00 80 00 76 8C 8E 90 32 19 09 67 83 0F 83 00 00 01 F4 07 D0 FF 92 A3 FF 83 00 02 EE 00 1F 40 00 CB 22 AF 37 95 45 35 57 60 35 DD 07`

| Start bit | Bits | DF | Field | Raw value(s) | Notes |
| --- | --- | --- | --- | --- | --- |
| 0 | 12 | DF002 | Message number | 1074 | GPS MSM4 |
| 12 | 12 | DF003 | Reference station ID | 1234 |  |
| 24 | 30 | DF004 | GPS epoch time | 345600000 | 345 600 000 ms = 4 days into the week |
| 54 | 1 | DF393 | Multiple message bit | 1 | more MSMs follow |
| 55 | 3 | DF409 | IODS | 2 |  |
| 58 | 7 | DF001 | Reserved | 0 |  |
| 65 | 2 | DF411 | Clock steering | 1 |  |
| 67 | 2 | DF412 | External clock | 0 |  |
| 69 | 1 | DF417 | Divergence-free smoothing | 1 |  |
| 70 | 3 | DF418 | Smoothing interval | 3 |  |
| 73 | 64 | DF394 | Satellite mask | 0x0810008000000000 | satellites [5, 12, 25] |
| 137 | 32 | DF395 | Signal mask | 0x40010000 | signals [2, 16] (1C and 2L) |
| 169 | 6 | DF396 | Cell mask | 111011 | 5 of 6 cells present |
| 175 | 24 | DF397 | Rough range, integer ms | 70 71 72 | 3 × 8 bits (one per satellite) |
| 199 | 30 | DF398 | Rough range modulo 1 ms | 100 200 300 | 3 × 10 bits (× 2^-10 ms) |
| 229 | 75 | DF400 | Fine pseudorange | -2000 -1000 0 1000 2000 | 5 × 15 bits (one per cell, × 2^-24 ms) |
| 304 | 110 | DF401 | Fine phase range | -7000 -2000 3000 8000 13000 | 5 × 22 bits (× 2^-29 ms) |
| 414 | 20 | DF402 | Lock time indicator | 10 11 12 13 14 | 5 × 4 bits |
| 434 | 5 | DF420 | Half-cycle ambiguity | 0 1 0 1 0 | 5 × 1 bits |
| 439 | 30 | DF403 | CNR | 40 41 42 43 44 | 5 × 6 bits (dB-Hz) |
| 469 | 3 | — | Padding | 0 | zero bits to the byte boundary |

Reading rule: the first `DF397` value (70) belongs to satellite 5, the second (71) to satellite 12, the third (72) to satellite 25. The five cell values map to the cells in cell-mask order: (5, 1C), (5, 2L), (12, 1C), (25, 1C), (25, 2L). The rough range of satellite 5 is `70 + 100/1024 ms = 70.0977 ms`, that is 21 014 km; the fine pseudorange of its first cell is `−2000 × 2^−24 ms`, that is −35.7 m.
