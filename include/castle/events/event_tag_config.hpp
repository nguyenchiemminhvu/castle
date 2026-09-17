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

// -----------------------------------------------------------------------------
// event_tag_config
// -----------------------------------------------------------------------------
// Unified compile-time descriptor of one event slot for BOTH delegate_dispatcher
// (non-owning callbacks) and event_dispatcher (owning callbacks).
//
// event_tag_config carries only compile-time traits — it does NOT embed a
// registry. Each dispatcher builds the concrete registry type it needs from
// the fields exposed here, exactly the way signal_event / signal_ipc_event
// consume signal_config:
//   - delegate_dispatcher         uses event_tag, max_callback, signature.
//   - event_dispatcher uses event_tag, max_callback, signature,
//                              callback_storage_size, callback_storage_alignment.
//
// Sharing a single config type means the user learns one spelling and can
// switch between the two dispatchers by changing only the outer class name;
// the pack of event_tag_config<...> entries is reused verbatim.
//
// Template parameters:
//   EventTag                    - the TYPE used as a compile-time key to look
//                                 up this slot. Carries no payload; only its
//                                 identity matters. Can be an empty struct,
//                                 an enum type, or any regular type.
//   MaxCallback                 - maximum number of concurrent subscribers.
//   Signature                   - callback signature, e.g. void(Args...).
//                                 Passed as a function type so both the return
//                                 type (required void) and the argument pack
//                                 are described in one canonical spelling.
//   CallbackStorageSize         - function inline buffer size. Used
//                                 only by event_dispatcher; ignored
//                                 by delegate_dispatcher.
//   CallbackStorageAlignment    - function inline buffer alignment.
//                                 Used only by event_dispatcher;
//                                 ignored by delegate_dispatcher.
//
// Examples:
//   struct TimerExpired   {};
//   struct VehicleStarted {};
//   struct Shutdown       {};
//
//   using configs =
//       event_tag_config<TimerExpired,   8, void(uint32_t, uint32_t)>,
//       event_tag_config<VehicleStarted, 4, void(uint8_t)>,
//       event_tag_config<Shutdown,       2, void(), 32>;
//
//   using dispatcher_t         = delegate_dispatcher<configs...>;
//   using inplace_dispatcher_t = event_dispatcher<configs...>;
// -----------------------------------------------------------------------------
template <
    typename EventTag,
    size_type MaxCallback,
    typename Signature,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
struct event_tag_config
{
    using event_tag = EventTag;
    using signature = Signature;

    static CASTLE_CONSTEXPR size_type max_callback = MaxCallback;
    static CASTLE_CONSTEXPR size_type callback_storage_size = CallbackStorageSize;
    static CASTLE_CONSTEXPR size_type callback_storage_alignment = CallbackStorageAlignment;
};

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_EVENT_TAG_CONFIG_HPP
