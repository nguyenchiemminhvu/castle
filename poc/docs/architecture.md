# Design decisions and bounded state

## Object graph relations

`LocationService::Graph` in `include/gnss/components.hpp` is the composition root. The config is injected read-only into the transport. `UbxController` aggregates config and transport. UBX/NMEA decoder nodes aggregate their matching parser, the shared data store, and the coordinate service. The receiver aggregates transport and both parser nodes. Logger, Pearlchain, and diagnostics aggregate the data store; Pearlchain/diagnostics also aggregate the worker pool. HAL aggregates transport, controller, receiver, diagnostics, and data store. TickEngine aggregates HAL and both consumers.

Every graph member exposes `init`, `deinit`, `suspend`, `resume`, and `shutdown`. `object_graph` owns object lifetime and invokes init/deinit; `objects()` returns Castle's composite, which supplies explicit reverse/forward lifecycle traversal for power events.

## Pool contract

`Pool` is compile-time typed to exactly four latest-value records:

| Type | Publisher | Consumers |
|---|---|---|
| `LocationData` | UBX PVT and NMEA GGA decoders | HAL health observer, logger, Pearlchain, future service clients |
| `TimeData` | UBX PVT decoder | future time clients |
| `SatelliteData` | UBX PVT and NMEA GGA decoders | future satellite clients |
| `PvtHeartbeat` | valid UBX PVT decoder | HAL 5-second valid-PVT health monitor |
| `HalStatus` | HAL and diagnostic component | future health clients |

The shared pool stores latest records rather than queued histories. `update_inbox` coalesces notifications; consumers must read the latest snapshot after taking an update set. The dedicated `PvtHeartbeat` type is intentional: a following GGA update may replace the latest `LocationData`, but it cannot mask that a valid PVT was received in the same batch. Snapshot reader counts are fixed at four per type; observer capacity is eight.

## Recovery state machine

```text
NORMAL
  | (no bytes >= 5 s OR no valid PVT >= 5 s)
  v
FAULT_ACTIVE -- action_sequence call --> REQUEST_LEVEL_1
  |                                  (event_handler -> worker_pool flag)
  | 5 s action_sequence delay
  v
REQUEST_LEVEL_2                    (event_handler -> worker_pool flag)
  |
  +-- HAL thread executes reopen at L1
  +-- HAL thread sends CFG-RST cold start at L2
  |
  +-- fault clears -> NORMAL / recovery level reset
```

The diagnostic manager owns DTC state and group activation. The action sequence is driven by elapsed Castle chrono milliseconds on the tick path. Async event and worker threads only move a bounded recovery request bitmask; the UART remains single-owner on the HAL thread.

## Power event order

- Suspend: disarm POSIX tick source first, pause Castle tick engine and diagnostics, then stop producers/consumers and close UART via reverse graph order.
- Resume: re-open UART and restore component state in forward graph order; re-arm timer last.
- Shutdown: reverse-order composite invokes every component shutdown, then `object_graph::deinit()` releases graph-managed resources.

The application does not manufacture a power-manager input. External IPC code can call `LocationService::suspend()`, `resume()`, and `shutdown()`.

## Determinism and capacities

- Config: 64 map entries and 64 UBX keys/VALSET entries.
- Pool: 8 observers; 4 reader slots for each published type.
- UART RX staging: 256 bytes.
- Decoder subscriber capacity: one callback per configured message type.
- Worker pool: one pthread, 16 queued tasks, 96-byte callable storage.
- Diagnostic event handler: 8 event slots.
- Pearlchain: 128 fixed waypoints; explicit force-push evicts the oldest record at capacity.
- Tick callback registry: one callback, one tick period, 10 ms external clock.
- Logging builders and parser buffers are fixed-size.
