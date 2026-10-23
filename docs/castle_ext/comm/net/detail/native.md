# Network Native Socket Layer

## Overview
`castle::comm::net::detail` holds the only code that touches the host BSD socket ABI. Each function performs one operation (`socket()`, `bind()`, `connect()`, `accept4()`/`accept()`, `send()`/`sendto()`, `recvfrom()`, option calls, `getsockname()`/`getpeername()`) and translates the result into a `castle::status`. `basic_socket` validates state and arguments and then forwards here, so it has no preprocessor branches.

This is an implementation detail; applications use `basic_socket`, `basic_client` and `basic_server`.

## Header
`#include "castle_ext/comm/net/detail/native.hpp"`

## Dependencies
- [`endpoint.hpp`](../endpoint.md)
- [`castle_ext/io/fd/control.hpp`](../../../../../include/castle_ext/io/fd/control.hpp) (`clear_system_error`, `capture_system_error`, `would_block`)
- [`castle_ext/io/fd/descriptor.hpp`](../../../../../include/castle_ext/io/fd/descriptor.hpp) (`basic_fd`)
- [`castle_ext/io/fd/poll.hpp`](../../../../../include/castle_ext/io/fd/poll.hpp) (`wait_fd`)

## Behavior
- Callers clear the optional `system_error` first; a failing call stores its `errno` there and returns `system_call_error`. The helpers are the ones `castle::io::fd` already uses, so the net layer follows the same convention.
- `open_socket()` and `accept_socket()` set close-on-exec and non-blocking mode atomically (`SOCK_CLOEXEC`, `SOCK_NONBLOCK`, `accept4()`) where available. Otherwise they fall back to `fcntl()` and report its failure, closing the new descriptor; `not_configured` from a target without descriptor flags is ignored because it has no `exec()` to leak into.
- Targets without `MSG_NOSIGNAL` get `SO_NOSIGPIPE` on every new socket where it exists, so a send to a reset connection cannot raise `SIGPIPE`.
- `connect_socket()` reports only `EINPROGRESS` and `EALREADY` as pending (`time_out`). `finish_connect_socket()` waits for writability, then checks both `SO_ERROR` and `getpeername()`, so a connect that never completed is never reported as success.
- `receive_bytes()` uses `recvfrom()`. For datagram sockets it passes `MSG_TRUNC`, which makes the call return the datagram's real length; a length above the buffer sets `truncated` and `received` is clamped to the buffer. Stream sockets never get the flag, because on Linux it would discard TCP data. Where a target defines neither `MSG_TRUNC` nor `ZSOCK_MSG_TRUNC`, truncation cannot be detected.
- Linux (`CASTLE_USING_LINUX_BSD_SOCKET`) and Zephyr (`CASTLE_USING_ZEPHYR_BSD_SOCKET`) share one body written against the POSIX socket names; only the included headers differ (`<sys/socket.h>` versus `<zephyr/net/socket.h>`). Zephyr needs `CONFIG_POSIX_API` for those names and `CONFIG_NET_SOCKETS` with the transports in use.
- Zephyr has no `SOCK_CLOEXEC`/`accept4()`/`MSG_NOSIGNAL`, so it takes the `fcntl()` fallback for non-blocking mode and needs no `SIGPIPE` handling. It stores an IPv6 scope as an 8-bit interface index, so larger `scope_id()` values are truncated. Whether `shutdown()` has any effect depends on the Zephyr version.
- Without both socket macros every function exists and returns `not_configured`.
