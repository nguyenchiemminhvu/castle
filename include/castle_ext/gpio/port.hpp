// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file port.hpp
 * @brief Optional fixed-width GPIO port facade.
 */
#ifndef CASTLE_EXT_GPIO_PORT_HPP
#define CASTLE_EXT_GPIO_PORT_HPP

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
struct has_port_backend_contract : meta::false_type {};

/**
 * @brief Detects whether `Backend` implements the `basic_port` backend contract.
 * @tparam Backend Candidate platform adapter.
 */
template <typename Backend>
struct has_port_backend_contract<Backend, meta::void_t<
    typename Backend::native_handle_type,
    decltype(Backend::read(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type&>())),
    decltype(Backend::read_raw(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type&>())),
    decltype(Backend::write(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type>())),
    decltype(Backend::write_masked(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type>(),
        meta::declval<mask_type>())),
    decltype(Backend::set(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type>())),
    decltype(Backend::clear(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type>())),
    decltype(Backend::toggle(
        meta::declval<typename Backend::native_handle_type CASTLE_CONST&>(),
        meta::declval<mask_type>()))>>
    : meta::true_type {};

} // namespace detail

/**
 * @brief Allocation-free GPIO port facade for backends with native bank/port access.
 *
 * Backend contract:
 *
 * @code
 * struct backend {
 *     using native_handle_type = ...;
 *     static castle::status read(native_handle_type CASTLE_CONST&, mask_type&);
 *     static castle::status read_raw(native_handle_type CASTLE_CONST&, mask_type&);
 *     static castle::status write(native_handle_type CASTLE_CONST&, mask_type);
 *     static castle::status write_masked(native_handle_type CASTLE_CONST&, mask_type, mask_type);
 *     static castle::status set(native_handle_type CASTLE_CONST&, mask_type);
 *     static castle::status clear(native_handle_type CASTLE_CONST&, mask_type);
 *     static castle::status toggle(native_handle_type CASTLE_CONST&, mask_type);
 * };
 * @endcode
 */
template <typename Backend>
class basic_port
{
public:
    using backend_type       = Backend;
    using native_handle_type = typename Backend::native_handle_type;

    static_assert(detail::has_port_backend_contract<Backend>::value,
                  "Backend must implement the basic_port backend contract: "
                  "native_handle_type plus read/read_raw/write/write_masked/set/clear/toggle "
                  "static functions (see basic_port's class doc comment).");

    CASTLE_CONSTEXPR basic_port() CASTLE_NOEXCEPT
        : handle_()
    {
    }

    explicit CASTLE_CONSTEXPR basic_port(native_handle_type CASTLE_CONST& handle) CASTLE_NOEXCEPT
        : handle_(handle)
    {
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR native_handle_type CASTLE_CONST& native_handle() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return handle_;
    }

    void reset(native_handle_type CASTLE_CONST& handle) CASTLE_NOEXCEPT
    {
        handle_ = handle;
    }

    CASTLE_NODISCARD castle::status read(mask_type& out_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Backend::read(handle_, out_value);
    }

    CASTLE_NODISCARD castle::status read_raw(mask_type& out_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Backend::read_raw(handle_, out_value);
    }

    CASTLE_NODISCARD castle::status write(mask_type value) CASTLE_NOEXCEPT
    {
        return Backend::write(handle_, value);
    }

    CASTLE_NODISCARD castle::status write_masked(mask_type mask, mask_type value) CASTLE_NOEXCEPT
    {
        return Backend::write_masked(handle_, mask, value);
    }

    CASTLE_NODISCARD castle::status set(mask_type mask) CASTLE_NOEXCEPT
    {
        return Backend::set(handle_, mask);
    }

    CASTLE_NODISCARD castle::status clear(mask_type mask) CASTLE_NOEXCEPT
    {
        return Backend::clear(handle_, mask);
    }

    CASTLE_NODISCARD castle::status toggle(mask_type mask) CASTLE_NOEXCEPT
    {
        return Backend::toggle(handle_, mask);
    }

private:
    native_handle_type handle_;
};

template <typename Backend>
using port = basic_port<Backend>;

} // namespace gpio
} // namespace castle

#endif // CASTLE_EXT_GPIO_PORT_HPP
