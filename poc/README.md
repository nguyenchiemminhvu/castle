# Castle GNSS Location Service (C++17)

A bounded Embedded Linux GNSS location service built on the supplied Castle header tree. The application has no STL containers/algorithms, exceptions, RTTI, virtual interface hierarchy, or application-owned heap allocation. All queues, registries, parser storage, maps, strings, observer slots, and task slots have compile-time capacities.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The executable is `build/location_service`. CMake vendors the supplied Castle `include/` tree under `third_party/castle/include`, sets C++17 and the POSIX/pthread Castle feature switches, and compiles with `-fno-exceptions -fno-rtti`.

## Run

### PTY simulation (default)

```sh
cd build
./location_service --simulate --run-for 12
```

The executable creates a POSIX pseudo-terminal, opens its slave through Castle's UART, and writes mock UBX/NMEA traffic to the master. The mock sends a CFG-VALGET response containing zero-valued current settings, an ACK-ACK for CFG-VALSET, a valid NAV-PVT update each second, and one NMEA GGA sentence. NAV-PVT coordinates advance by roughly 50 m+ per fix so Pearlchain waypoint recording is visible.

To exercise the no-data/no-valid-PVT recovery path instead of continuing to send fixes:

```sh
./location_service --simulate-stall --run-for 12
```

This sends the initial mock frames and then remains silent; Level 1 reopen and Level 2 cold-start actions become visible in the log.

### Hardware UART

```sh
./location_service --hardware --uart /dev/ttyS1 --config /etc/ubx_config.cfg
```

The default UART baud is 2,000,000, 8-N-1, no flow control. Hardware mode runs until SIGINT or SIGTERM. The executable installs Castle signal IPC callbacks before starting the POSIX 10 ms timer.

## Architecture

The compile-time root is `gnss::LocationService::Graph` in `include/gnss/components.hpp`. Castle `object_graph` constructs the components in dependency order and controls init/deinit. The declared graph order is:

1. `GnssTransConfig`, `DataStore`, `WorkerPool`, and `ChinaCoordinateService`.
2. `GnssTransport` (constructed with read-only config), both parser nodes, and `UbxController` (uses config and transport).
3. UBX/NMEA decode nodes (use their parser, the data store, and China coordinate service).
4. `GnssReceiver`, `LocationLogger`, `Pearlchain`, and `DiagnosticComponent`.
5. `GnssHal`, `TickEngine`, and `PosixTickSource`.

The graph's `objects()` composite is used for power fan-out. Suspend and shutdown visit components in reverse declaration order; resume visits them forward. Resume is rejected unless the service was suspended. Startup initializes and starts all components immediately; no artificial initial resume event is required.

### Receive and data flow

`GnssTransport` owns Castle `castle_ext::io::uart::uart`. Reads are readiness-checked through `castle_ext::io::fd::wait_fd`; the UART itself owns Castle's fd descriptor. One receiver owns the byte stream and feeds the same bounded buffer to the UBX parser and NMEA parser in a fixed order. This intentionally replaces the POC-style concept of two independent readers: two parser threads must not race to consume one UART stream. Parser callbacks are synchronous, so the associated Castle decoder registries immediately decode each message and copy the resulting values into the pool.

`Pool` holds the latest `LocationData`, `TimeData`, `SatelliteData`, `PvtHeartbeat`, and `HalStatus`. Its fixed `data_slot` reader capacities protect short-lived snapshots. A dedicated `PvtHeartbeat` prevents a coalesced GGA latest-value update from hiding a valid PVT from the 5-second HAL health observer. Logger, Pearlchain, and HAL register independent `update_inbox` observers; notifications coalesce changed-type bits and consumers fetch only the relevant latest snapshots. Location data retains the original WGS-84 pair for traceability while exposing consumer-facing coordinates already shifted to GCJ-02 inside Castle's configured region.

### UBX boot configuration

`UbxController` parses the global key/value INI document from the configured file into `castle::dsa::map<uint32_t, uint64_t, 64>`. At HAL startup it requests all desired keys in one CFG-VALGET poll. On response, it compares returned key/value pairs against the desired map and creates a CFG-VALSET message containing only missing or differing values. The VALSET is sent to RAM, BBR, and Flash layers; a matching ACK-ACK marks synchronization complete, while ACK-NAK marks it failed. No write is emitted when there are no differences.

### Health and recovery

Castle `diagnostic_manager` tracks `NoBytesTag` and `NoPvtTag`, and a composite `ReceiverFaultTag` is active when either source fault is active. A fault is asserted after 5,000 ms without bytes or without a valid UBX PVT. The diagnostic manager recovery callback queues Level 1 immediately; a Castle `action_sequence` waits a further 5,000 ms and then queues Level 2. Both requests pass through Castle `event_handler`; it forwards the request to the bounded Castle worker pool. Transport operations remain on the HAL/service thread:

- Level 1: close/reopen the UART transport.
- Level 2: send UBX-CFG-RST cold-start.

### Periodic updates and Pearlchain

A Linux `timer_create(CLOCK_MONOTONIC)` timer generates SIGUSR1 every 10 ms. Castle `signal_ipc_event` dispatches the signal to a callback that only sets a `sig_atomic_t` flag. The main loop computes elapsed 10 ms periods (accounting for coalesced signals) and advances Castle `tick_timer`; tick callbacks update HAL, logger, and Pearlchain. Pearlchain uses Castle's `haversine_distance` and a 128-entry fixed ring buffer, keeping the first valid fix as its anchor and recording each later point at least 50 m away. Waypoint output is submitted as a bounded worker-pool task.

## Source layout

- `include/gnss/types.hpp`: pool types, service configuration, and fixed data types.
- `include/gnss/components.hpp`: component contracts and object graph relationships.
- `src/transport.cpp`: UART/fd transport and bounded polling.
- `src/ubx_controller.cpp`: INI parsing, fixed config map, VALGET/VALSET diff, reset.
- `src/parsers.cpp`: Castle geodetic conversion and UBX/NMEA decoder registries.
- `src/receiver.cpp`: sole UART reader feeding both parsers.
- `src/consumers.cpp`: update-inbox logger and Pearlchain.
- `src/diagnostic.cpp`: diagnostic manager, event handler, worker-pool recovery handoff.
- `src/gnss_hal.cpp`: orchestration and recovery execution.
- `src/tick_source.cpp`: Castle tick timer and POSIX timer source.
- `src/location_service.cpp`: object-graph lifecycle and composite power fan-out.
- `src/main.cpp`: signal IPC, PTY simulation, hardware CLI, and main loop.
- `config/ubx_config.cfg`: requested key/value boot configuration.
- `third_party/castle/include`: supplied Castle headers, unmodified.

## Simulation scope and integration notes

The PTY mock exercises the actual UART-open/read path, both parsers, decoder registries, config diff/send path, ACK handling, pool notifications, coordinate conversion, logging, and waypoint logic. Its current-value CFG-VALGET data is intentionally synthetic (all zero); it is not a full u-blox firmware emulator. Real hardware validation is still required for receiver-specific key availability, layer persistence policy, baud support, and actual ACK timing. The service currently registers NAV-PVT, CFG-VALGET, ACK-ACK/ACK-NAK, and NMEA GGA decoders; additional message decoders can be added to the compile-time registries without changing the transport or pool architecture.
