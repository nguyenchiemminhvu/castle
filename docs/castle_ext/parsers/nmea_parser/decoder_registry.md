# NMEA Parser Decoder Registry

## Overview

`decoder_registry.hpp` provides the NMEA-specific facade over CASTLE's protocol-agnostic decoder registry infrastructure.

It binds the generic decoder machinery to `castle::protocols::nmea::message_view`, so NMEA applications do not need to repeat the raw-view type when declaring sentence decoders or registries.

The facade is intentionally thin: sentence matching, decoding, subscriber storage, tuple-based dispatch, and callback wiring are implemented by [`castle::parsers::decoder_registry`](../decoder_registry.md). The NMEA layer contributes only the protocol type binding and compile-time validation at the parser attachment boundary.

This keeps the NMEA API concise while allowing the same architecture to be reused by UBX and future protocol-specific parser facades without duplicating dispatch logic.

## Header

`#include "castle_ext/parsers/nmea_parser/decoder_registry.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/config.hpp`](../../../core/config.md)
- [`core/traits.hpp`](../../../core/traits.md)
- [`core/types.hpp`](../../../core/types.md)
- [`parsers/decoder_registry.hpp`](../decoder_registry.md)
- [`protocols/nmea/nmea.hpp`](../../protocols/nmea/nmea.md)

## Public API

| API | Description |
| --- | --- |
| `raw_view_type` | Alias for `castle::protocols::nmea::message_view`, the validated zero-copy view consumed by NMEA decoders. |
| `message_decoder<Message, MaxSubscribers, StorageSize, StorageAlignment>` | NMEA-bound alias of the generic `castle::parsers::message_decoder`. |
| `decoder_registry<Decoders...>` | NMEA-bound alias of the generic fixed-capacity `castle::parsers::decoder_registry`. |
| `default_decoder_registry<MaxSubscribers, StorageSize, StorageAlignment>` | Pre-configured NMEA decoder registry covering all built-in NMEA message decoders. |
| `dispatch_result` | Alias for the generic dispatch outcome containing `matched` and `decoded` counts. |
| `attach(Parser&, Registry&)` | Connects an NMEA parser's sentence callback to an NMEA decoder registry with compile-time raw-view validation. |

### `raw_view_type`

`raw_view_type` is the protocol binding that differentiates the NMEA facade from the generic registry:

```cpp
using raw_view_type = castle::protocols::nmea::message_view;
```

Every `nmea_parser::message_decoder` and `nmea_parser::decoder_registry` ultimately uses this type.

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

The registry is a fixed compile-time set of concrete decoder instances. Dispatch evaluates each registered decoder's `matches()` predicate and invokes `handle()` only for matching sentence types.

### `default_decoder_registry`

For applications that need to parse a wide variety of standard sentences, a fully populated registry is provided out of the box:

```cpp
template <
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using default_decoder_registry =
    castle::parsers::decoder_registry<
        protocols::nmea::message_view,
        dtm_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gbs_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gga_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gll_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gns_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gsa_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gst_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        gsv_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        rmc_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        vtg_decoder<MaxSubscribers, StorageAlignment StorageSize,>,
        zda_decoder<MaxSubscribers, StorageAlignment StorageSize,>
    >;
```

### `dispatch_result`

The result contains two counters:

| Member | Meaning |
| --- | --- |
| `matched` | Number of registered decoders whose `matches()` predicate returned `true`. |
| `decoded` | Number of matching decoders whose `decode()` operation succeeded. |

More than one decoder may match a single raw NMEA sentence when an application deliberately registers overlapping decoder types.

### `attach()`

`attach()` connects the parser callback directly to `registry.dispatch()`.

Before the connection is installed, two compile-time checks are performed:

1. `Parser::message_type` must be exactly `protocols::nmea::message_view`.
2. `Registry::raw_view_type` must be exactly `protocols::nmea::message_view`.

There is no runtime protocol-type test and no type-erased callback object introduced by the facade.

## Template Parameters

### `message_decoder`

| Parameter | Default | Meaning |
| --- | --- | --- |
| `Message` | — | Concrete NMEA sentence structure exposing `matches()` and `decode()`. |
| `MaxSubscribers` | `1U` | Maximum simultaneously connected subscribers for this sentence decoder. |
| `StorageSize` | `castle::inplace_storage_reserved` | Inline callback storage reserved by the signal. |
| `StorageAlignment` | `castle::inplace_alignment_default` | Alignment used for inline callback storage. |

### `decoder_registry`

| Parameter | Meaning |
| --- | --- |
| `Decoders...` | NMEA-bound `message_decoder` types stored in the compile-time tuple. |

The number of registered decoders is `sizeof...(Decoders)` and therefore known at compile time.

## Usage Example

## Usage Example

Using the built-in `default_decoder_registry` allows an application to quickly subscribe to any supported NMEA sentence without manually listing every decoder type:

```cpp
#include "castle_ext/parsers/nmea_parser/decoder_registry.hpp"
#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

castle::parsers::nmea_parser::nmea_parser<> parser;
castle::parsers::nmea_parser::default_decoder_registry<> registry{};

// Connect a callback specifically to the GGA decoder within the registry
auto connection = registry.decoder<
    castle::protocols::nmea::messages::gga>()
    .connect([](const auto& gga) {
        (void)gga;
        // Consume decoded GGA data.
    });

// Wire the parser to dispatch sentences into the registry
castle::parsers::nmea_parser::attach(parser, registry);
```

Alternatively, a small NMEA application can register only the sentence types it needs:

```cpp
#include "castle_ext/parsers/nmea_parser/decoder_registry.hpp"
#include "castle_ext/protocols/nmea/messages/gga.hpp"
#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

using gga_decoder =
    castle::parsers::nmea_parser::message_decoder<
        castle::protocols::nmea::messages::gga>;

using registry_type =
    castle::parsers::nmea_parser::decoder_registry<gga_decoder>;

castle::parsers::nmea_parser::nmea_parser<> parser;
registry_type registry{};

auto connection = registry.decoder<
    castle::protocols::nmea::messages::gga>()
    .connect([](const auto& gga) {
        (void)gga;
        // Consume decoded GGA data.
    });

castle::parsers::nmea_parser::attach(parser, registry);
```

The registry, decoder, subscriber storage, and callback connection are all bounded by their compile-time configuration.

## Relationship to the Generic Registry

The NMEA facade is intentionally not a second implementation of the dispatch algorithm.

```text
NMEA parser
    │
    │ message_view
    ▼
nmea_parser::attach()
    │
    ▼
nmea_parser::decoder_registry<...>
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
- The facade adds no per-sentence runtime metadata; the NMEA raw-view type is fixed at compile time.
- `attach()` captures the registry by reference through the generic callback adapter. The registry must remain alive while the parser callback is installed.
- `message_decoder` inherits the generic decoder behavior: subscribers are notified only after successful deserialization.
- The registry itself has fixed storage determined entirely by its decoder pack and each decoder's signal capacity/storage parameters.
- This header is intentionally protocol-specific. Future parser families should expose equivalent facades bound to their own validated raw-view type rather than adding protocol conditionals to the generic registry.
