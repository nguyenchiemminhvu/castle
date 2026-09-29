// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file pin.hpp
 * @brief Backend-independent, allocation-free GPIO pin wrapper.
 */
#ifndef CASTLE_EXT_GPIO_PIN_HPP
#define CASTLE_EXT_GPIO_PIN_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/gpio/types.hpp"

namespace castle
{
namespace gpio
{

namespace detail
{

template <typename Backend, typename = void>
struct has_pin_backend_contract : meta::false_type {};

/**
 * @brief Detects whether `Backend` implements the `basic_pin` backend contract.
 * @tparam Backend Candidate platform adapter.
 */
template <typename Backend>
struct has_pin_backend_contract<Backend, meta::void_t<
    typename Backend::native_handle_type,
    decltype(Backend::configure(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<pin_config CASTLE_CONST&>())),
    decltype(Backend::write(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<level>())),
    decltype(Backend::read(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<level&>())),
    decltype(Backend::write_raw(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<level>())),
    decltype(Backend::read_raw(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<level&>())),
    decltype(Backend::toggle(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>()))>>
    : meta::true_type {};

} // namespace detail

/**
 * @brief Compile-time GPIO pin facade over a platform backend.
 *
 * Backend contract:
 *
 * @code
 * struct backend {
 *     using native_handle_type = ...;
 *
 *     static castle::status configure(native_handle_type CASTLE_CONST&, pin_config CASTLE_CONST&);
 *     static castle::status write(native_handle_type CASTLE_CONST&, level);
 *     static castle::status read(native_handle_type CASTLE_CONST&, level&);
 *     static castle::status write_raw(native_handle_type CASTLE_CONST&, level);
 *     static castle::status read_raw(native_handle_type CASTLE_CONST&, level&);
 *     static castle::status toggle(native_handle_type CASTLE_CONST&);
 * };
 * @endcode
 *
 * `write`/`read` use the backend's logical semantics; `write_raw`/`read_raw`
 * address the physical pin state when the platform exposes that distinction.
 * The wrapper stores only the backend's native handle, so its size is exactly
 * the size required by the backend binding.
 *
 * Thread/ISR safety is intentionally delegated to the backend. No mutex,
 * atomic flag, heap storage, or hidden critical section is introduced by this
 * class because those mechanisms are not universally ISR-safe.
 *
 * @tparam Backend Platform adapter implementing the contract above.
 */
template <typename Backend>
class basic_pin
{
public:
    using backend_type       = Backend;
    using native_handle_type = typename Backend::native_handle_type;

    static_assert(detail::has_pin_backend_contract<Backend>::value,
                  "Backend must implement the basic_pin backend contract: "
                  "native_handle_type plus configure/write/read/write_raw/read_raw/toggle "
                  "static functions (see basic_pin's class doc comment).");

    /** @brief Default-constructs an empty native handle. */
    CASTLE_CONSTEXPR basic_pin() CASTLE_NOEXCEPT
        : handle_()
    {
    }

    /** @brief Binds the pin to a platform-native handle. */
    explicit CASTLE_CONSTEXPR basic_pin(native_handle_type CASTLE_CONST& handle) CASTLE_NOEXCEPT
        : handle_(handle)
    {
    }

    /** @brief Returns the bound platform-native handle. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR native_handle_type CASTLE_CONST& native_handle() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return handle_;
    }

    /** @brief Returns a mutable platform-native handle when an adapter requires it. */
    CASTLE_NODISCARD native_handle_type& native_handle() CASTLE_NOEXCEPT
    {
        return handle_;
    }

    /** @brief Rebinds the object to another native GPIO handle. */
    void reset(native_handle_type CASTLE_CONST& handle) CASTLE_NOEXCEPT
    {
        handle_ = handle;
    }

    /** @brief Configures the pin through the backend. */
    CASTLE_NODISCARD castle::status configure(pin_config CASTLE_CONST& config) CASTLE_NOEXCEPT
    {
        return Backend::configure(handle_, config);
    }

    /** @brief Writes a logical GPIO level. */
    CASTLE_NODISCARD castle::status write(level value) CASTLE_NOEXCEPT
    {
        return Backend::write(handle_, value);
    }

    /** @brief Writes a logical GPIO level from a boolean. */
    CASTLE_NODISCARD castle::status write(bool value) CASTLE_NOEXCEPT
    {
        return write(to_level(value));
    }

    /** @brief Reads a logical GPIO level. */
    CASTLE_NODISCARD castle::status read(level& out_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Backend::read(handle_, out_value);
    }

    /** @brief Reads a logical GPIO level into a boolean. */
    CASTLE_NODISCARD castle::status read(bool& out_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        level value = level::low;
        CASTLE_CONST castle::status result = read(value);
        if (castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            out_value = to_bool(value);
        }
        return result;
    }

    /** @brief Writes a physical high/low level, ignoring active-low polarity when supported. */
    CASTLE_NODISCARD castle::status write_raw(level value) CASTLE_NOEXCEPT
    {
        return Backend::write_raw(handle_, value);
    }

    /** @brief Reads a physical high/low level, ignoring active-low polarity when supported. */
    CASTLE_NODISCARD castle::status read_raw(level& out_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Backend::read_raw(handle_, out_value);
    }

    /** @brief Toggles the logical GPIO output using the backend's native operation. */
    CASTLE_NODISCARD castle::status toggle() CASTLE_NOEXCEPT
    {
        return Backend::toggle(handle_);
    }

private:
    native_handle_type handle_;
};

/**
 * @brief Convenience alias for the common boolean-facing GPIO use case.
 */
template <typename Backend>
using pin = basic_pin<Backend>;

} // namespace gpio
} // namespace castle

#endif // CASTLE_EXT_GPIO_PIN_HPP
