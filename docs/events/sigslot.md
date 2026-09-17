# sigslot

## Overview
`castle::sigslot` provides a classic signal/slot pattern with fixed slot capacity, inline callback storage, and move-only RAII connection handles. It exists for deterministic callback fan-out without heap allocation or STL containers.

## Header
`#include "castle/events/sigslot.hpp"`

## Dependencies
- [`function`](../callbacks/function.md)
- [`array`](../container/array.md)

## Public API
| API | Description |
| --- | --- |
| `enum class signal_error : uint8_t` | Result codes for connect/disconnect operations: `ok`, `full`, `invalid_connection`. |
| `template <size_type MaxSlot, typename Signature, size_type StorageSize, size_type StorageAlignment> class signal` | Fixed-capacity signal storing callbacks by value. `Signature` must be `void(Args...)`. |
| `template <typename Signal, size_type MaxSlot> class signal_connection` | Move-only handle that disconnects one slot on destruction or explicit `disconnect()`. |
| `signal_connection()` | Creates an empty handle. |
| `signal_connection(signal_connection&&)` / `operator=(signal_connection&&)` | Transfers ownership of one live connection and rebinds the slot to the new handle object. |
| `signal_error signal_connection::disconnect()` | Disconnects the represented slot. Repeated calls return `signal_error::invalid_connection`. |
| `bool signal_connection::connected() const` / `explicit operator bool() const` | Reports whether the handle still refers to a live connection. |
| `using signal::callback_type` | Inline callback wrapper stored in each slot. |
| `using signal::connection_type` | Alias for the returned connection handle type. |
| `signal::connect(callback_type&&, signal_error* = nullptr)` | Connects a pre-built callback into the first inactive slot. |
| `signal::connect(Callable&&, signal_error* = nullptr)` | Wraps and connects any supported callable. |
| `signal::connect(object, member, signal_error* = nullptr)` | Connects a non-const member function; the object lifetime remains external. |
| `signal::connect(const object, const_member, signal_error* = nullptr)` | Connects a const member function; the object lifetime remains external. |
| `void signal::emit(Args...)` / `void signal::operator()(Args...)` | Invokes active slots in ascending slot order. |
| `void signal::disconnect_all()` | Disconnects every active slot and invalidates the associated handles. |
| `size_type signal::size() const` / `bool signal::empty() const` | Query the active connection count. |
| `static constexpr size_type signal::capacity()` | Returns `MaxSlot`. |

## Usage Example
See [`samples/sample_sigslot.cpp`](../../samples/sample_sigslot.cpp).

```cpp
castle::sigslot::signal<3U, void(std::uint32_t)> signal;
std::uint32_t total = 0U;

auto connection = signal.connect([&total](std::uint32_t value) {
    total += value;
});

signal.emit(5U);
connection.disconnect();
```

## Constraints & Notes
- The signal stores callbacks inline through `callbacks::function`; Castle performs no heap allocation.
- Only `void`-returning signatures are supported.
- `connect()` reuses the first inactive slot in index order. `emit()` also walks slots in index order.
- `signal_connection` is move-only so exactly one handle object remains responsible for a live slot.
- The component has no internal locking. Do not race connect, disconnect, destruction, and emit without external synchronization.
- `disconnect_all()` and signal destruction invalidate handles by resetting the slot-backed connection object when available.
