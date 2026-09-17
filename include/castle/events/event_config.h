#ifndef CASTLE_EVENTS_EVENT_CONFIG_H
#define CASTLE_EVENTS_EVENT_CONFIG_H

#include "castle/core/compiler.h"
#include "castle/core/config.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

namespace castle
{
namespace events
{

// -----------------------------------------------------------------------------
// event_config
// -----------------------------------------------------------------------------
// Unified compile-time descriptor of one event slot for BOTH event_dispatcher
// (non-owning callbacks) and inplace_event_dispatcher (owning callbacks).
//
// event_config carries only compile-time traits — it does NOT embed a
// registry. Each dispatcher builds the concrete registry type it needs from
// the fields exposed here, exactly the way signal_event / inplace_signal_event
// consume signal_config:
//   - event_dispatcher         uses event_tag, max_callback, signature.
//   - inplace_event_dispatcher uses event_tag, max_callback, signature,
//                              callback_storage_size, callback_storage_alignment.
//
// Sharing a single config type means the user learns one spelling and can
// switch between the two dispatchers by changing only the outer class name;
// the pack of event_config<...> entries is reused verbatim.
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
//   CallbackStorageSize         - inplace_function inline buffer size. Used
//                                 only by inplace_event_dispatcher; ignored
//                                 by event_dispatcher.
//   CallbackStorageAlignment    - inplace_function inline buffer alignment.
//                                 Used only by inplace_event_dispatcher;
//                                 ignored by event_dispatcher.
//
// Examples:
//   struct TimerExpired   {};
//   struct VehicleStarted {};
//   struct Shutdown       {};
//
//   using configs =
//       event_config<TimerExpired,   8, void(uint32_t, uint32_t)>,
//       event_config<VehicleStarted, 4, void(uint8_t)>,
//       event_config<Shutdown,       2, void(), 32>;
//
//   using dispatcher_t         = event_dispatcher<configs...>;
//   using inplace_dispatcher_t = inplace_event_dispatcher<configs...>;
// -----------------------------------------------------------------------------
template <
    typename EventTag,
    size_type MaxCallback,
    typename Signature,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
struct event_config
{
    using event_tag = EventTag;
    using signature = Signature;

    static constexpr size_type max_callback = MaxCallback;
    static constexpr size_type callback_storage_size = CallbackStorageSize;
    static constexpr size_type callback_storage_alignment = CallbackStorageAlignment;
};

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_EVENT_CONFIG_H
