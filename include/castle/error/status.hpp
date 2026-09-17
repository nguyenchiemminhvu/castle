// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Common Castle status codes for deterministic, exception-free error handling.
 *
 * Use this header when an API must report success, capacity exhaustion, invalid input,
 * or similar bounded runtime outcomes without exceptions. The type is a compact
 * `uint8_t` enum and does not allocate memory.
 *
 * @code
 * #include "castle/error/status.hpp"
 *
 * castle::status value = castle::status::not_found;
 * if (!castle::succeeded(value))
 * {
 *     // Handle the bounded error path.
 * }
 * @endcode
 */
#ifndef CASTLE_ERROR_STATUS_HPP
#define CASTLE_ERROR_STATUS_HPP

#include "castle/core/compiler.hpp"

#include <stdint.h>

namespace castle
{

/**
 * @brief Standard status results returned by Castle APIs.
 *
 * @note `status::ok` is the only success value. Every other enumerator represents
 * a failure or rejected operation.
 */
enum class status : uint8_t
{
    ok = 0U,              /**< Operation completed successfully. */
    full,                 /**< A fixed-capacity destination cannot accept more data. */
    empty,                /**< The requested source contains no data or elements. */
    out_of_range,         /**< An index, numeric conversion, or configured bound exceeded the supported range. */
    not_found,            /**< The requested item, key, node, or attribute does not exist. */
    not_configured,       /**< Required configuration or initialization has not been provided yet. */
    invalid_config,       /**< Configuration data exists but is malformed or inconsistent. */
    invalid_argument,     /**< Caller input or parsed source data is invalid for the operation. */
    invalid_callback,     /**< A callback target is missing, malformed, or otherwise unusable. */
    invalid_subscription, /**< A subscription or registration token is invalid. */
    already_exists,       /**< Creation or insertion failed because the target already exists. */
    system_call_error,    /**< A wrapped platform or compiler primitive reported failure. */
    data_loss,            /**< Completing the request would lose information. */
    unknown_error         /**< Fallback status for an unclassified failure. */
};

/**
 * @brief Tests whether a Castle status value represents success.
 *
 * @param value Status code to examine.
 * @return `true` only when `value == status::ok`; otherwise `false`.
 */
CASTLE_CONSTEXPR bool succeeded(status value) CASTLE_NOEXCEPT
{
    return value == status::ok;
}

} // namespace castle

#endif // CASTLE_ERROR_STATUS_HPP
