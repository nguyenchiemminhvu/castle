// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file decoder_registry.hpp
 * @brief NMEA-specific decoder registry facade for Castle.
 *
 * This header binds the protocol-agnostic decoder infrastructure to
 * `castle::protocols::nmea::message_view`, so NMEA users do not need to
 * repeat the raw-view type in every decoder or registry declaration.
 *
 * The implementation remains in `castle_ext/parsers/decoder_registry.hpp`.
 * This layer intentionally contains only type aliases and a zero-overhead
 * `attach()` adapter, making it straightforward to add equivalent facades
 * for future protocols without duplicating dispatch machinery.
 *
 * @code
 * using gga_decoder = castle::parsers::nmea_parser::message_decoder<
 *     castle::protocols::nmea::messages::gga>;
 *
 * castle::parsers::nmea_parser::decoder_registry<gga_decoder> registry;
 *
 * auto connection = registry.decoder<
 *     castle::protocols::nmea::messages::gga>()
 *     .connect([](const auto& gga) { (void)gga; });
 *
 * castle::parsers::nmea_parser::attach(parser, registry);
 * @endcode
 */
#ifndef CASTLE_EXT_PARSERS_NMEA_PARSER_DECODER_REGISTRY_HPP
#define CASTLE_EXT_PARSERS_NMEA_PARSER_DECODER_REGISTRY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/messages/messages.hpp"
#include "castle_ext/parsers/decoder_registry.hpp"

namespace castle
{
namespace parsers
{
namespace nmea_parser
{

/**
 * @brief Validated raw sentence view consumed by all NMEA decoders.
 */
using raw_view_type = protocols::nmea::message_view;

/**
 * @brief NMEA-bound version of the generic message decoder.
 *
 * @tparam Message Decoded NMEA sentence type exposing `matches()` and `decode()`.
 * @tparam MaxSubscribers Maximum simultaneously connected subscribers.
 * @tparam StorageSize Inline storage reserved for each subscriber callback.
 * @tparam StorageAlignment Alignment used for each subscriber callback.
 */
template <
    typename Message,
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using message_decoder = ::castle::parsers::message_decoder<
    Message,
    raw_view_type,
    MaxSubscribers,
    StorageSize,
    StorageAlignment>;

#define CASTLE_EXT_NMEA_DECODER_ALIAS(name, type) \
    template <\
        castle::size_type MaxSubscribers = 1U,\
        castle::size_type StorageSize = castle::inplace_storage_reserved,\
        castle::size_type StorageAlignment = castle::inplace_alignment_default>\
    using name = message_decoder<type, MaxSubscribers, StorageSize, StorageAlignment>;

CASTLE_EXT_NMEA_DECODER_ALIAS(dtm_decoder, protocols::nmea::messages::dtm);
CASTLE_EXT_NMEA_DECODER_ALIAS(gbs_decoder, protocols::nmea::messages::gbs);
CASTLE_EXT_NMEA_DECODER_ALIAS(gga_decoder, protocols::nmea::messages::gga);
CASTLE_EXT_NMEA_DECODER_ALIAS(gll_decoder, protocols::nmea::messages::gll);
CASTLE_EXT_NMEA_DECODER_ALIAS(gns_decoder, protocols::nmea::messages::gns);
CASTLE_EXT_NMEA_DECODER_ALIAS(gsa_decoder, protocols::nmea::messages::gsa);
CASTLE_EXT_NMEA_DECODER_ALIAS(gst_decoder, protocols::nmea::messages::gst);
CASTLE_EXT_NMEA_DECODER_ALIAS(gsv_decoder, protocols::nmea::messages::gsv);
CASTLE_EXT_NMEA_DECODER_ALIAS(rmc_decoder, protocols::nmea::messages::rmc);
CASTLE_EXT_NMEA_DECODER_ALIAS(vtg_decoder, protocols::nmea::messages::vtg);
CASTLE_EXT_NMEA_DECODER_ALIAS(zda_decoder, protocols::nmea::messages::zda);
#undef CASTLE_EXT_NMEA_DECODER_ALIAS

/** @brief Default NMEA decoder registry covering every built-in sentence type. */
template <
    castle::size_type MaxSubscribers = 1U,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
using default_decoder_registry =
    castle::parsers::decoder_registry<
        protocols::nmea::message_view,
        dtm_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gbs_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gga_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gll_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gns_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gsa_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gst_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        gsv_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        rmc_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        vtg_decoder<MaxSubscribers, StorageSize, StorageAlignment>,
        zda_decoder<MaxSubscribers, StorageSize, StorageAlignment>
    >;

/**
 * @brief Fixed, compile-time NMEA decoder registry.
 *
 * @tparam Decoders NMEA-bound `message_decoder` types.
 */
template <typename... Decoders>
using decoder_registry = ::castle::parsers::decoder_registry<
    raw_view_type,
    Decoders...>;

/**
 * @brief Result returned after dispatching one validated NMEA sentence.
 */
using dispatch_result = ::castle::parsers::dispatch_result;

/**
 * @brief Attaches an NMEA-compatible parser to an NMEA decoder registry.
 *
 * @tparam Parser A parser exposing `message_type`, `set_message_callback()`,
 *                and whose `message_type` is `protocols::nmea::message_view`.
 * @tparam Registry A `nmea_parser::decoder_registry<...>`.
 *
 * @warning `registry` must outlive the installed parser callback.
 */
template <typename Parser, typename Registry>
void attach(Parser& parser, Registry& registry) CASTLE_NOEXCEPT
{
    static_assert(
        castle::meta::is_same<typename Parser::message_type, raw_view_type>::value,
        "nmea_parser::attach requires a parser whose message_type is NMEA message_view");
    static_assert(
        castle::meta::is_same<typename Registry::raw_view_type, raw_view_type>::value,
        "nmea_parser::attach requires a registry bound to NMEA message_view");

    ::castle::parsers::attach(parser, registry);
}

} // namespace nmea_parser
} // namespace parsers
} // namespace castle

#endif // CASTLE_EXT_PARSERS_NMEA_PARSER_DECODER_REGISTRY_HPP
