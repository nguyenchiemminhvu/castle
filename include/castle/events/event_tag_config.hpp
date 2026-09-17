// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file event_tag_config.hpp
 * @brief Compile-time event descriptor shared by Castle event dispatchers.
 * @details Use this configuration type to describe one event tag, its callback
 * signature, subscriber capacity, and optional inline callback storage for
 * owning dispatchers. The type stores no runtime state, performs no allocation,
 * and lets `delegate_dispatcher` and `event_dispatcher` share the same event-set
 * declaration.
 *
 * @code
 * #include "castle/events/event_tag_config.hpp"
 *
 * struct TickEvent {};
 *
 * using tick_config = castle::events::event_tag_config<
 *     TickEvent,
 *     4U,
 *     void(uint32_t),
 *     32U>;
 * @endcode
 */
#ifndef CASTLE_EVENTS_EVENT_TAG_CONFIG_HPP
#define CASTLE_EVENTS_EVENT_TAG_CONFIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace events
{

/**
 * @brief Describes one compile-time event entry for Castle dispatcher types.
 * @tparam EventTag Unique tag type used as the compile-time event key.
 * @tparam MaxCallback Maximum number of subscribers allowed for the event.
 * @tparam Signature Callback signature in the form `void(Args...)`.
 * @tparam CallbackStorageSize Inline callback storage reserved by owning dispatchers.
 * @tparam CallbackStorageAlignment Alignment of the inline callback storage.
 * @note `delegate_dispatcher` ignores the storage-related template parameters.
 * @warning `EventTag` identity, not object state, selects the event slot.
 */
template <
    typename EventTag,
    size_type MaxCallback,
    typename Signature,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
struct event_tag_config
{
    /**
     * @brief Tag type used by dispatchers to select this event at compile time.
     */
    using event_tag = EventTag;

    /**
     * @brief Callback signature associated with the event.
     */
    using signature = Signature;

    /**
     * @brief Maximum number of callbacks that the event accepts.
     */
    static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;

    /**
     * @brief Inline callback storage size reserved by owning dispatcher variants.
     */
    static CASTLE_CONSTEXPR size_type callback_storage_size = CallbackStorageSize;

    /**
     * @brief Inline callback storage alignment reserved by owning dispatcher variants.
     */
    static CASTLE_CONSTEXPR size_type callback_storage_alignment = CallbackStorageAlignment;
};

} 
} 

#endif 
