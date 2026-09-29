// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file decoder_registry.hpp
 * @brief Protocol-agnostic, compile-time decoder dispatch backend for Castle.
 *
 * This header is the shared implementation backend reused by protocol-specific
 * parser facades (UBX, NMEA, and future protocols). It contains no protocol-
 * specific knowledge: a "message" is any type exposing
 *
 * @code
 * static bool Message::matches(const RawView& raw) noexcept;
 * static bool Message::decode(const RawView& raw, Message& out) noexcept;
 * @endcode
 *
 * where `RawView` is the parser's validated, zero-copy view type (e.g.
 * `castle::protocols::ubx::message_view` or
 * `castle::protocols::nmea::message_view`).
 *
 * `message_decoder<Message, RawView>` pairs one such `Message` type with a
 * fixed-capacity `castle::sigslot::signal` so interested code can subscribe
 * without heap allocation or virtual dispatch. `decoder_registry<RawView,
 * Decoders...>` holds a `castle::tuple` of decoders and dispatches a single
 * validated `RawView` to every decoder whose `matches()` predicate is true,
 * fully unrolled at compile time (no vtable, no runtime loop over a type-
 * erased list).
 *
 * Protocol-specific users should normally include
 * `castle_ext/parsers/ubx_parser/decoder_registry.hpp` or
 * `castle_ext/parsers/nmea_parser/decoder_registry.hpp`, which bind `RawView`
 * automatically and keep protocol APIs separate.
 *
 * @code
 * using nav_pvt_decoder = castle::parsers::message_decoder<
 *     castle::protocols::ubx::messages::nav_pvt,
 *     castle::protocols::ubx::message_view>;
 *
 * castle::parsers::decoder_registry<
 *     castle::protocols::ubx::message_view,
 *     nav_pvt_decoder> registry;
 *
 * auto connection = registry.decoder<castle::protocols::ubx::messages::nav_pvt>()
 *     .connect([](const auto& pvt) { ... });
 *
 * castle::parsers::attach(parser, registry); // wires parser.set_message_callback()
 * @endcode
 */
#ifndef CASTLE_EXT_PARSERS_DECODER_REGISTRY_HPP
#define CASTLE_EXT_PARSERS_DECODER_REGISTRY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/tuple.hpp"
#include "castle/utility/integral_sequence.hpp"
#include "castle/utility/forward.hpp"
#include "castle/events/sigslot.hpp"

namespace castle
{
namespace parsers
{

namespace detail
{

/**
 * Checks if a type `T` has a static `matches` method with the signature:
 * `static bool matches(const RawView&)`.
 */
template<typename T, typename RawView, typename = void>
struct has_matches
    : castle::meta::false_type
{
};

template<typename T, typename RawView>
struct has_matches<T, RawView, castle::meta::void_t<decltype(T::matches(castle::meta::declval<CASTLE_CONST RawView&>()))>>
    : castle::integral_constant<
        bool,
        castle::meta::is_same<
            decltype(T::matches(castle::meta::declval<CASTLE_CONST RawView&>())),
            bool
        >::value
      >
{
};

/**
 * Checks if a type `T` has a static `decode` method with the signature:
 * `static bool decode(const RawView&, T&)`.
 */
template<typename T, typename RawView, typename = void>
struct has_decode
    : castle::meta::false_type
{
};

template<typename T, typename RawView>
struct has_decode<
            T,
            RawView,
            castle::meta::void_t<
                decltype(
                    T::decode(
                        castle::meta::declval<CASTLE_CONST RawView&>(),
                        castle::meta::declval<T&>()
                    )
                )
            >
       >
    : castle::integral_constant<
        bool,
        castle::meta::is_same<
            decltype(
                T::decode(
                    castle::meta::declval<CASTLE_CONST RawView&>(),
                    castle::meta::declval<T&>()
                )
            ),
            bool
        >::value>
{
};

} // namespace detail

/**
 * @brief Pairs a decodable message type with a bounded notification signal.
 *
 * @tparam Message Decoded message type. Must expose `static bool matches(const
 * RawView&)` and `static bool decode(const RawView&, Message&)`.
 * @tparam RawView Validated, zero-copy parser view type passed to `matches()`/`decode()`.
 * @tparam MaxSubscribers Maximum simultaneously connected subscribers.
 * @tparam StorageSize Inline storage reserved for each subscriber callback.
 * @tparam StorageAlignment Alignment used for each subscriber callback.
 *
 * @note `handle()` only emits the signal when `decode()` succeeds; a
 * malformed payload for an otherwise-matching message is silently dropped
 * rather than notifying subscribers with a half-decoded value.
 */
template <
    typename Message,
    typename RawView,
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
class message_decoder CASTLE_FINAL
{
    static_assert(detail::has_decode<Message, RawView>::value,
                  "Message must have a static decode method with the signature: static bool decode(const RawView&, Message&).");
    static_assert(detail::has_matches<Message, RawView>::value,
                  "Message must have a static matches method with the signature: static bool matches(const RawView&).");

public:
    using message_type = Message;
    using raw_view_type = RawView;
    using signal_type = castle::sigslot::signal<
        MaxSubscribers,
        void(CASTLE_CONST Message&),
        StorageSize,
        StorageAlignment>;
    using connection_type = typename signal_type::connection_type;

    message_decoder() CASTLE_DEFAULT;

    message_decoder(CASTLE_CONST message_decoder&) CASTLE_DELETE;
    message_decoder& operator=(CASTLE_CONST message_decoder&) CASTLE_DELETE;
    message_decoder(message_decoder&&) CASTLE_DELETE;
    message_decoder& operator=(message_decoder&&) CASTLE_DELETE;

    /**
     * @brief Reports whether `raw` is recognized by `Message`.
     */
    CASTLE_NODISCARD bool matches(CASTLE_CONST RawView& raw) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Message::matches(raw);
    }

    /**
     * @brief Decodes `raw` and notifies subscribers on success.
     * @return `true` when `Message::decode()` succeeded.
     */
    bool handle(CASTLE_CONST RawView& raw) CASTLE_NOEXCEPT
    {
        Message decoded{};
        if (!Message::decode(raw, decoded)) // LCOV_EXCL_BR_LINE
        {
            return false;
        }

        signal_.emit(decoded);
        return true;
    }

    /**
     * @brief Subscribes any callable or member function supported by `signal_type::connect()`.
     */
    template <typename... Callback>
    connection_type connect(Callback&&... callback)
    {
        return signal_.connect(CASTLE_FORWARD<Callback>(callback)...);
    }

    /**
     * @brief Returns the number of currently connected subscribers.
     */
    CASTLE_NODISCARD castle::size_type subscriber_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return signal_.size();
    }

private:
    signal_type signal_{};
};

/**
 * @brief Outcome of dispatching one validated message to a `decoder_registry`.
 */
struct dispatch_result
{
    /** @brief Number of decoders whose `matches()` predicate was true. */
    castle::size_type matched = 0U;
    /** @brief Number of matching decoders whose `decode()` also succeeded. */
    castle::size_type decoded = 0U;
};

namespace detail
{

/**
 * @brief Finds the pack index of the decoder whose `message_type` is `Message`.
 * @warning `Message` must appear as exactly one decoder's `message_type` in `Decoders...`.
 */
template <typename Message, typename... Decoders>
struct decoder_index_for;

template <typename Message, typename Head, typename... Tail>
struct decoder_index_for<Message, Head, Tail...>
{
    static CASTLE_CONSTEXPR castle::size_type value =
        castle::meta::is_same<typename Head::message_type, Message>::value
            ? 0U
            : (1U + decoder_index_for<Message, Tail...>::value);
};

/** @brief Terminal case: `Message` was not found anywhere in the decoder pack. */
template <typename Message>
struct decoder_index_for<Message>
{
    static CASTLE_CONSTEXPR castle::size_type value = 0U;
};

} // namespace detail

/**
 * @brief Fixed, compile-time set of `message_decoder` instances dispatched by
 * matching rather than virtual dispatch.
 *
 * @tparam RawView Validated parser view type shared by every decoder in `Decoders...`.
 * @tparam Decoders Decoder types, each normally a `message_decoder<Message, RawView, ...>`.
 *
 * @note All decoders are evaluated for every dispatched message (`matches()`
 * is cheap: a class/ID or sentence-type comparison), so more than one
 * decoder may legitimately handle the same message, mirroring the reference
 * libraries' "multiple subscribers per message type" behavior.
 */
template <typename RawView, typename... Decoders>
class decoder_registry CASTLE_FINAL
{
public:
    using raw_view_type = RawView;

    decoder_registry() CASTLE_DEFAULT;

    decoder_registry(CASTLE_CONST decoder_registry&) CASTLE_DELETE;
    decoder_registry& operator=(CASTLE_CONST decoder_registry&) CASTLE_DELETE;
    decoder_registry(decoder_registry&&) CASTLE_DELETE;
    decoder_registry& operator=(decoder_registry&&) CASTLE_DELETE;

    /**
     * @brief Returns the compile-time number of registered decoders.
     */
    static CASTLE_CONSTEXPR castle::size_type decoder_count() CASTLE_NOEXCEPT
    {
        return sizeof...(Decoders);
    }

    /**
     * @brief Dispatches `raw` to every decoder whose `matches()` predicate is true.
     */
    dispatch_result dispatch(CASTLE_CONST RawView& raw) CASTLE_NOEXCEPT
    {
        dispatch_result result{};
        dispatch_impl(raw, result, castle::sequence::index_sequence_for<Decoders...>{});
        return result;
    }

    /**
     * @brief Returns the decoder instance by pack index.
     */
    template <castle::size_type Index>
    CASTLE_NODISCARD auto& decoder_at() CASTLE_NOEXCEPT
    {
        return castle::get<Index>(decoders_);
    }

    /**
     * @brief Returns the decoder instance whose `message_type` is `Message`.
     * @warning `Message` must appear as exactly one decoder's `message_type`.
     */
    template <typename Message>
    CASTLE_NODISCARD auto& decoder() CASTLE_NOEXCEPT
    {
        static_assert(sizeof...(Decoders) > 0U,
                      "decoder_registry<RawView> has no registered decoders");
        return castle::get<detail::decoder_index_for<Message, Decoders...>::value>(decoders_);
    }

private:
    template <castle::size_type Index>
    void dispatch_one(CASTLE_CONST RawView& raw, dispatch_result& result) CASTLE_NOEXCEPT
    {
        auto& current = castle::get<Index>(decoders_);
        if (current.matches(raw))
        {
            ++result.matched;
            if (current.handle(raw))
            {
                ++result.decoded;
            }
        }
    }

    template <castle::size_type... Indices>
    void dispatch_impl(
        CASTLE_CONST RawView& raw,
        dispatch_result& result,
        castle::sequence::index_sequence<Indices...>) CASTLE_NOEXCEPT
    {
        (dispatch_one<Indices>(raw, result), ...);
    }

    castle::tuple<Decoders...> decoders_{};
};

/**
 * @brief Wires a streaming parser's message callback to a decoder registry.
 *
 * @tparam Parser Any `*_parser` exposing `set_message_callback()`.
 * @tparam Registry A `decoder_registry<RawView, ...>` whose `raw_view_type`
 * matches `Parser`'s message callback argument.
 *
 * @warning `registry` must outlive `parser` (or be cleared from the parser
 * first); the installed callback captures it by reference.
 */
template <typename Parser, typename Registry>
void attach(Parser& parser, Registry& registry) CASTLE_NOEXCEPT
{
    parser.set_message_callback(
        [&registry](CASTLE_CONST typename Registry::raw_view_type& raw) CASTLE_NOEXCEPT
        {
            registry.dispatch(raw);
        });
}

} // namespace parsers
} // namespace castle

#endif // CASTLE_EXT_PARSERS_DECODER_REGISTRY_HPP
