# castle_ext/net — Architecture

Heap-free TCP/UDP networking over the BSD socket API for CASTLE. It provides numeric IPv4/IPv6 endpoints, typed move-only sockets, callback-driven clients, and fixed-capacity servers. It follows CASTLE's constraints: no heap, no exceptions, no RTTI, no virtual functions, no STL, and bounded, deterministic memory.

Per-header API reference lives in [`docs/castle_ext/net/`](../../../docs/castle_ext/net/net.md). This document explains how the pieces fit together and why.

## 1. Goals and constraints

| Goal | How it is met |
|---|---|
| No allocation | Peer table, receive buffer and callbacks are plain members sized by template parameters; `sizeof(server)` is its whole memory cost. |
| No virtual / RTTI | Composition and templates only. Callbacks are `castle::callbacks::function` (function-pointer type erasure). |
| Errors without exceptions | Every operation returns `castle::status`; the native `errno` is reported through an optional `int* system_error`. |
| Reuse | Descriptor ownership, `poll()` and `fcntl` helpers come from `castle_ext/io/fd`; callbacks from `castle/callbacks`. |
| Portable build | All host-ABI code sits in one header. Without `CASTLE_USING_POSIX_APIS` the API still compiles and returns `not_configured`. |
| Deterministic | One datagram or one stream chunk per `process_*()` call; no worker threads, no hidden loops. The application owns the event loop. |

## 2. Layering

Dependencies point downward only. Upper layers never call the host socket API directly.

```plantuml
@startuml
skinparam componentStyle rectangle
skinparam shadowing false
skinparam linetype polyline

package "Application" {
  [Event loop / firmware code] as App
}

package "castle_ext/net" {
  [client.hpp\nbasic_client<Protocol,Buf,Cb>] as Client
  [server.hpp\nbasic_server<Protocol,Max,Buf,Cb>] as Server
  [socket.hpp\nbasic_socket<Protocol>] as Socket
  [endpoint.hpp\nendpoint, address_family] as Endpoint
  package "detail" {
    [native.hpp\nBSD calls -> castle::status] as Native
    [callback_dispatcher.hpp\nin-place invoke, deferred replace/clear] as Dispatcher
  }
}

package "castle_ext/io/fd" {
  [descriptor.hpp\nbasic_fd (RAII)] as Fd
  [control.hpp\nfcntl, errno helpers] as Control
  [poll.hpp\nwait_fd, fd_event_manager] as Poll
}

package "castle core" {
  [callbacks::function] as Function
  [error::status] as Status
}

cloud "Host BSD sockets\n(Linux / POSIX / Zephyr)" as Host

App --> Client
App --> Server
App ..> Poll : multiplex clients via\nclient_native_handle()
Client --> Socket
Server --> Socket
Client --> Dispatcher
Server --> Dispatcher
Dispatcher --> Function
Client --> Function
Server --> Function
Socket --> Native
Socket --> Endpoint
Native --> Endpoint
Native --> Fd
Native --> Control
Native --> Poll
Native --> Host
Socket ..> Status
@enduml
```

| Layer | Responsibility |
|---|---|
| `endpoint` | Value type: family, 4 or 16 address bytes, host-order port, IPv6 scope. Trivially copyable, fully `constexpr`, no socket ABI dependency. |
| `detail/native` | The only code that includes `<sys/socket.h>`. One function per operation, each returning `status`; converts `endpoint` to and from `sockaddr`; handles close-on-exec, `accept4`, `MSG_NOSIGNAL`/`SO_NOSIGPIPE`, `MSG_TRUNC` truncation. One body serves Linux and Zephyr; provides non-POSIX stubs. |
| `basic_socket` | Move-only owner of one descriptor (`basic_fd`). Validates state and arguments (closed, family mismatch, protocol misuse) and maps results: would-block read is `empty`, would-block write is `full`, a short datagram is `data_loss`. No `#if`. |
| `basic_client` | One socket, one receive buffer, one callback. `process_once()` = wait, receive, dispatch. |
| `basic_server` | A listener (TCP) or bound socket (UDP), a fixed slot table with generation-checked handles, a shared receive buffer, a default callback copied into each new slot. |
| `callback_dispatcher` | Runs a stored callback in place and defers a replace or clear that the callback requests for itself. |

## 3. Class model

```plantuml
@startuml
skinparam classAttributeIconSize 0
skinparam shadowing false
skinparam linetype polyline

enum address_family {
  ipv4
  ipv6
}
enum socket_protocol {
  tcp
  udp
}

class endpoint {
  -address_[16] : uint8_t
  -family_ : address_family
  -port_ : uint16_t
  -scope_id_ : uint32_t
  +ipv4(a,b,c,d,port)$ : endpoint
  +ipv6(bytes,port,scope)$ : endpoint
  +family() / port() / scope_id()
  +is_any() : bool
  +operator==(endpoint) : bool
}

class "basic_socket<Protocol>" as Socket {
  -descriptor_ : io::fd::basic_fd
  -family_ : address_family
  -nonblocking_ : bool
  +open() / close()
  +bind() / listen() / accept()
  +connect() / finish_connect()
  +send_some() / send_all() / send_to()
  +receive_some() / receive_from()
  +wait() / wait_readable()
  +local_endpoint() / peer_endpoint()
  +shutdown()
}

class "callback_dispatcher<Callback>" as Dispatcher {
  -running_ : Callback const*
  -pending_ : Callback
  -action_ : none|replace|clear
  +is_dispatching() : bool
  +is_running(target) : bool
  +replace(target, next)
  +clear(target)
  +invoke(target, args...)
}

class "basic_client<Protocol,Buf,Cb>" as Client {
  -socket_ : basic_socket
  -receive_callback_ : callback_type
  -dispatcher_ : callback_dispatcher
  -peer_ : endpoint
  -buffer_[Buf] : uint8_t
  +open() / bind() / connect() / close()
  +send_* / receive_*
  +set_receive_callback() / clear_receive_callback()
  +process_once(timeout) : status
}

class "basic_server<Protocol,Max,Buf,Cb>" as Server {
  -listener_ : basic_socket
  -slots_[Max] : slot
  -default_callback_ : callback_type
  -dispatcher_ : callback_dispatcher
  -buffer_[Buf] : uint8_t
  -client_count_ : size_type
  +open(local) / close()
  +accept_client(out, timeout)  : TCP only
  +process_client(handle, timeout)  : TCP only
  +process_datagram(out, timeout, err, source)  : UDP only
  +send_some / send_all
  +set_client_callback / clear_client_callback
  +client_at(i) / client_native_handle(h)
  +close_client(h)
}

class "basic_server::slot" as Slot {
  +socket : basic_socket
  +peer : endpoint
  +callback : callback_type
  +generation : uint32_t
  +used : bool
  +closing : bool
}

class "basic_server::client_handle" as Handle {
  +index : size_type
  +generation : uint32_t
  +is_valid() : bool
}

Socket *-- "1" "io::fd::basic_fd"
Socket --> address_family
Socket ..> endpoint : uses
Client *-- "1" Socket
Client *-- "1" Dispatcher
Client *-- "1" endpoint : peer_
Server *-- "1" Socket : listener_
Server *-- "Max" Slot
Server *-- "1" Dispatcher
Slot *-- "1" Socket
Slot *-- "1" endpoint
Handle ..> Slot : index + generation
Server ..> Handle : issues
Client ..> socket_protocol
Server ..> socket_protocol
@enduml
```

`Protocol` is a compile-time template parameter, so TCP-only and UDP-only operations on the wrong type fail at runtime with `invalid_config` (and `server::accept_client` is compiled out for UDP via `if constexpr`). There is no inheritance anywhere.

## 4. Key behaviours

### 4.1 Client: connect and receive

```plantuml
@startuml
skinparam shadowing false
actor App
participant "basic_client" as C
participant "basic_socket" as S
participant "detail::native" as N
participant "callback_dispatcher" as D

App -> C : connect(remote, timeout_ms)
C -> S : open(family) [if closed]
C -> S : connect(remote)
S -> N : connect_socket()
N --> S : time_out (EINPROGRESS)
C -> S : finish_connect(timeout_ms)
S -> N : finish_connect_socket()
note right of N
  poll(POLLOUT), then SO_ERROR
  and getpeername() must both succeed
end note
N --> C : ok
C -> C : peer_ = remote (cached)
alt failure and socket opened here
  C -> S : close()
end

App -> C : process_once(timeout)
C -> S : wait_readable(timeout)
C -> S : receive_some / receive_from -> buffer_
S -> N : receive_bytes() [recvfrom]
C -> D : invoke(receive_callback_, peer, buffer_, n)
D -> App : callback(peer, data, size)
App --> D : returns
D -> D : apply deferred replace/clear
C --> App : ok
@enduml
```

The TCP peer endpoint is cached at connect time, so bytes already read are always delivered even if the connection has just been reset.

### 4.2 Server: accept, process, dispatch

```plantuml
@startuml
skinparam shadowing false
actor App
participant "basic_server" as V
participant "listener_ / slot.socket" as S
participant "callback_dispatcher" as D

App -> V : accept_client(out, timeout)
V -> V : find_free_slot()
alt no free slot
  V --> App : full (does not wait)
else
  V -> S : wait_readable(timeout)
  V -> S : accept(slot.socket)
  V -> V : activate_slot(): copy default callback,\nbump generation
  V --> App : ok, out = {index, generation}
end

App -> V : process_client(handle, timeout)
V -> V : find_slot(handle) [generation check]
V -> S : wait_readable / receive_some -> buffer_
alt EOF or fatal error (e.g. ECONNRESET)
  V -> V : release_slot()
  V --> App : not_configured / system_call_error
else data
  V -> D : invoke(slot.callback, handle, peer, buffer_, n)
  D -> App : callback(...)
  App --> D : returns
  V -> V : if slot.closing -> release_slot()
  V --> App : ok
end
@enduml
```

UDP servers use `process_datagram()` instead. It receives one datagram from the shared socket, looks up the source in the slot table, activates a slot for a new source, and dispatches. When the table is full the datagram is dropped, `full` is returned, and the optional `source` output reports the sender.

### 4.3 Slot lifecycle and handle validity

A handle is `{index, generation}`. Every release bumps the slot's generation, so a stale handle can never address a reused slot.

```plantuml
@startuml
skinparam shadowing false
[*] --> Free
Free --> Active : activate_slot()\n(accept / new UDP peer)\ngeneration++
Active --> Free : close_client() / EOF /\nfatal recv error\nrelease_slot(), generation++
Active --> Closing : close_client() or close()\nwhile its callback is running
Closing --> Free : callback returns\nrelease_slot(), generation++
note right of Closing
  find_slot() rejects Closing slots,
  so the callback sees the handle
  as already closed (not_found)
end note
@enduml
```

### 4.4 Safe self-modification inside callbacks

A callback runs in place, so a `mutable` lambda keeps its state across calls. Destroying or overwriting the running callable would free its captures mid-call, so the dispatcher records which callback is running and defers changes aimed at it.

```plantuml
@startuml
skinparam shadowing false
participant "owner (client/server)" as O
participant "callback_dispatcher" as D
participant "target callback" as T

O -> D : invoke(target, args...)
D -> D : running_ = &target
D -> T : operator()(args...)
T -> D : clear(target) / replace(target, next)
note right of D
  is_running(target) -> store in pending_,
  set action_ (last request wins)
end note
T --> D : returns
D -> D : running_ = nullptr
D -> T : apply replace (move pending_) or clear
D --> O : done
@enduml
```

Rules enforced for every callback:

- It may replace or clear its own callback, close its own peer (or the whole server), send, and manage other peers. These changes take effect when it returns.
- `process_once()` / `process_client()` / `process_datagram()` called from a callback return `invalid_config`, because the shared receive buffer is still in use.
- It must not move or destroy the client or server that is running it.

## 5. Memory model

```plantuml
@startuml
skinparam shadowing false
rectangle "basic_server<Protocol, Max, Buf, Cb>" {
  rectangle "listener_ : basic_socket" as L
  rectangle "slots_[Max]\n(socket + endpoint + callback<Cb> + generation + flags)" as Sl
  rectangle "default_callback_ : function<Cb>" as Dc
  rectangle "dispatcher_ (one pending function<Cb>)" as Dp
  rectangle "buffer_[Buf]  (shared by all slots)" as B
}
@enduml
```

- Everything is sized at compile time; nothing is placed on the stack per call.
- A callable larger than `CallbackStorageSize` is a compile-time `static_assert`, not a runtime error.
- The dispatcher adds one extra callback's storage per owner (one per client, one per server), not one per slot.

## 6. Status mapping

| Situation | Status |
|---|---|
| Success | `ok` |
| Non-blocking read with no data | `empty` |
| Non-blocking write that cannot proceed | `full` |
| Wait expired / connect still pending | `time_out` |
| Server out of client slots (accept, or new UDP peer) | `full` |
| Datagram larger than the receive buffer (dropped) | `data_loss` |
| Closed socket, or no POSIX sockets on this target | `not_configured` |
| Wrong protocol for the operation, re-entrant `process_*` | `invalid_config` |
| Family mismatch, null buffer with non-zero size, bad mode | `invalid_argument` |
| Stale or unknown `client_handle` | `not_found` |
| Null function pointer passed as a callback | `invalid_callback` |
| Failed system call (errno in `system_error`) | `system_call_error` |
| TCP peer EOF (`process_once` / `process_client`) | `not_configured`, and the connection is closed |

## 7. Waiting on many clients

The server does not run a loop. To wait on many clients with one `poll()`, register the descriptors with `castle::io::fd::fd_event_manager`:

```plantuml
@startuml
skinparam shadowing false
actor App
participant "basic_server" as V
participant "fd_event_manager" as M

loop i in 0 .. client_capacity()
  App -> V : client_at(i)
  V --> App : handle (or invalid)
  App -> V : client_native_handle(handle)
  V --> App : fd (TCP: own fd, UDP: listener fd)
  App -> M : add_fd(fd, poll_read, callback)
end
App -> M : wait_and_process(timeout)
M -> App : callback(fd, revents)
App -> V : process_client(handle) / process_datagram()
@enduml
```

## 8. Design decisions

- **One host-ABI header.** Concentrating `sockaddr`, `recvfrom` and friends in `detail/native.hpp` keeps the public classes free of preprocessor branches and gives every platform difference one home.
- **Atomic descriptor flags.** Sockets are created with `SOCK_CLOEXEC | SOCK_NONBLOCK` and accepted with `accept4()` where available, with an `fcntl` fallback that reports errors, so no fork or exec race can leak a descriptor.
- **No `SIGPIPE`.** `MSG_NOSIGNAL` where defined, otherwise `SO_NOSIGPIPE` on each new socket.
- **Truncation is visible.** `recvfrom` plus `MSG_TRUNC` (datagram sockets only) makes an oversize UDP datagram `data_loss` for both `receive_some` and `receive_from`. A target without `MSG_TRUNC`/`ZSOCK_MSG_TRUNC` cannot report it.
- **`SO_REUSEADDR` on TCP listeners only**, so a second UDP server cannot silently share a port.
- **Generation-checked handles** instead of pointers: a stale handle fails with `not_found` after the slot is reused.
- **Server pending-connection policy.** `accept_client` checks capacity first. At capacity it returns `full` immediately and leaves the connection in the OS accept queue.
- **No eviction policy for UDP peers.** The library reports `full` and the rejected source, and exposes `client_at()` / `close_client()`. The application decides whom to evict.
- **No threads in the library.** Tests and samples may use threads; the library itself never does.

## 9. Thread-safety and portability

- Clients, servers and sockets have no internal synchronization. Serialize access to one instance externally. The `castle/sync` primitives can be used for that.
- Linux (`CASTLE_USING_LINUX_BSD_SOCKET`) and Zephyr (`CASTLE_USING_ZEPHYR_BSD_SOCKET`) run the same code in `detail/native.hpp`; only its headers differ. On Zephyr, enable `CONFIG_POSIX_API`, BSD sockets (`CONFIG_NET_SOCKETS`) and the transport you use (`CONFIG_NET_TCP` / `CONFIG_NET_UDP`). Zephyr-specific limits: IPv6 scope is an 8-bit interface index, and `shutdown()` and `TCP_NODELAY` may be no-ops on older releases.
- Without `CASTLE_USING_POSIX_APIS`, every call returns `not_configured`; code using the API still compiles.

## 10. File map

| Path | Content |
|---|---|
| `endpoint.hpp` | `address_family`, `endpoint` |
| `socket.hpp` | `socket_protocol`, `shutdown_mode`, `basic_socket`, `tcp_socket`, `udp_socket` |
| `client.hpp` | `basic_client`, `tcp_client`, `udp_client` |
| `server.hpp` | `basic_server`, `tcp_server`, `udp_server`, `client_handle` |
| `net.hpp` | Umbrella include |
| `detail/native.hpp` | BSD calls as `castle::status` (internal) |
| `detail/callback_dispatcher.hpp` | In-place callback invocation with deferred self-modification (internal) |

Samples are in `samples/castle_ext/net/`, tests in `test/castle_ext/net/`.

## 11. Rendering the diagrams

The diagrams are plain PlantUML. Render them with the VS Code PlantUML extension, `plantuml README.md`, or any PlantUML server. If your Markdown viewer does not render ` ```plantuml ` fences, paste each block into <https://www.plantuml.com/plantuml>.
