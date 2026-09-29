# UBX Parser Decoder Registry

## Overview

`decoder_registry.hpp` provides the UBX-specific facade over CASTLE's protocol-agnostic decoder registry infrastructure.

It binds the generic decoder machinery to `castle::protocols::ubx::message_view`, so UBX applications do not need to repeat the raw-view type when declaring message decoders or registries.

The facade is intentionally thin: message matching, decoding, subscriber storage, tuple-based dispatch, and callback wiring are implemented by [`castle::parsers::decoder_registry`](../decoder_registry.md). The UBX layer contributes only the protocol type binding and compile-time validation at the parser attachment boundary.

This design keeps the UBX API concise while allowing the same architecture to be reused by NMEA and future protocol-specific parser facades without duplicating dispatch logic.

## Header

`#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/config.hpp`](../../../core/config.md)
- [`core/traits.hpp`](../../../core/traits.md)
- [`core/types.hpp`](../../../core/types.md)
- [`parsers/decoder_registry.hpp`](../decoder_registry.md)
- [`protocols/ubx/ubx.hpp`](../../protocols/ubx/ubx.md)

## Public API

| API | Description |
| --- | --- |
| `raw_view_type` | Alias for `castle::protocols::ubx::message_view`, the validated zero-copy view consumed by UBX decoders. |
| `message_decoder<Message, MaxSubscribers, StorageSize, StorageAlignment>` | UBX-bound alias of the generic `castle::parsers::message_decoder`. |
| `decoder_registry<Decoders...>` | UBX-bound alias of the generic fixed-capacity `castle::parsers::decoder_registry`. |
| `default_decoder_registry<MaxSubscribers, StorageSize, StorageAlignment>` | Pre-configured UBX decoder registry covering all built-in UBX message decoders. |
| `dispatch_result` | Alias for the generic dispatch outcome containing `matched` and `decoded` counts. |
| `attach(Parser&, Registry&)` | Connects a UBX parser's message callback to a UBX decoder registry with compile-time raw-view validation. |

### `raw_view_type`

`raw_view_type` is the protocol binding that differentiates the UBX facade from the generic registry:

```cpp
using raw_view_type = castle::protocols::ubx::message_view;
```

Every `ubx_parser::message_decoder` and `ubx_parser::decoder_registry` ultimately uses this type.

### `message_decoder`

```cpp
template <
    typename Message,
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using message_decoder = castle::parsers::message_decoder<
    Message,
    raw_view_type,
    MaxSubscribers,
    StorageSize,
    StorageAlignment>;
```

`Message` must provide the generic decoder contract:

```cpp
static bool matches(const raw_view_type& raw) noexcept;
static bool decode(const raw_view_type& raw, Message& out) noexcept;
```

The decoder owns a bounded signal. A subscriber is notified only when `Message::decode()` succeeds.

### `decoder_registry`

```cpp
template <typename... Decoders>
using decoder_registry = castle::parsers::decoder_registry<
    raw_view_type,
    Decoders...>;
```

The registry is a fixed compile-time set of concrete decoder instances. Dispatch evaluates each registered decoder's `matches()` predicate and invokes `handle()` only for matching message types.

### `default_decoder_registry`

```cpp
template <
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using default_decoder_registry = castle::parsers::decoder_registry<...>;

```

`default_decoder_registry` is a batteries-included, compile-time registry that packs decoders for all built-in UBX message types (ACK, CFG, ESF, INF, MON, NAV, NAV2, RXM, SEC, TIM, UPD).

It allows applications to listen to any standard UBX message out of the box without needing to manually specify a custom decoder list.

### `dispatch_result`

The result contains two counters:

| Member | Meaning |
| --- | --- |
| `matched` | Number of registered decoders whose `matches()` predicate returned `true`. |
| `decoded` | Number of matching decoders whose `decode()` operation succeeded. |

More than one decoder may match a single raw UBX message when an application deliberately registers overlapping decoder types.

### `attach()`

`attach()` connects the parser callback directly to `registry.dispatch()`.

Before the connection is installed, two compile-time checks are performed:

1. `Parser::message_type` must be exactly `protocols::ubx::message_view`.
2. `Registry::raw_view_type` must be exactly `protocols::ubx::message_view`.

There is no runtime protocol-type test and no type-erased callback object introduced by the facade.

## Template Parameters

### `message_decoder`

| Parameter | Default | Meaning |
| --- | --- | --- |
| `Message` | — | Concrete UBX message structure exposing `matches()` and `decode()`. |
| `MaxSubscribers` | `1U` | Maximum simultaneously connected subscribers for this message decoder. |
| `StorageSize` | `castle::inplace_storage_reserved` | Inline callback storage reserved by the signal. |
| `StorageAlignment` | `castle::inplace_alignment_default` | Alignment used for inline callback storage. |

### `decoder_registry`

| Parameter | Meaning |
| --- | --- |
| `Decoders...` | UBX-bound `message_decoder` types stored in the compile-time tuple. |

The number of registered decoders is `sizeof...(Decoders)` and therefore known at compile time.

## Usage Example

Applications can either use `default_decoder_registry` for zero-configuration setup, or build a lean custom registry for minimum memory footprint:

```cpp
#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"
#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

// Option 1: Use default_decoder_registry to cover all built-in UBX messages
using registry_type = castle::parsers::ubx_parser::default_decoder_registry<>;

// Option 2: Define a custom minimal registry using convenience decoder aliases
using minimal_registry_type = castle::parsers::ubx_parser::decoder_registry<
    castle::parsers::ubx_parser::nav_pvt_decoder<>,
    castle::parsers::ubx_parser::ack_ack_decoder<>>;

castle::parsers::ubx_parser::ubx_parser<> parser;
registry_type registry{};

auto connection = registry.decoder<
    castle::protocols::ubx::messages::nav_pvt>()
    .connect([](const auto& pvt) {
        (void)pvt;
        // Consume decoded NAV-PVT data.
    });

castle::parsers::ubx_parser::attach(parser, registry);

```

The registry, decoder, subscriber storage, and callback connection are all bounded by their compile-time configuration.

## Relationship to the Generic Registry

The UBX facade is intentionally not a second implementation of the dispatch algorithm.

```text
UBX parser
    │
    │ message_view
    ▼
ubx_parser::attach()
    │
    ▼
ubx_parser::decoder_registry<...>
    │
    ▼
parsers::decoder_registry<message_view, ...>
    │
    ├── message_decoder<Message, message_view, ...>
    │       ├── Message::matches()
    │       ├── Message::decode()
    │       └── bounded signal subscribers
    │
    └── compile-time tuple dispatch
```

This separation provides a reusable protocol-neutral core while keeping protocol-specific user code explicit and type-safe.

## Constraints & Notes

- Header-only; no `.cpp` implementation is required.
- No heap allocation, exceptions, RTTI, or virtual dispatch is introduced by this facade.
- The facade adds no per-message runtime metadata; the UBX raw-view type is fixed at compile time.
- `attach()` captures the registry by reference through the generic callback adapter. The registry must remain alive while the parser callback is installed.
- `message_decoder` inherits the generic decoder behavior: subscribers are notified only after successful deserialization.
- The registry itself has fixed storage determined entirely by its decoder pack and each decoder's signal capacity/storage parameters.
- This header is intentionally protocol-specific. Future parser families should expose equivalent facades bound to their own validated raw-view type rather than adding protocol conditionals to the generic registry.
