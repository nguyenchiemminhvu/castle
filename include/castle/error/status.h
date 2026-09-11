#ifndef CASTLE_ERROR_STATUS_H
#define CASTLE_ERROR_STATUS_H

#include "castle/core/compiler.h"

#include <stdint.h>

namespace castle
{

enum class status : uint8_t
{
    ok = 0U,
    full,
    empty,
    out_of_range,
    not_found,
    not_configured,
    invalid_config,
    invalid_argument,
    invalid_callback,
    invalid_subscription,
    already_exists,
    system_call_error,
    unknown_error    
};

CASTLE_CONSTEXPR bool succeeded(status value) CASTLE_NOEXCEPT
{
    return value == status::ok;
}

} // namespace castle

#endif // CASTLE_ERROR_STATUS_H
