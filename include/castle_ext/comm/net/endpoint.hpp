// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file endpoint.hpp
 * @brief Fixed-size IPv4 and IPv6 endpoint value types.
 *
 * Endpoints keep addresses as bytes and ports in host byte order. They do not
 * depend on the host socket ABI, perform DNS lookups, allocate memory, or own
 * resources. `socket.hpp` translates them to and from native BSD socket address
 * structures.
 */
#ifndef CASTLE_EXT_COMM_NET_ENDPOINT_HPP
#define CASTLE_EXT_COMM_NET_ENDPOINT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{
namespace comm
{
namespace net
{

/** @brief IP address family supported by CASTLE's BSD socket layer. */
enum class address_family : uint8_t
{
    ipv4 = 4U,
    ipv6 = 6U
};

/**
 * @brief IP endpoint containing a numeric address and a host-order port.
 *
 * IPv4 addresses are stored in the first four bytes; IPv6 addresses use all
 * sixteen bytes. IPv6 scope identifiers are retained for link-local addresses.
 * The default endpoint is IPv4 any-address with port zero.
 */
class endpoint
{
public:
    /** @brief Creates the IPv4 any-address endpoint on port zero. */
    CASTLE_CONSTEXPR endpoint() CASTLE_NOEXCEPT
        : address_{}
        , family_(address_family::ipv4)
        , port_(0U)
        , scope_id_(0U)
    {
    }

    /** @brief Builds an IPv4 endpoint from four address octets and a host-order port. */
    static CASTLE_CONSTEXPR endpoint ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
                                          uint16_t port) CASTLE_NOEXCEPT
    {
        endpoint result;
        result.address_[0] = a;
        result.address_[1] = b;
        result.address_[2] = c;
        result.address_[3] = d;
        result.port_ = port;
        return result;
    }

    /** @brief Returns the IPv4 wildcard address (`0.0.0.0`) on `port`. */
    static CASTLE_CONSTEXPR endpoint any_ipv4(uint16_t port) CASTLE_NOEXCEPT
    {
        return ipv4(0U, 0U, 0U, 0U, port);
    }

    /** @brief Returns the IPv4 loopback address (`127.0.0.1`) on `port`. */
    static CASTLE_CONSTEXPR endpoint loopback_ipv4(uint16_t port) CASTLE_NOEXCEPT
    {
        return ipv4(127U, 0U, 0U, 1U, port);
    }

    /**
     * @brief Builds an IPv6 endpoint from sixteen address bytes and a host-order port.
     * @param bytes Pointer to sixteen address bytes. A null pointer produces the any-address.
     * @param port Host-order TCP/UDP port.
     * @param scope_id IPv6 scope identifier, usually used for link-local addresses.
     */
    static CASTLE_CONSTEXPR endpoint ipv6(CASTLE_CONST uint8_t* bytes, uint16_t port,
                                          uint32_t scope_id = 0U) CASTLE_NOEXCEPT
    {
        endpoint result;
        result.family_ = address_family::ipv6;
        result.port_ = port;
        result.scope_id_ = scope_id;
        if (bytes != nullptr)
        {
            for (size_type i = 0U; i < 16U; ++i)
            {
                result.address_[i] = bytes[i];
            }
        }
        return result;
    }

    /** @brief Returns the IPv6 wildcard address (`::`) on `port`. */
    static CASTLE_CONSTEXPR endpoint any_ipv6(uint16_t port) CASTLE_NOEXCEPT
    {
        return ipv6(nullptr, port);
    }

    /** @brief Returns the IPv6 loopback address (`::1`) on `port`. */
    static CASTLE_CONSTEXPR endpoint loopback_ipv6(uint16_t port) CASTLE_NOEXCEPT
    {
        uint8_t bytes[16] = {};
        bytes[15] = 1U;
        return ipv6(bytes, port);
    }

    /** @brief Returns the endpoint's address family. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR address_family family() CASTLE_CONST CASTLE_NOEXCEPT { return family_; }
    /** @brief Returns the TCP/UDP port in host byte order. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR uint16_t port() CASTLE_CONST CASTLE_NOEXCEPT { return port_; }
    /** @brief Returns the IPv6 scope identifier (zero for IPv4). */
    CASTLE_NODISCARD CASTLE_CONSTEXPR uint32_t scope_id() CASTLE_CONST CASTLE_NOEXCEPT { return scope_id_; }
    /** @brief Returns a pointer to four IPv4 or sixteen IPv6 address bytes. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST uint8_t* address_bytes() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return address_;
    }
    /** @brief Returns four for IPv4 and sixteen for IPv6. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type address_size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return family_ == address_family::ipv4 ? 4U : 16U;
    }

    /** @brief Tests whether every address byte is zero. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool is_any() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < address_size(); ++i)
        {
            if (address_[i] != 0U)
            {
                return false;
            }
        }
        return true;
    }

    /** @brief Tests endpoint equality including family, port, address and IPv6 scope. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator==(endpoint CASTLE_CONST& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (family_ != other.family_ || port_ != other.port_ || scope_id_ != other.scope_id_)
        {
            return false;
        }
        for (size_type i = 0U; i < address_size(); ++i)
        {
            if (address_[i] != other.address_[i])
            {
                return false;
            }
        }
        return true;
    }

    /** @brief Tests endpoint inequality. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool operator!=(endpoint CASTLE_CONST& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    uint8_t address_[16];
    address_family family_;
    uint16_t port_;
    uint32_t scope_id_;
};

} // namespace net
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_NET_ENDPOINT_HPP
