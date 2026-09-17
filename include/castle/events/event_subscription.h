#pragma once

#include <cstdint>

namespace castle
{
namespace events
{

enum class event_subscription_error : std::uint8_t
{
    ok = 0,
    full,
    invalid_callback,
    invalid_subscription,
    event_disabled,
    not_configured,
    invalid_config,
    system_call_error,
    unknown_error,
};

} // namespace events
} // namespace castle