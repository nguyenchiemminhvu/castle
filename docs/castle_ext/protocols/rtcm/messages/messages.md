# RTCM Message Umbrella

## Overview

`messages.hpp` includes every RTCM 3.3 message model shipped with Castle: the legacy GPS and GLONASS RTK observables (1001-1004, 1009-1012), the station information messages (1005, 1006, 1007, 1033, 1230), and the Multiple Signal Messages MSM4/MSM5/MSM7 for GPS, GLONASS, Galileo and BeiDou. These are the RTCM messages a u-blox ZED-F9R accepts as correction input (UBX-19056845). Include it for convenience, or include single message headers for a smaller build.

The module is **encode only**: it builds RTCM frames that are sent *to* the receiver. There is no parser, no decoder and no round-trip support, by design.

Every model follows the same shape:

- each model is one message number fixed at compile time: `static constexpr uint16_t MESSAGE_TYPE` is the number it writes, and `static constexpr bool accepts(uint16_t type)` is `type == MESSAGE_TYPE`. Families of related numbers are one class template with one alias per number (`gps_rtk_l1_obs` ... `gps_rtk_l1_l2_obs_ext`, `station_arp` / `station_arp_height`, `gps_msm4` ...), so a message number can never be wrong at run time;
- `void encode(bit_writer&) const noexcept` writes the payload as an MSB-first bit stream and latches `invalid_argument`, `out_of_range` or `full` on the writer instead of returning a value;
- `encode_message(model, output, capacity, written)` (see [`bit_writer`](../bit_writer.md)) writes the payload directly into the frame buffer and adds the preamble, length and CRC-24Q, with no scratch copy;
- variable-length models keep their blocks in a fixed-capacity `castle::container::vector` whose capacity is a template parameter (`gps_rtk_observables<RTCM_TYPE_GPS_L1L2_OBS_EXT, 12U>` ...), so nothing allocates.

## Header

`#include "castle_ext/protocols/rtcm/messages/messages.hpp"`

## Dependencies

- [`castle_ext/protocols/rtcm/messages/gps_rtk_observables.hpp`](gps_rtk_observables.md)
- [`castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp`](glonass_rtk_observables.md)
- [`castle_ext/protocols/rtcm/messages/station_arp.hpp`](station_arp.md)
- [`castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp`](antenna_descriptor.md)
- [`castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp`](receiver_antenna_descriptor.md)
- [`castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp`](glonass_code_phase_biases.md)
- [`castle_ext/protocols/rtcm/messages/msm.hpp`](msm.md)

## Public API

This header declares nothing itself. It exposes the following models, all in `castle::protocols::rtcm::messages`.

### Legacy RTK observables

| Header | Model | Messages |
| --- | --- | --- |
| [`gps_rtk_observables.hpp`](gps_rtk_observables.md) | `gps_rtk_l1_obs`, `gps_rtk_l1_obs_ext`, `gps_rtk_l1_l2_obs`, `gps_rtk_l1_l2_obs_ext` (`gps_rtk_observables<Type, MaxSatellites>`) | 1001 L1, 1002 L1 extended, 1003 L1/L2, 1004 L1/L2 extended GPS observables. |
| [`glonass_rtk_observables.hpp`](glonass_rtk_observables.md) | `glonass_rtk_l1_obs`, `glonass_rtk_l1_obs_ext`, `glonass_rtk_l1_l2_obs`, `glonass_rtk_l1_l2_obs_ext` (`glonass_rtk_observables<Type, MaxSatellites>`) | 1009 L1, 1010 L1 extended, 1011 L1/L2, 1012 L1/L2 extended GLONASS observables. |

### Station information

| Header | Model | Messages |
| --- | --- | --- |
| [`station_arp.hpp`](station_arp.md) | `station_arp`, `station_arp_height` (`station_arp_message<Type>`) | 1005 stationary reference station ARP; 1006 the same with antenna height. |
| [`antenna_descriptor.hpp`](antenna_descriptor.md) | `antenna_descriptor` | 1007 antenna descriptor and setup ID. |
| [`receiver_antenna_descriptor.hpp`](receiver_antenna_descriptor.md) | `receiver_antenna_descriptor` | 1033 receiver and antenna descriptors. |
| [`glonass_code_phase_biases.hpp`](glonass_code_phase_biases.md) | `glonass_code_phase_biases` | 1230 GLONASS L1 and L2 code-phase biases. |

### Multiple Signal Messages

| Header | Model | Messages |
| --- | --- | --- |
| [`msm.hpp`](msm.md) | `gps_msm4`, `gps_msm5`, `gps_msm7` | 1074, 1075, 1077. |
| | `glonass_msm4`, `glonass_msm5`, `glonass_msm7` | 1084, 1085, 1087. |
| | `galileo_msm4`, `galileo_msm5`, `galileo_msm7` | 1094, 1095, 1097. |
| | `beidou_msm4`, `beidou_msm5`, `beidou_msm7` | 1124, 1125, 1127. |

Every supported message number is accepted by exactly one model; all other numbers (for example 1008, 1013, the MSM1-MSM3/MSM6 levels, 1071-1073, and the u-blox proprietary 4072, which UBX-RXM-RTCM reports but the receiver does not take as input) are accepted by none.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/messages.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

// Appends one frame to `stream` and advances `used`.
template <typename Model>
castle::status append(const Model& model, uint8_t* stream, castle::size_type capacity, castle::size_type& used)
{
    castle::size_type written = 0U;
    const castle::status result = encode_message(model, &stream[used], capacity - used, written);
    used += written; // written is 0 on failure
    return result;
}

castle::status encode_epoch(uint8_t* stream, castle::size_type capacity, castle::size_type& used)
{
    used = 0U;

    station_arp_height station{}; // 1006
    station.station_id = 1234U;
    station.gps = true;
    station.glonass = true;
    station.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    station.set_antenna_height_m(1.5);

    gps_rtk_l1_l2_obs_ext gps{}; // 1004
    gps.station_id = 1234U;

    glonass_rtk_l1_l2_obs_ext glonass{}; // 1012
    glonass.station_id = 1234U;

    castle::status result = append(station, stream, capacity, used);
    if (castle::succeeded(result))
    {
        result = append(gps, stream, capacity, used);
    }
    if (castle::succeeded(result))
    {
        result = append(glonass, stream, capacity, used);
    }
    return result;
}
```

See [`samples/castle_ext/protocols/rtcm/messages/messages.cpp`](../../../../../samples/castle_ext/protocols/rtcm/messages/messages.cpp). Before sending, enable RTCM 3 input on the receiver port with a UBX-CFG-VALSET (`CFG-<port>INPROT-RTCM3X`, see [`ubx_config`](../../ubx/ubx_config.md)); the encoded frames are then written to that port unchanged, without a UBX header. [RTCM 3.3 correction input to a u-blox receiver](../../ubx/rtcm/rtcm_input.md) maps every u-blox `RTCM-3X-TYPEnnnn` entry to its model and has one sample per message group.

## Constraints & Notes

- Header-only. No heap allocation, exceptions, RTTI, virtual dispatch or STL. Capacities of variable-length models are template parameters; exceeding one is reported by `vector::push_back()` as `status::full`.
- Encode only. Nothing in this module decodes RTCM; verify frames against an independent decoder (for example the Python `pyrtcm` library) when adding messages.
- Models are plain value types, `encode()` does not modify them, and they are not thread-safe. Encoding the same model twice produces identical bytes.
- On any failure `encode_message()` returns the first failure and leaves `written == 0`; bytes already stored in `output` are undefined and must not be transmitted.
- The models do not share a base class or registry; select the model by message number yourself (`accepts()` helps when numbers come from configuration).
