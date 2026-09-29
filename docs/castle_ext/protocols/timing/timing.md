# Timing

## Overview
The Castle timing umbrella header exposes the slave-side time synchronization components as one include. It currently combines the transport-neutral PTPv2 end-to-end slave engine with the deterministic proportional step/slew servo.

## Header
`#include "castle_ext/protocols/timing/timing.hpp"`

## Dependencies
- [`castle_ext/protocols/timing/ptp_v2.hpp`](ptp_v2.md) — PTPv2 slave protocol engine.
- [`castle_ext/protocols/timing/ptp_servo.hpp`](ptp_servo.md) — deterministic clock adjustment policy.

## Public API
The umbrella header adds no new types or functions. It makes the following components available together:

| Component | Purpose |
|---|---|
| `castle::timing::ptp::slave` | Parse PTPv2 traffic and calculate slave/master offset and path delay. |
| `castle::timing::ptp::proportional_servo` | Convert an offset measurement into a bounded step or slew action. |
| PTP utility types/functions | Timestamp, port identity, decoding, Delay_Req encoding, and arithmetic helpers. |
| Clock adapter helper | `apply_clock_adjustment()` dispatches to a user-supplied `step()` / `slew()` adapter. |

See [`PtpV2`](ptp_v2.md) and [`PtpServo`](ptp_servo.md) for complete APIs and algorithm details.

## Usage Example

The matching sample is [`samples/castle_ext/protocols/timing/timing.cpp`](../../../../samples/castle_ext/protocols/timing/timing.cpp).

```cpp
#include "castle_ext/protocols/timing/timing.hpp"

int main()
{
    castle::timing::ptp::port_identity local{};
    local.port_number = 1U;

    castle::timing::ptp::slave protocol(local);
    castle::timing::ptp::proportional_servo servo;
    (void)protocol;
    (void)servo;
    return 0;
}
```

## Detailed End-to-End Sequence Diagram

This is the architectural sequence across the complete Castle timing path. It shows where the master, network timestamping boundary, PTPv2 state machine, servo, and OS/RTOS clock service meet.

```plantuml
@startuml
hide footbox
autonumber 1
participant "PTP Master" as Master
participant "Slave NIC /\nSocket / RTOS transport" as Net
participant "Slave application\nPTP adapter" as App
participant "castle::timing::ptp::slave" as PTP
participant "castle::timing::ptp::proportional_servo" as Servo
participant "Platform clock adapter" as Clock
participant "System / PTP hardware clock" as Sys

== Synchronization ==
Master -> Net: Sync
Net -> App: packet + precise RX timestamp (t2)
App -> PTP: receive(packet, t2, action)
PTP -> PTP: decode_message()
PTP -> PTP: consume_sync()
PTP -> PTP: encode_delay_request()
PTP --> App: action.send_delay_request=true
App -> Net: transmit(action.delay_request)
Net -> App: precise TX timestamp (t3)
App -> PTP: delay_request_transmitted(t3, action)

alt two-step Sync
    Master -> Net: Follow_Up (t1)
    Net -> App: packet
    App -> PTP: receive(packet, rx_ts, action)
    PTP -> PTP: consume_follow_up()
    PTP -> PTP: read_message_timestamp()
end

Master -> Net: Delay_Resp (t4)
Net -> App: packet
App -> PTP: receive(packet, rx_ts, action)
PTP -> PTP: consume_delay_response()
PTP -> PTP: validate master/requesting identity
PTP -> PTP: read_timestamp(t4)
PTP -> PTP: try_complete()
PTP -> PTP: timestamp_difference_checked()
PTP -> PTP: apply correction fields
PTP -> PTP: path_delay = (sync + delay) / 2
PTP -> PTP: offset = sync - path_delay
PTP --> App: measurement_ready + measurement.offset_nanoseconds

== Clock discipline ==
App -> Servo: update(offset, correction_interval, adjustment)
Servo -> Servo: validate input/config
alt large phase error
    Servo -> Servo: step decision
    Servo --> App: adjustment_mode::step
else small phase error
    Servo -> Servo: proportional_ppb()
    Servo -> Servo: saturate frequency correction
    Servo --> App: adjustment_mode::slew
else zero / no effective correction
    Servo --> App: adjustment_mode::none
end

App -> Clock: apply_clock_adjustment(clock, adjustment)
alt step
    Clock -> Sys: step(offset_nanoseconds)
else slew
    Clock -> Sys: slew(frequency_ppb)
else none
    Clock --> App: status::ok
end
Sys --> Clock: status
Clock --> App: status
@enduml
```

### Responsibilities and boundaries

```plantuml
@startuml
left to right direction
rectangle "Master clock" as Master
rectangle "Transport boundary\nEthernet / UDP / socket / RTOS" as Transport
rectangle "castle::timing::ptp::slave\nPTPv2 protocol state" as PTP
rectangle "castle::timing::ptp::proportional_servo\nClock discipline policy" as Servo
rectangle "Platform clock adapter\nstep()/slew()" as Adapter
rectangle "OS / RTOS / PHC" as OS

Master --> Transport : PTP wire messages
Transport --> PTP : RX bytes + t2/timestamp callback
PTP --> Transport : bounded Delay_Req bytes
Transport --> PTP : t3 TX timestamp + Delay_Resp
PTP --> Servo : measured offset
Servo --> Adapter : step/slew action
Adapter --> OS : platform-specific clock operation
OS --> Adapter : status
@enduml
```

## Constraints & Notes

- The umbrella header itself does not allocate or introduce a transport/OS dependency.
- Protocol operation remains event-driven: the application supplies packet bytes and precise timestamps.
- Clock changes are not performed implicitly. Applications explicitly turn measurements into actions and then apply those actions through their platform adapter.
- Include individual headers instead of the umbrella when compile-time include footprint matters.

## Detailed Architecture

```text
                 transport / NIC / driver
                           |
                 RX bytes + RX timestamp
                           v
                 +----------------------+
                 |   PTPv2 slave        |
                 |  castle_ext timing   |
                 +----------+-----------+
                            |
                   Delay_Req action
                            |
                            v
                 transport / NIC / driver
                            |
                     TX timestamp
                            |
                            v
                 +----------------------+
                 |   PTPv2 slave        |
                 +----------+-----------+
                            |
                     measurement
                    (offset, path delay)
                            |
                            v
                 +----------------------+
                 |  proportional servo  |
                 +----------+-----------+
                            |
                  step / slew action
                            |
                            v
                 platform clock adapter
```

The separation means the protocol can be reused whether timestamps come from hardware timestamping, a kernel socket timestamp, Zephyr's network stack, or another deterministic timestamp source. It also makes unit testing possible without a network or real system clock.
